#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <atomic>
#include <cstddef>
#include <vector>

struct Reading {
  double x = 0.0;
  double y = 0.0;
  long sequence = 0;
};

// A queue for exactly one producer thread and exactly one consumer thread.
//
// That restriction is the design rather than a compromise: each index is written
// by exactly one thread, so no index is ever contended and almost all of the
// difficulty of lock free programming disappears.
class SpscQueue {
 public:
  explicit SpscQueue(std::size_t capacity)
      : capacity_(capacity == 0 ? 1 : capacity),
        // One more slot than the capacity. Keeping one empty is what makes
        // head == tail mean empty unambiguously, and it costs less than a
        // separate count, which would be another shared variable and therefore
        // another ordering problem.
        buffer_(capacity_ + 1) {}

  bool push(const Reading& reading) {
    // The producer owns tail_, so nobody else writes it and reading it relaxed
    // is safe.
    const std::size_t tail = tail_.load(std::memory_order_relaxed);
    const std::size_t next = advance(tail);

    // head_ belongs to the consumer, so acquire, to see its most recent release.
    if (next == head_.load(std::memory_order_acquire)) return false;   // full

    buffer_[tail] = reading;

    // Release publishes the write above. Everything this thread did before this
    // store is visible to the consumer once it acquires this value. With relaxed
    // here the index would still be atomic and the reading it points at might
    // not be visible yet, which is the torn read of the last lesson arriving by
    // a subtler route.
    tail_.store(next, std::memory_order_release);
    return true;
  }

  bool pop(Reading& out) {
    const std::size_t head = head_.load(std::memory_order_relaxed);

    // Acquire pairs with the producer's release and makes its write visible.
    if (head == tail_.load(std::memory_order_acquire)) return false;   // empty

    out = buffer_[head];
    head_.store(advance(head), std::memory_order_release);
    return true;
  }

  std::size_t size() const {
    const std::size_t tail = tail_.load(std::memory_order_acquire);
    const std::size_t head = head_.load(std::memory_order_acquire);
    return (tail + buffer_.size() - head) % buffer_.size();
  }

  std::size_t capacity() const { return capacity_; }
  bool empty() const { return size() == 0; }

 private:
  // The modulus is against the storage size, which is one more than the
  // capacity. Using the capacity here is an off by one that shows up only after
  // the first wrap.
  std::size_t advance(std::size_t index) const { return (index + 1) % buffer_.size(); }

  std::size_t capacity_ = 1;
  std::vector<Reading> buffer_;
  std::atomic<std::size_t> head_{0};
  std::atomic<std::size_t> tail_{0};
};

#endif  // LESSON_SOLUTION_HPP
