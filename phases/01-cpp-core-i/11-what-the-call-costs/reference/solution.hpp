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

// TODO signatures are implemented below. Each one answers the same question
// about a different way of writing the parameter.

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
inline Address passed_as_span(rc::span<const double> samples) {
  return address_of(samples.data());
}

// Both addresses along a chain of two calls that each take their own copy.
struct ChainAddresses {
  Address first = 0;
  Address second = 0;
};

inline ChainAddresses passed_by_value_twice(std::vector<double> samples) {
  ChainAddresses seen;
  seen.first = address_of(samples.data());
  seen.second = passed_by_value(samples);
  return seen;
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
inline std::vector<double> built_and_returned(std::size_t count, Address* inside) {
  std::vector<double> local(count, 1.0);
  *inside = address_of(local.data());
  return local;
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
inline bool stored_inside_the_object(const std::string& text) {
  const Address object = address_of(&text);
  const Address characters = address_of(text.data());
  return characters >= object && characters < object + sizeof(std::string);
}

#endif  // LESSON_SOLUTION_HPP
