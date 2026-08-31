// rc/rt/spsc_queue.hpp
//
// The queue from lesson 07-02, graduated.
//
// One producer thread, one consumer thread, no lock. That restriction is the
// design rather than a compromise: each index is written by exactly one thread,
// so no index is ever contended and almost all of the difficulty of lock free
// programming disappears. It is also the shape robotics needs, one sensor
// thread producing and one control thread consuming.
//
// What lock free actually promises, measured in that lesson rather than
// assumed: no thread is blocked because another thread holding a lock was
// descheduled. Against a mutex queue the median was about three times better
// and the 99th percentile about nine, and the worst case was not reliably
// better, because the worst case belongs to the scheduler and no data structure
// can take it back.
//
// Templated where the lesson was concrete, for the same reason as rc::rt::Latest.

#ifndef RC_RT_SPSC_QUEUE_HPP
#define RC_RT_SPSC_QUEUE_HPP

#include <atomic>
#include <cstddef>
#include <vector>

namespace rc {
namespace rt {

template <class T>
class SpscQueue {
 public:
  explicit SpscQueue(std::size_t capacity)
      : capacity_(capacity == 0 ? 1 : capacity),
        // One more slot than the capacity. Keeping one empty is what makes
        // head == tail mean empty unambiguously, and it costs less than a
        // separate count, which would be another shared variable and therefore
        // another ordering problem.
        buffer_(capacity_ + 1) {}

  // Returns false when full rather than blocking or growing. A deadline path
  // needs to know it dropped a sample, and growing would allocate in the loop.
  bool push(const T& value) {
    // The producer owns tail_, so nobody else writes it and a relaxed read is
    // safe.
    const std::size_t tail = tail_.load(std::memory_order_relaxed);
    const std::size_t next = advance(tail);

    // head_ belongs to the consumer, so acquire, to see its most recent release.
    if (next == head_.load(std::memory_order_acquire)) return false;

    buffer_[tail] = value;

    // Release publishes the write above. With relaxed here the index would
    // still be atomic and the value it points at might not be visible yet,
    // which is the torn read of lesson 07-01 arriving by a subtler route.
    tail_.store(next, std::memory_order_release);
    return true;
  }

  bool pop(T& out) {
    const std::size_t head = head_.load(std::memory_order_relaxed);

    // Acquire pairs with the producer's release and makes its write visible.
    if (head == tail_.load(std::memory_order_acquire)) return false;

    out = buffer_[head];
    head_.store(advance(head), std::memory_order_release);
    return true;
  }

  // Both indices move under other threads, so this is a reading taken at a
  // moment rather than a fact that stays true. Useful for reporting how full
  // the queue is running; wrong as a basis for deciding whether push or pop
  // will succeed, which is what their return values are for.
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
  std::vector<T> buffer_;
  std::atomic<std::size_t> head_{0};
  std::atomic<std::size_t> tail_{0};
};

}  // namespace rt
}  // namespace rc

#endif  // RC_RT_SPSC_QUEUE_HPP
