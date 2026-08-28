// rc/core/compat.hpp
//
// The bridge between the C++ this curriculum teaches and the C++ that came
// after it.
//
// Every lesson here is written in C++17. That is a deliberate choice. C++17 is
// available on every compiler you are likely to meet, including the GCC 11 that
// ships with Ubuntu 22.04 and the toolchains inside a great deal of robotics
// hardware, and it is complete enough that nothing in this curriculum has to be
// written in an old style to accommodate it.
//
// The three facilities below arrived later, in C++20 and C++23, and they are
// genuinely useful. Rather than pretend they do not exist or force a newer
// compiler on everybody, this file provides them:
//
//   rc::span      a view of contiguous values      (std::span, C++20)
//   rc::format    formatting with {} placeholders  (std::format, C++20)
//   rc::expected  a value or an error              (std::expected, C++23)
//
// On a toolchain that has the standard version, rc:: is an alias for it and you
// get the real thing. On one that does not, you get the small implementation
// written here.
//
// Read this file. It is the whole point of teaching C++17 first: you write and
// understand these facilities, and then when you move to C++20 or C++23 you are
// not learning a new idea, you are deleting your own code and changing an
// include. That is a much better position than meeting std::span as magic.

#ifndef RC_CORE_COMPAT_HPP
#define RC_CORE_COMPAT_HPP

#include <cstddef>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>

#if defined(__has_include)
#  if __has_include(<version>)
#    include <version>
#  endif
#endif

// Feature detection and the includes it implies happen here, at global scope.
// A standard header included inside a namespace produces hundreds of errors
// inside files you never wrote, which is catalogued as E-CPP-0003 and is worth
// causing once on purpose to see what it looks like.

#if defined(__cpp_lib_span) && __cpp_lib_span >= 202002L && __cplusplus >= 202002L
#  define RC_HAS_STD_SPAN 1
#  include <span>
#else
#  define RC_HAS_STD_SPAN 0
#endif

#if defined(__cpp_lib_format) && __cpp_lib_format >= 201907L
#  define RC_HAS_STD_FORMAT 1
#  include <format>
#else
#  define RC_HAS_STD_FORMAT 0
#endif

#if defined(__cpp_lib_expected) && __cpp_lib_expected >= 202202L
#  define RC_HAS_STD_EXPECTED 1
#  include <expected>
#else
#  define RC_HAS_STD_EXPECTED 0
#  include <cassert>
#  include <variant>
#endif

namespace rc {

// ---------------------------------------------------------------------------
// rc::span
// ---------------------------------------------------------------------------
// A pointer to the first element and a count. Nothing more, and in particular
// no ownership: a span does not keep the data it views alive.
//
// This is how you pass a block of readings to a function without caring whether
// they came from a plain array or a std::vector, and without copying them.
//
// C++20 standardised this as std::span. What you read below is the same idea in
// about sixty lines, which is roughly what it actually is.

#if RC_HAS_STD_SPAN
template <class T>
using span = std::span<T>;
#else
template <class T>
class span {
 public:
  using element_type = T;
  using value_type = typename std::remove_cv<T>::type;
  using size_type = std::size_t;
  using iterator = T*;

  constexpr span() noexcept : data_(nullptr), size_(0) {}

  constexpr span(T* first, size_type count) noexcept : data_(first), size_(count) {}

  // From a plain array. The size is part of the array's type, so it is deduced
  // and never has to be passed separately. This is exactly the bug class that
  // a pointer and a hand written length keeps producing.
  template <std::size_t N>
  constexpr span(T (&array)[N]) noexcept : data_(array), size_(N) {}

  // From anything exposing data() and size(), which covers std::vector and
  // std::array. The trailing template parameters are the C++17 way of saying
  // "only when this expression is valid", a technique named SFINAE. C++20
  // replaced it with concepts, which say the same thing in a readable sentence.
  // Seeing both is the point of this file.
  template <class Container,
            class = decltype(std::declval<Container&>().data()),
            class = decltype(std::declval<Container&>().size()),
            class = typename std::enable_if<
                !std::is_same<typename std::decay<Container>::type, span>::value>::type>
  constexpr span(Container& container)
      : data_(container.data()), size_(container.size()) {}

  constexpr T* data() const noexcept { return data_; }
  constexpr size_type size() const noexcept { return size_; }
  constexpr bool empty() const noexcept { return size_ == 0; }

  constexpr T& operator[](size_type index) const { return data_[index]; }
  constexpr T& front() const { return data_[0]; }
  constexpr T& back() const { return data_[size_ - 1]; }

  constexpr iterator begin() const noexcept { return data_; }
  constexpr iterator end() const noexcept { return data_ + size_; }

  // A view of part of this view. Out of range requests are clamped rather than
  // being undefined behaviour, because a curriculum should not hand beginners a
  // silent memory bug in a helper they did not write.
  constexpr span subspan(size_type offset, size_type count) const {
    if (offset > size_) return span();
    const size_type available = size_ - offset;
    return span(data_ + offset, count < available ? count : available);
  }

 private:
  T* data_;
  size_type size_;
};
#endif

// ---------------------------------------------------------------------------
// rc::format
// ---------------------------------------------------------------------------
// Replaces every "{}" in the pattern with the next argument, in order.
//
// The fallback understands "{}" and nothing else, which is all this curriculum
// uses. Keep patterns simple and your code builds identically on a toolchain
// that has std::format and one that does not.

#if RC_HAS_STD_FORMAT
template <class... Args>
std::string format(std::format_string<Args...> pattern, Args&&... args) {
  return std::format(pattern, std::forward<Args>(args)...);
}
#else
namespace detail {

inline void format_into(std::ostringstream& out, const std::string& pattern) {
  out << pattern;
}

template <class First, class... Rest>
void format_into(std::ostringstream& out, const std::string& pattern,
                 const First& first, const Rest&... rest) {
  const std::size_t hole = pattern.find("{}");
  if (hole == std::string::npos) {
    out << pattern;
    return;
  }
  out << pattern.substr(0, hole) << first;
  format_into(out, pattern.substr(hole + 2), rest...);
}

}  // namespace detail

template <class... Args>
std::string format(const std::string& pattern, const Args&... args) {
  std::ostringstream out;
  detail::format_into(out, pattern, args...);
  return out.str();
}
#endif

// ---------------------------------------------------------------------------
// rc::expected
// ---------------------------------------------------------------------------
// A value or an error, in one return type. It replaces the habit of returning
// a bool and writing the real answer into an output parameter, which is easy to
// ignore, and it replaces throwing where an error is an ordinary outcome rather
// than an exceptional one.
//
// A serial port that cannot open is not exceptional. It is Tuesday.

#if RC_HAS_STD_EXPECTED
template <class T, class E>
using expected = std::expected<T, E>;
template <class E>
using unexpected = std::unexpected<E>;
#else
template <class E>
class unexpected {
 public:
  explicit unexpected(E error) : error_(std::move(error)) {}
  const E& error() const& { return error_; }
  E& error() & { return error_; }

 private:
  E error_;
};

template <class T, class E>
class expected {
 public:
  expected() : store_(T{}) {}
  expected(T value) : store_(std::move(value)) {}
  expected(unexpected<E> error) : store_(std::move(error.error())) {}

  bool has_value() const { return store_.index() == 0; }
  explicit operator bool() const { return has_value(); }

  const T& value() const& {
    assert(has_value() && "value() was called on an expected holding an error");
    return std::get<0>(store_);
  }
  T& value() & {
    assert(has_value() && "value() was called on an expected holding an error");
    return std::get<0>(store_);
  }

  const E& error() const& {
    assert(!has_value() && "error() was called on an expected holding a value");
    return std::get<1>(store_);
  }

  const T& operator*() const& { return value(); }
  T& operator*() & { return value(); }

  T value_or(T fallback) const {
    return has_value() ? std::get<0>(store_) : std::move(fallback);
  }

 private:
  std::variant<T, E> store_;
};
#endif

}  // namespace rc

#endif  // RC_CORE_COMPAT_HPP
