#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <rc/core/compat.hpp>

// You have written a great many functions by now and nobody has said what
// happens when one is called. This lesson says it, by measuring rather than
// explaining, and the instrument is the address of the data.
//
// A std::vector keeps its elements in a buffer somewhere else, and data()
// reports where. If a callee sees the same address the caller had, no copy was
// made. If it sees a different one, a whole new buffer was allocated and the
// elements were copied into it, and nothing in the source said so.
//
// Every function below reports an address as a plain number and never
// dereferences it, so none of this depends on a callee's copy still existing
// by the time the answer is read.

using Address = std::uintptr_t;

// Where a buffer lives, as a number to compare rather than a pointer to use.
template <class T>
inline Address address_of(const T* pointer) {
  return reinterpret_cast<Address>(pointer);
}

// Takes its own copy. The caller's vector is untouched and a second buffer
// exists for as long as this call does.
inline Address passed_by_value(std::vector<double> samples) {
  return address_of(samples.data());
}

// Borrows the caller's. No buffer is allocated, nothing is copied, and the
// callee cannot modify it.
inline Address passed_by_const_reference(const std::vector<double>& samples) {
  return address_of(samples.data());
}

// Borrows the caller's, and does not care what kind of container it came from.
//
// This is the signature almost every function that reads a buffer should have.
// A const reference to std::vector demands that the caller own a std::vector; a
// span accepts a vector, a plain array, or a pointer and a length from a device
// driver, and copies none of them.
// TODO 1: report where the data a span refers to actually lives.
//
// One line, and the interesting part is the signature above rather than the
// body. A span is a pointer and a length: it owns nothing, so there is nothing
// for it to copy, and data() gives you the caller's buffer.
//
// The test calls this with a std::vector, a std::array and a plain C array,
// and expects the caller's own address back from all three.
inline Address passed_as_span(rc::span<const double> samples) {
  (void)samples;
  return 0;
}

// Both addresses along a chain of two calls that each take their own copy.
struct ChainAddresses {
  Address first = 0;
  Address second = 0;
};

// TODO 2: pass it on, and report both addresses.
//
// Record where the buffer this function received lives, then hand it to
// passed_by_value and record where that one's lives. Two addresses from two
// calls.
//
// The test counts how many of them differ from the caller's original, and
// expects two: the cost of passing by value is per call, not once.
inline ChainAddresses passed_by_value_twice(std::vector<double> samples) {
  (void)samples;
  return ChainAddresses{};
}

inline ChainAddresses passed_by_reference_twice(const std::vector<double>& samples) {
  ChainAddresses seen;
  seen.first = address_of(samples.data());
  seen.second = passed_by_const_reference(samples);
  return seen;
}

// Building a vector and returning it, reporting where the local one lived.
//
// The instinct, coming from the measurements above, is that this must copy a
// whole buffer on the way out. It does not, and the suite shows the same
// address arriving at the caller. Whether the compiler elided the local
// entirely or moved it, the buffer was handed over rather than rebuilt.
// TODO 3: build a vector of count copies of 1.0, and return it.
//
// Before returning it, write where the local vector's buffer lives through the
// inside pointer, so the caller can compare.
//
// Return the local by name. Do not return a reference to it, which the checks
// directory holds as code that must not compile, and do not take a detour
// through std::move, which is unnecessary here and can prevent the compiler
// eliding the local altogether.
//
// Predict what the test will find before you run it. Most people expect a
// thousand doubles to be copied on the way out.
inline std::vector<double> built_and_returned(std::size_t count, Address* inside) {
  (void)count;
  (void)inside;
  return {};
}

// The same work, written the way people reach for when they believe returning
// is expensive.
inline void built_into_an_out_parameter(std::size_t count, std::vector<double>& out) {
  out.assign(count, 1.0);
}

// Does this string keep its characters inside the string object itself.
//
// Short strings do, which is why copying one allocates nothing at all. The
// length at which that stops is a choice your standard library made and not a
// rule of the language, so this reports it rather than assuming it.
// TODO 4: are this string's characters stored inside the string object.
//
// Take the address of the object itself and the address of its characters, and
// answer whether the second lies within the sizeof(std::string) bytes starting
// at the first.
//
// Both ends of that range matter. A short string's characters are somewhere
// inside the object; a long string's are on the heap, which may be at a lower
// address or a higher one, so a single comparison is not enough.
inline bool stored_inside_the_object(const std::string& text) {
  (void)text;
  return false;
}

#endif  // LESSON_SOLUTION_HPP
