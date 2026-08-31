// rc/io/handle.hpp
//
// The device handle from lesson 02-03, graduated.
//
// A resource is acquired before the object exists and released when it stops
// existing, so there is no window in which a half built handle is visible and
// no exit path that can forget to close. That includes the exit path somebody
// adds next year without reading the file, which is the whole point.
//
// The lesson's version was welded to one fake open and close pair, because a
// lesson on destructors should not also be a lesson on templates. A driver
// closes a file descriptor, a serial port, a socket or a vendor library
// context, so the graduated version takes the closing action as a parameter.

#ifndef RC_IO_HANDLE_HPP
#define RC_IO_HANDLE_HPP

#include <utility>

namespace rc {
namespace io {

// Owns one resource of type T and releases it exactly once.
//
// Empty is the value that means "holds nothing", which for a POSIX file
// descriptor is -1 and for a pointer is nullptr. It is a constructor argument
// rather than a template parameter because a double cannot be a template
// argument in C++17 and a driver should not have to care which of its resource
// types can.
template <class T, class Closer>
class Handle {
 public:
  Handle(T resource, T empty, Closer closer)
      : resource_(resource), empty_(empty), closer_(std::move(closer)) {}

  ~Handle() { close(); }

  // Copying is refused. Two handles holding one resource would each release it,
  // and a double release can shut something another part of the program has
  // since opened for its own purposes, which is a bug that reads as impossible
  // at the site where it goes wrong. Deleting the operation reports it at the
  // line attempting the copy, which is where it actually is.
  Handle(const Handle&) = delete;
  Handle& operator=(const Handle&) = delete;

  // Moving is allowed, and leaves the source empty. That is what makes a handle
  // returnable from a factory function without the resource being released on
  // the way out.
  Handle(Handle&& other) noexcept
      : resource_(other.resource_), empty_(other.empty_),
        closer_(std::move(other.closer_)) {
    other.resource_ = other.empty_;
  }

  Handle& operator=(Handle&& other) noexcept {
    if (this != &other) {
      close();
      resource_ = other.resource_;
      empty_ = other.empty_;
      closer_ = std::move(other.closer_);
      other.resource_ = other.empty_;
    }
    return *this;
  }

  bool is_open() const { return !(resource_ == empty_); }
  T get() const { return resource_; }

  // Marks the handle empty before releasing, so a second call has nothing left
  // to do. That is what makes close and the destructor safe together, in either
  // order and any number of times.
  void close() {
    if (!is_open()) return;
    const T held = resource_;
    resource_ = empty_;
    closer_(held);
  }

  // Hands the resource out and gives up ownership, for the case where something
  // else must take over. The handle is empty afterwards and will not close it.
  T release() {
    const T held = resource_;
    resource_ = empty_;
    return held;
  }

 private:
  T resource_;
  T empty_;
  Closer closer_;
};

// Deduces the closer, so callers write make_handle(fd, -1, ::close) rather than
// naming the type of a lambda, which cannot be named.
template <class T, class Closer>
Handle<T, Closer> make_handle(T resource, T empty, Closer closer) {
  return Handle<T, Closer>(resource, empty, std::move(closer));
}

}  // namespace io
}  // namespace rc

#endif  // RC_IO_HANDLE_HPP
