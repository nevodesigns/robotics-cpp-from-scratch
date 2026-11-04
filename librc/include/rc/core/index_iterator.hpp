// rc/core/index_iterator.hpp
//
// One iterator for any container that can be indexed, from lesson 03-08,
// graduated.
//
// A container with operator[] and size() and a value_type gains range-for and
// the whole of <algorithm> by adding two lines that return one of these.
//
// Built on operator[] rather than on the container's storage, and that is the
// design rather than a convenience. A Ring's elements are not in array order:
// measured on a ring of four holding 3 4 5 6, the slots hold 5 6 3 4. An
// iterator over the raw array would hand back the right four readings in the
// wrong sequence, and only once the buffer had wrapped. operator[] already
// does that arithmetic, so going through it is correct for free.
//
// Two things the lesson measured that are worth keeping.
//
// range-for asks for exactly three operations: dereference, pre-increment and
// inequality. That is the whole requirement, and it is smaller than people
// expect.
//
// The standard algorithms ask for more, and the way you declare what you
// provide is five typedefs that cannot be guessed. Declaring
// random_access_iterator_tag is a promise that stepping n places costs what
// stepping one costs, and the library holds you to the entire interface:
// std::advance needs operator+=, and leaving it out produces an error inside
// stl_iterator_base_funcs.h rather than at your own code, which is lesson
// 03-07's finding about template diagnostics all over again.
//
// The references handed out are const. A window of readings you have collected
// is not a thing to sort in place, and std::sort on one of these is refused,
// which lesson 03-08 keeps in checks/.

#ifndef RC_CORE_INDEX_ITERATOR_HPP
#define RC_CORE_INDEX_ITERATOR_HPP

#include <cstddef>
#include <iterator>

namespace rc {
namespace core {

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

}  // namespace core
}  // namespace rc

#endif  // RC_CORE_INDEX_ITERATOR_HPP
