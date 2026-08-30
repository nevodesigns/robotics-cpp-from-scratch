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
    // TODO
    //
    //   read tail_ relaxed, because the producer owns it
    //   work out the next index with advance()
    //   read head_ with acquire; if next lands on it the queue is full
    //   write the reading into buffer_[tail]
    //   store the new tail_ with release, which publishes that write
    //
    // Relaxed on that last store would leave the index atomic and the reading it
    // points at possibly not visible yet, which is a torn read by a subtler
    // route.
    (void)reading;
    return false;
  }

  bool pop(Reading& out) {
    // TODO: the mirror image. head_ relaxed because the consumer owns it,
    // tail_ with acquire to pair with the producer's release, and the new head_
    // stored with release.
    (void)out;
    return false;
  }

  std::size_t size() const {
    // TODO: how many readings are waiting. Mind the wrap: the answer is not
    // simply tail minus head once the indices have gone round.
    return 0;
  }

  std::size_t capacity() const { return capacity_; }
  bool empty() const { return size() == 0; }

 private:
  // The modulus is against the storage size, which is one more than the
  // capacity. Using the capacity here is an off by one that shows up only after
  // the first wrap.
  std::size_t advance(std::size_t index) const {
    // TODO: one step forward, wrapping at the end of the storage.
    //
    // The modulus is against the storage size, which is one more than the
    // capacity. Using the capacity here is an off by one that appears only
    // after the first wrap, which is exactly the kind that reaches a robot.
    return index;
  }

  std::size_t capacity_ = 1;
  std::vector<Reading> buffer_;
  std::atomic<std::size_t> head_{0};
  std::atomic<std::size_t> tail_{0};
};

#endif  // LESSON_SOLUTION_HPP
