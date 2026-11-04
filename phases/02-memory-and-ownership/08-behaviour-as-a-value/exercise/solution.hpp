#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cstdint>
#include <functional>
#include <vector>

// A lambda is a value that is a piece of behaviour. You can store one, pass one
// and return one, which is why the standard algorithms take them and why Qt
// connects to them.
//
// You have been handed them already. Lesson 03-04 passes four of them to
// standard algorithms, and this lesson exists because nothing had explained
// what the square brackets at the front mean.
//
// They are the capture list, and they are an ownership decision, which is why
// this lesson is in the phase about ownership rather than the one about values.
// Capturing by value takes a copy and owns it. Capturing by reference borrows,
// and lesson 02-07 is about what borrowing costs when the thing you borrowed
// from goes away.

using Address = std::uintptr_t;

template <class T>
inline Address address_of(const T* pointer) {
  return reinterpret_cast<Address>(pointer);
}

// A test against a copy of the limit, taken when this is called.
//
// The limit inside the returned callable is this function's own copy. Whatever
// the caller does to their variable afterwards, this keeps answering against
// the value it was given.
// TODO 1: return a callable that tests a value against a copy of the limit.
//
// The capture list is the square brackets. Naming the variable in it, with no
// ampersand, captures it by value: the returned callable gets its own copy and
// owns it.
//
// The test creates one of these while the limit is 1, then changes the limit to
// 100, then asks whether 50 exceeds it. A by value capture says true, because
// it is still answering against the 1 it was given.
inline std::function<bool(double)> watching_a_copy(double limit) {
  (void)limit;
  return [](double) { return false; };
}

// A test against the caller's limit, read when the callable is used.
//
// Precondition: the object `limit` refers to must outlive the returned
// callable. This signature is how a threshold that somebody can turn is
// written, and it is also how the worst bug in this lesson is written. The
// difference is entirely in whether that precondition holds, and nothing in
// the type system or either compiler will check it for you.
// TODO 2: return a callable that tests against the caller's limit, read when
// the callable is used.
//
// An ampersand before the name captures by reference. Nothing is copied, and
// the value is read at every call rather than at the capture.
//
// The same test asks the same question of this one and expects false, because
// by the time it is called the limit is 100.
//
// Precondition: the object limit refers to must outlive the returned callable.
// Write that down wherever you write a signature like this, because neither
// the type system nor either compiler will check it, and returning a callable
// that captured a local by reference compiles cleanly on both.
inline std::function<bool(double)> watching_the_original(const double& limit) {
  (void)limit;
  return [](double) { return false; };
}

// Where the buffer that this callable reads actually lives, when the samples
// are captured by value.
//
// Capturing a container by value copies it, exactly as passing one by value
// does, and lesson 01-11 measured that. The capture list is a list of
// initialisations, so the same rules apply.
// TODO 3: return a callable that reports where the buffer it reads lives,
// having captured the samples by value.
//
// The capture list is a list of initialisations, so capturing a container by
// value copies it exactly as passing one by value does, which lesson 01-11
// measured. The test checks that the address differs from the caller's.
//
// Use address_of(samples.data()) inside the lambda.
inline std::function<Address()> address_of_a_copied_buffer(
    const std::vector<double>& samples) {
  (void)samples;
  return []() { return Address{0}; };
}

// The same, capturing by reference.
//
// Precondition: samples must outlive the returned callable.
// TODO 4: the same, capturing by reference instead.
//
// Nothing is copied, so the address must be the caller's own.
//
// Precondition: samples must outlive the returned callable.
inline std::function<Address()> address_of_a_borrowed_buffer(
    const std::vector<double>& samples) {
  (void)samples;
  return []() { return Address{0}; };
}

#endif  // LESSON_SOLUTION_HPP
