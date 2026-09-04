// rc/rt/cache_line.hpp
//
// How far apart two hot variables have to be, from lesson 07-05.
//
// Caches do not move bytes, they move lines, and a line is 64 bytes on every
// processor this curriculum targets. Two threads writing to two different
// variables in the same line are, as far as the hardware is concerned, writing
// to the same thing: each write takes the line away from the other core.
//
// Measured with two atomic counters and two threads, one and a half million
// increments each: 48.8 ms when the counters were 8 bytes apart, 55.2 ms at 16,
// and 15.2 ms once they crossed a line boundary. Nothing changes until they
// cross it, and then everything does.
//
// The most common way to write this bug is the most natural code there is: a
// vector with one slot per thread. Four threads incrementing their own slot in
// a std::vector<std::atomic<long>> took 60.5 ms; one slot per cache line took
// 10.2; a local on the stack, published once at the end, took 2.9.
//
// C++17 named the constant std::hardware_destructive_interference_size. The
// standard library on Ubuntu 22.04 does not provide it, which is a fair summary
// of how portable that answer is, so this is a plain constant with a
// measurement behind it.

#ifndef RC_RT_CACHE_LINE
#define RC_RT_CACHE_LINE

#include <cstddef>

namespace rc {
namespace rt {

constexpr std::size_t kCacheLine = 64;

// A value with a cache line to itself.
//
// alignas on the type rather than on a member, so the padding survives being
// put in an array: a vector of these has one element per line, which is exactly
// what a vector of the bare type does not give you.
// MSVC warns, at /W4, that it padded the structure because of the alignment
// specifier. That is exactly what was asked for, so the warning is turned off
// here rather than worked around: C4324 is the compiler confirming the request,
// and the alternative spellings that avoid it are the ones E-RT-0007 is about.
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4324)
#endif
template <class T>
struct alignas(kCacheLine) Padded {
  T value{};

  T& operator*() { return value; }
  const T& operator*() const { return value; }
  T* operator->() { return &value; }
  const T* operator->() const { return &value; }
};
#ifdef _MSC_VER
#pragma warning(pop)
#endif

}  // namespace rt
}  // namespace rc

#endif  // RC_RT_CACHE_LINE
