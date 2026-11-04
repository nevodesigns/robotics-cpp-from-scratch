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

  // The three range-for asks for. That is all it asks for: it expands to a
  // loop that dereferences, advances and compares.
  reference operator*() const { return (*container_)[index_]; }
  IndexIterator& operator++() {
    ++index_;
    return *this;
  }
  bool operator!=(const IndexIterator& other) const { return index_ != other.index_; }

  // And the rest, which the algorithms ask for.
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
  bool operator<(const IndexIterator& other) const { return index_ < other.index_; }

  // The random access operations. std::distance uses the subtraction instead
  // of counting steps, which is the difference between constant time and
  // walking the whole range.
  difference_type operator-(const IndexIterator& other) const {
    return static_cast<difference_type>(index_) - static_cast<difference_type>(other.index_);
  }
  IndexIterator operator+(difference_type step) const {
    return IndexIterator(container_, index_ + static_cast<std::size_t>(step));
  }
  IndexIterator operator-(difference_type step) const {
    return IndexIterator(container_, index_ - static_cast<std::size_t>(step));
  }

  // The compound forms, which std::advance uses. Declaring the random access
  // tag above is a promise, and the library holds you to the whole of it: the
  // error for a missing one of these arrives from inside
  // stl_iterator_base_funcs.h rather than from here, which is lesson 03-07's
  // finding about template diagnostics arriving all over again.
  IndexIterator& operator+=(difference_type step) {
    index_ += static_cast<std::size_t>(step);
    return *this;
  }
  IndexIterator& operator-=(difference_type step) {
    index_ -= static_cast<std::size_t>(step);
    return *this;
  }
  reference operator[](difference_type step) const {
    return (*container_)[index_ + static_cast<std::size_t>(step)];
  }

 private:
  const Container* container_;
  std::size_t index_;
};

// The two that bracket a container. Free functions, so they work for a
// container whose definition you cannot change.
template <class Container>
IndexIterator<Container> index_begin(const Container& container) {
  return IndexIterator<Container>(&container, 0);
}

template <class Container>
IndexIterator<Container> index_end(const Container& container) {
  return IndexIterator<Container>(&container, container.size());
}

#endif  // LESSON_SOLUTION_HPP
