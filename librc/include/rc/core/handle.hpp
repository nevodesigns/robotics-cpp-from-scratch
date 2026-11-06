// rc/core/handle.hpp
//
// Owning a handle a C library gave you, from lesson 02-16, graduated.
//
// A C library hands out an opaque pointer from an open function and wants exactly one call to its own close function on it. Written by hand, the
// early return is where it leaks: over 100 attempts where the second of two devices could not be opened, raw handles left 100 open and the owner
// left 0. A std::unique_ptr with the library's close as its deleter closes on every way out of the scope, an exception included, never calls the
// close on a handle that failed to open, closes once when the duty is handed over with std::move, and costs no more than the raw pointer (8 bytes
// for both on the machine measured, since the deleter has no data).
//
// The lesson writes the closer for one fake device by hand. This is the same thing as a template, for any type and any close function:
//
//     void close_port(Port*);
//     using PortPtr = rc::core::Handle<Port, close_port>;

#ifndef RC_CORE_HANDLE_HPP
#define RC_CORE_HANDLE_HPP

#include <memory>

namespace rc {
namespace core {

template <typename T, void (*Close)(T*)>
struct Closer {
  void operator()(T* handle) const {
    if (handle != nullptr) Close(handle);
  }
};

template <typename T, void (*Close)(T*)>
using Handle = std::unique_ptr<T, Closer<T, Close>>;

}  // namespace core
}  // namespace rc

#endif  // RC_CORE_HANDLE_HPP
