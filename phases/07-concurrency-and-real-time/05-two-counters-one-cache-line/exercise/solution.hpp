#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <atomic>
#include <cstddef>
#include <vector>

// How far apart two variables have to be before two processors stop fighting
// over them.
//
// Caches do not move bytes, they move lines, and a line is 64 bytes on every
// processor this curriculum targets. Two threads writing to two different
// variables in the same line are, as far as the hardware is concerned, writing
// to the same thing: each write takes the line away from the other core.
//
// C++17 named this std::hardware_destructive_interference_size. The library on
// Ubuntu 22.04 does not provide it, which is a fair summary of how portable that
// answer is, so this is a constant with a measurement behind it: the test in
// this lesson sweeps the distance and reports where the interference stops.
constexpr std::size_t kCacheLine = 64;

// TODO 1: a value with a cache line to itself.
//
// Put alignas(kCacheLine) on the struct rather than on the member, so that the
// padding survives being put in an array. A vector of a type aligned this way
// has one element per line, which is the whole point and is exactly what a
// vector of the bare type does not give you.
//
// Give it operator* and operator-> so it still reads like the thing it holds.
template <class T>
struct Padded {
  T value{};

  T& operator*() { return value; }
  const T& operator*() const { return value; }
  T* operator->() { return &value; }
  const T* operator->() const { return &value; }
};

// The queue from lesson 07-02, with the two things this lesson is about.
//
// Correctness is unchanged and was already right. What changes is that the
// producer and the consumer stop taking each other's cache line away, and
// measured on one machine that is worth a little over twice the throughput.
//
// Two changes, and the measurement is emphatic that they are worth far more
// together than apart. Over four runs of four million items through a 1024 slot
// queue, padding alone was worth between 1 and 6 percent and caching alone
// between minus 8 and plus 14, both inside the noise. The two together were
// worth 76, 77, 83 and 95.
//
// Padding without caching still reads the other side's line on every single
// operation. Caching without padding still shares a line, so the cached copy is
// invalidated about as often as the real one would have been read. Each change
// removes one of two reasons the line moves, and the line still moves.
template <class T>
class FastQueue {
 public:
  explicit FastQueue(std::size_t capacity)
      : capacity_(capacity == 0 ? 1 : capacity), buffer_(capacity_ + 1) {}

  // TODO 2: push, reading the consumer's index as rarely as possible.
  //
  // Load tail_ relaxed, since the producer owns it. Work out the next index.
  //
  // Then, instead of loading head_ every time, compare against cached_head_.
  // Only when the cached value says there is no room, load the real head_ with
  // acquire and try again; if it still says no room, return false.
  //
  // Write the value, then store the new tail_ with release, so the consumer
  // cannot see the index move before the value it points at.
  bool push(const T& value) {
    (void)value;
    return false;
  }

  // TODO 3: pop, the mirror image.
  //
  // Load head_ relaxed, compare against cached_tail_, refresh cached_tail_ from
  // tail_ with acquire only when it says the queue is empty, and believe it if
  // it still does.
  //
  // Read the value, then store the advanced head_ with release.
  bool pop(T& out) {
    (void)out;
    return false;
  }

  std::size_t capacity() const { return capacity_; }

 private:
  std::size_t advance(std::size_t index) const { return (index + 1) % buffer_.size(); }

  std::size_t capacity_ = 1;
  std::vector<T> buffer_;

  // Each index on its own line, with the cache of the opposite index beside the
  // index its owner writes: the producer touches tail_ and cached_head_, the
  // consumer touches head_ and cached_tail_, and the two sets never meet.
  Padded<std::atomic<std::size_t>> head_;
  std::size_t cached_tail_ = 0;   // consumer only
  Padded<std::atomic<std::size_t>> tail_;
  std::size_t cached_head_ = 0;   // producer only
};

#endif  // LESSON_SOLUTION_HPP
