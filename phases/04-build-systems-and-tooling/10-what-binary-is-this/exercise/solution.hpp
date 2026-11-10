#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cstddef>
#include <cstring>
#include <string>
#include <vector>

// What a binary was built with. Every field is something that changes what the program does or
// how fast it does it.
struct BuildInfo {
  std::string compiler;        // "gcc", "clang" or "msvc"
  int version = 0;             // the major version, or _MSC_VER for msvc
  int standard = 17;           // the C++ standard, 17 for C++17
  bool optimized = false;      // true when NDEBUG is set, which is what a release build does
  std::string sanitizer;       // "none", "address" or "thread"
  int pointer_bits = 64;
  bool little_endian = true;
};

// The build this code was compiled into. Given.
inline BuildInfo current_build() {
  BuildInfo b;

#if defined(__clang__)
  b.compiler = "clang";
  b.version = __clang_major__;
#elif defined(_MSC_VER)
  b.compiler = "msvc";
  b.version = _MSC_VER;
#elif defined(__GNUC__)
  b.compiler = "gcc";
  b.version = __GNUC__;
#else
  b.compiler = "unknown";
#endif

#if defined(_MSVC_LANG)
  b.standard = static_cast<int>((_MSVC_LANG - 200000L) / 100L);     // 201703 is C++17
#else
  b.standard = static_cast<int>((__cplusplus - 200000L) / 100L);
#endif

#if defined(NDEBUG)
  b.optimized = true;
#endif

  b.sanitizer = "none";
#if defined(__SANITIZE_ADDRESS__)
  b.sanitizer = "address";
#elif defined(__SANITIZE_THREAD__)
  b.sanitizer = "thread";
#elif defined(__has_feature)
#  if __has_feature(address_sanitizer)
  b.sanitizer = "address";
#  elif __has_feature(thread_sanitizer)
  b.sanitizer = "thread";
#  endif
#endif

  b.pointer_bits = static_cast<int>(sizeof(void*) * 8);

  // Found by looking at the bytes of a number, not by asking a macro.
  const unsigned int one = 1;
  b.little_endian = *reinterpret_cast<const unsigned char*>(&one) == 1;
  return b;
}

// One line a person can read and a program can read back, such as
//   gcc-11 c++17 release none 64bit little
inline std::string describe(const BuildInfo& b) {
  // TODO: six words separated by single spaces:
  //   <compiler>-<version>  c++<standard>  release or debug (optimized)  <sanitizer>
  //   <pointer_bits>bit  little or big (little_endian)
  // for example "gcc-11 c++17 release none 64bit little". std::to_string turns a number into text.
  (void)b;
  return "";
}

namespace build_detail {

inline bool read_int(const std::string& s, int* out) {
  if (s.empty() || s.size() > 9) return false;
  for (char c : s) {
    if (c < '0' || c > '9') return false;
  }
  *out = std::stoi(s);
  return true;
}

// Split a line at single spaces. Two spaces in a row, or a space at either end, make an empty piece,
// which is how a malformed line is noticed.
inline std::vector<std::string> split(const std::string& line) {
  std::vector<std::string> parts;
  std::string::size_type start = 0;
  while (true) {
    const std::string::size_type space = line.find(' ', start);
    if (space == std::string::npos) {
      parts.push_back(line.substr(start));
      break;
    }
    parts.push_back(line.substr(start, space - start));
    start = space + 1;
  }
  return parts;
}

}  // namespace build_detail

// Read back what describe wrote. False, leaving *out alone, for anything that is not exactly that format.
inline bool parse_build(const char* text, BuildInfo* out) {
  // TODO: build_detail::split(text) gives the pieces; there must be exactly 6. Piece 0 is
  // compiler-version (cut at the LAST '-', compiler must be gcc, clang, msvc or unknown, version through
  // build_detail::read_int). Piece 1 starts with "c++" followed by the standard. Piece 2 is
  // "release" or "debug". Piece 3 is "none", "address" or "thread". Piece 4 is a number then "bit".
  // Piece 5 is "little" or "big". Anything else returns false and leaves *out alone.
  (void)text;
  (void)out;
  return false;
}

// May a speed measured on one of these be compared with a speed measured on the other?
//
// Only if it is the same compiler and version, both optimised or both not, and neither carries a
// sanitizer: a sanitizer slows a program by an amount that depends on what it does, so a time
// measured under one says nothing about the same code without it.
inline bool comparable_for_speed(const BuildInfo& a, const BuildInfo& b) {
  // TODO: false if either has a sanitizer other than "none". Otherwise true only if the compiler and
  // version match, both are optimised or both are not, and pointer_bits match.
  (void)a;
  (void)b;
  return false;
}

// Reasons not to trust a speed measured on this build, one string each. Empty for a build that is fit to time.
inline std::vector<std::string> speed_warnings(const BuildInfo& b) {
  // TODO: one string for a build that is not optimised, and one for a build with a sanitizer (its name in
  // the message). A build that is optimised with no sanitizer gives an empty list.
  (void)b;
  return std::vector<std::string>();
}

#endif  // LESSON_SOLUTION_HPP
