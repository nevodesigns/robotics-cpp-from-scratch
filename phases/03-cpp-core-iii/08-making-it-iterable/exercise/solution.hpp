#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cstddef>
#include <iterator>

// Lesson 03-07 built a Ring, and you cannot write this:
//
//     for (double reading : ring) { ... }
//
// because the compiler looks for begin() and end() and does not find them.
// Nor can you pass it to std::accumulate, std::max_element or anything else in
// <algorithm>, which lesson 03-04 taught you to prefer over writing loops.
//
// An iterator is the fix, and the useful thing about writing one is what it
// teaches about what the language actually asks for. range-for wants three
// operations. The standard algorithms want more, and the way you declare which
// you provide is a set of typedefs that nobody can guess.
//
// This iterator is built on operator[] rather than on the container's storage,
// and that choice is the whole design. A Ring's elements are not in array
// order: a ring of four holding 3 4 5 6 keeps them in slots as 5 6 3 4.
// operator[] already does that arithmetic, so an iterator that goes through it
// is correct for free, and one that walks the raw array is wrong in a way that
// only appears after the buffer has wrapped.

// An iterator over anything with operator[] and size().
template <class Container>
class IndexIterator {
 public:
  // The five typedefs the standard library looks for. Nobody guesses these;
  // they are looked up, and leaving one out produces an error inside
  // <algorithm> rather than here, which lesson 03-07 measured the cost of.
  //
  // random_access_iterator_tag is a promise: that stepping n places costs the
  // same as stepping one. It is true here because the work is an index
  // addition, and declaring it is what lets std::sort and std::distance do
  // their arithmetic rather than walking.
  using iterator_category = std::random_access_iterator_tag;
  using value_type = typename Container::value_type;
  using difference_type = std::ptrdiff_t;
  using pointer = const value_type*;
  using reference = const value_type&;

  IndexIterator(const Container* container, std::size_t index)
      : container_(container), index_(index) {}

  // TODO 1: the element this iterator is currently on.
  //
  // Go through the container's operator[], not through its storage. That is
  // the design decision of the whole lesson: a Ring's elements are not in
  // array order, operator[] already does that arithmetic, and an iterator
  // built on it is correct for free.
  //
  // The test pushes 1 to 6 into a ring of 4 and expects to walk 3 4 5 6,
  // while the slots themselves hold 5 6 3 4.
  reference operator*() const { return (*container_)[0]; }

  // TODO 2: advance, and compare.
  //
  // Advancing means moving to the next index. The stub below already does it,
  // because an operator++ that does not advance makes every loop in the suite
  // run forever rather than fail, and a test that hangs tells you nothing.
  //
  // Comparing means asking whether two iterators are at the same index. The
  // stub answers false, which makes every range empty, so the tests fail
  // quickly instead of hanging while you work on it.
  IndexIterator& operator++() {
    ++index_;
    return *this;
  }
  bool operator!=(const IndexIterator& other) const {
    (void)other;
    return false;
  }

  // Given, because they are the same two ideas again in different clothes.
  IndexIterator operator++(int) {
    const IndexIterator before = *this;
    ++index_;
    return before;
  }
  IndexIterator& operator--() {
    --index_;
    return *this;
  }
  bool operator==(const IndexIterator& other) const { return index_ == other.index_; }

  // TODO 3: the random access operations.
  //
  // Declaring random_access_iterator_tag above is a promise that stepping n
  // places costs the same as stepping one. These are how you keep it, and the
  // library holds you to the whole set: a missing one produces an error from
  // inside a standard library header rather than from here.
  //
  // The subtraction of two iterators is a count, so it returns a
  // difference_type and both indices need casting to it before subtracting, or
  // an unsigned subtraction will wrap, which lesson 01-07 was about.
  //
  // The ordering comparison is what std::lower_bound and std::sort need.
  difference_type operator-(const IndexIterator& other) const {
    (void)other;
    return 0;
  }
  IndexIterator operator+(difference_type step) const {
    (void)step;
    return *this;
  }
  IndexIterator operator-(difference_type step) const {
    (void)step;
    return *this;
  }
  bool operator<(const IndexIterator& other) const {
    (void)other;
    return false;
  }

  // The compound forms, which std::advance uses. Declaring the random access
  // tag above is a promise, and the library holds you to the whole of it: the
  // error for a missing one of these arrives from inside
  // stl_iterator_base_funcs.h rather than from here, which is lesson 03-07's
  // finding about template diagnostics arriving all over again.
  IndexIterator& operator+=(difference_type step) {
    (void)step;
    return *this;
  }
  IndexIterator& operator-=(difference_type step) {
    (void)step;
    return *this;
  }
  reference operator[](difference_type step) const {
    (void)step;
    return (*container_)[0];
  }

 private:
  const Container* container_;
  std::size_t index_;
};

// The two that bracket a container. Free functions, so they work for a
// container whose definition you cannot change.
// TODO 4: the two iterators that bracket a container.
//
// begin is at the first element. end is one past the last, which for an index
// based iterator means the size itself: there is no element there and there
// does not need to be, because end is never dereferenced, only compared
// against.
//
// Both stubs point at index 0, which makes every range empty. That is
// deliberate: an empty range fails the tests immediately rather than looping.
template <class Container>
IndexIterator<Container> index_begin(const Container& container) {
  return IndexIterator<Container>(&container, 0);
}

template <class Container>
IndexIterator<Container> index_end(const Container& container) {
  return IndexIterator<Container>(&container, 0);
}

#endif  // LESSON_SOLUTION_HPP
