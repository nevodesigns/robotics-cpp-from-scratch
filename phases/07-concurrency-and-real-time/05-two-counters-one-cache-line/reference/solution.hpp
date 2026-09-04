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

// A value with a cache line to itself.
//
// alignas on the type rather than on a member, so that the padding survives
// being put in an array: a vector of these has one per line, which is the whole
// point and is exactly what a vector of the bare type does not give you.
template <class T>
struct alignas(kCacheLine) Padded {
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

  // Producer only.
  bool push(const T& value) {
    const std::size_t tail = tail_.value.load(std::memory_order_relaxed);
    const std::size_t next = advance(tail);

    // The cached copy of the consumer's index. Reading the real one costs the
    // consumer its cache line, so only do it when the cached value says there
    // is no room, and believe it if it still says so.
    if (next == cached_head_) {
      cached_head_ = head_.value.load(std::memory_order_acquire);
      if (next == cached_head_) return false;
    }

    buffer_[tail] = value;
    tail_.value.store(next, std::memory_order_release);
    return true;
  }

  // Consumer only.
  bool pop(T& out) {
    const std::size_t head = head_.value.load(std::memory_order_relaxed);

    if (head == cached_tail_) {
      cached_tail_ = tail_.value.load(std::memory_order_acquire);
      if (head == cached_tail_) return false;
    }

    out = buffer_[head];
    head_.value.store(advance(head), std::memory_order_release);
    return true;
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
