// rc/core/ring.hpp
//
// The fixed capacity history buffer from lesson 03-07, graduated.
//
// A robot keeps the last N of things. The last twenty encoder readings to
// smooth, the last hundred poses to draw a trail, the last fifty loop times to
// find the slow one. Always the last N, never all of them, because a machine
// that runs for a week cannot keep a week of samples.
//
// Two template parameters of different kinds. T is a type parameter. Capacity
// is a non type parameter, a number known at compile time, and that is what
// lets the storage be a plain array inside the object. Measured: 1000 pushes
// into a Ring<double, 64> performed 0 allocations, and
// sizeof(Ring<double, 64>) is 528 bytes of which 512 are the doubles. That is
// the reason to reach for this rather than a std::vector in a control loop.
//
// Each instantiation is a separate class. sizeof(Ring<double, 4>) is 48 and
// sizeof(Ring<double, 8>) is 80, and Ring<double, 4> and Ring<int, 4> share a
// definition and nothing else.
//
// A note on the static_asserts, because they are the part worth copying.
//
// A template has no declared interface. It is checked against whatever its
// body happens to use, so an unsuitable type fails wherever the body first
// touches something that type lacks, which may be several layers down inside
// somebody else's header. Measured, calling std::sort on a vector of a type
// with no operator<:
//
//                              gcc                      clang
//   no constraint              113 lines, 12884 chars   224 lines, 24069
//   constrained with a trait     5 lines,   509 chars   254 lines, 25648
//
// and the unconstrained error names /usr/include/c++/11/bits/predefined_ops.h,
// 21 instantiation frames deep in a file the caller has never opened.
//
// Read that table carefully, because the obvious conclusion is half wrong.
// gcc stops at the failed assertion and the diagnostic collapses. clang
// reports the assertion and then instantiates the rest anyway, so the volume
// goes up rather than down. What a static_assert reliably fixes on both is the
// first error line, which becomes a sentence you wrote, in your own file. That
// is what a reader acts on, and it is why the message should say what to pass
// instead rather than what went wrong.

#ifndef RC_CORE_RING_HPP
#define RC_CORE_RING_HPP

#include <cstddef>
#include <type_traits>

namespace rc {
namespace core {

// A fixed capacity buffer that forgets the oldest element when it is full.
template <class T, std::size_t Capacity>
class Ring {
  // A ring with no room is not a smaller ring. Every operation would be an
  // error and the modular arithmetic below would divide by zero.
  static_assert(Capacity > 0,
                "a Ring needs room for at least one element. Ring<double, 0> has "
                "no coherent behaviour, so pick a capacity.");

 public:
  static constexpr std::size_t capacity() { return Capacity; }

  std::size_t size() const { return count_; }
  bool empty() const { return count_ == 0; }
  bool full() const { return count_ == Capacity; }

  void clear() {
    first_ = 0;
    count_ = 0;
  }

  // Where the element at this logical position actually sits.
  //
  // Positions count from the oldest, and the oldest moves as the buffer is
  // reused, so a position and an array slot are not the same number.
  std::size_t slot(std::size_t index) const { return (first_ + index) % Capacity; }

  // Add a reading, forgetting the oldest if there is no room.
  //
  // Overwriting rather than refusing is what makes this a history. A sensor
  // does not stop producing readings because the buffer is full, and the
  // newest reading is the one you least want to lose.
  void push(const T& value) {
    if (count_ < Capacity) {
      storage_[slot(count_)] = value;
      ++count_;
      return;
    }
    storage_[first_] = value;
    first_ = (first_ + 1) % Capacity;
  }

  // Position 0 is the oldest element still held, not the first ever pushed.
  // Counting from the oldest is what makes a moving window read in order.
  //
  // Precondition: index < size().
  const T& operator[](std::size_t index) const { return storage_[slot(index)]; }

  // Precondition for both: the ring is not empty.
  const T& oldest() const { return storage_[first_]; }
  const T& newest() const { return storage_[slot(count_ - 1)]; }

 private:
  T storage_[Capacity] = {};
  std::size_t first_ = 0;
  std::size_t count_ = 0;
};

// The mean of what a ring is holding, for any element type and any capacity.
//
// Returns zero for an empty ring rather than dividing by its size.
template <class T, std::size_t Capacity>
double average(const Ring<T, Capacity>& ring) {
  static_assert(std::is_arithmetic<T>::value,
                "average() needs numbers. Ring<double, N> or Ring<int, N> work. "
                "For your own type, average the member you mean.");
  if (ring.empty()) return 0.0;
  double total = 0.0;
  for (std::size_t i = 0; i < ring.size(); ++i) total += static_cast<double>(ring[i]);
  return total / static_cast<double>(ring.size());
}

}  // namespace core
}  // namespace rc

#endif  // RC_CORE_RING_HPP
