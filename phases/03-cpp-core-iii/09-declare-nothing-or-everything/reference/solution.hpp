#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>
#include <vector>

#include <rc/test/leak_check.hpp>

// Lesson 02-05 had you write the five operations by hand, once, so that what
// a resource owning type has to get right is something you have done.
//
// This lesson is the other half of that: almost never do it again.
//
// The compiler writes all five for you, correctly, as long as every member
// knows how to look after itself. Declaring even one of them by hand changes
// what it writes for the others, and the change is silent.

using Address = std::uintptr_t;

template <class T>
inline Address address_of(const T* pointer) {
  return reinterpret_cast<Address>(pointer);
}

// Four classes holding exactly the same thing, differing only in what they
// say about the five operations. These are the subject of the lesson rather
// than something to implement.

// Declares nothing. The compiler writes all five.
struct Quiet {
  std::vector<double> samples;
};

// Declares a destructor that does nothing whatever, written as = default so
// it is as close to declaring nothing as the language allows.
struct WithDestructor {
  std::vector<double> samples;
  ~WithDestructor() = default;
};

// Declares a copy constructor, which somebody added to log a copy.
struct WithCopy {
  std::vector<double> samples;
  WithCopy() = default;
  WithCopy(const WithCopy& other) : samples(other.samples) {}
};

// Declares all five, every one of them = default. Back to where Quiet is, at
// the cost of six lines that must be kept correct.
struct AllFive {
  std::vector<double> samples;
  AllFive() = default;
  ~AllFive() = default;
  AllFive(const AllFive&) = default;
  AllFive& operator=(const AllFive&) = default;
  AllFive(AllFive&&) = default;
  AllFive& operator=(AllFive&&) = default;
};

// Will moving one of these actually move, or copy.
//
// Not std::is_move_constructible, which is the trait everybody reaches for and
// which answers yes for every class above. A copy constructor can construct
// from an rvalue, so "is move constructible" is satisfied by copying.
//
// The trait that tells the truth is the nothrow one, because that is also the
// condition the standard library uses: a std::vector reallocating will only
// move its elements if moving them cannot throw, since otherwise it could not
// put them back if one did.
template <class T>
constexpr bool moves_cheaply() {
  return std::is_nothrow_move_constructible<T>::value;
}

// Build one with a buffer in it, move it, and report whether the buffer went
// with it or was rebuilt.
//
// The address technique from lesson 01-11: if the destination sees the same
// buffer, nothing was copied.
template <class T>
inline bool buffer_travels_on_move() {
  T source;
  source.samples.assign(1024, 1.0);
  const Address before = address_of(source.samples.data());

  T destination = std::move(source);
  return address_of(destination.samples.data()) == before;
}

// How many allocations it takes to grow a vector of these to n elements.
//
// This is where the cost shows up in a program rather than in a trait. Every
// time the outer vector runs out of room it has to relocate its elements, and
// whether that is a move or a copy is decided by moves_cheaply above.
template <class T>
inline std::size_t allocations_to_grow(int n) {
  std::vector<T> rows;
  rc::test::AllocationCount counted;
  for (int i = 0; i < n; ++i) {
    T row;
    row.samples.assign(64, 1.0);
    rows.push_back(std::move(row));
  }
  return counted.total();
}

#endif  // LESSON_SOLUTION_HPP
