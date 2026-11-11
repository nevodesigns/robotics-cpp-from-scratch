#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cstddef>
#include <deque>
#include <map>
#include <string>
#include <vector>

// What to do with a message that arrives when a subscriber's queue is full.
enum class Overflow {
  drop_oldest,   // keep the newest: right for a position the controller wants now
  drop_newest,   // keep what is already waiting: right for a log that must not have holes at the start
};

// A bus where every subscriber has its own queue, and takes messages when it is ready.
class QueuedBus {
 public:
  using Handle = int;

  // A queue of at most `depth` messages (at least one) for this topic.
  Handle subscribe(const std::string& topic, std::size_t depth, Overflow overflow) {
    // TODO: add a Queue to queues_ with this topic, a depth of at least 1, and this overflow rule; the handle is its index (the size of queues_ before adding)
    (void)topic;
    (void)depth;
    (void)overflow;
    return 0;
  }

  // Put a copy in the queue of every subscriber of the topic. A full queue loses a message by its own rule and counts it.
  void publish(const std::string& topic, const std::string& payload) {
    // TODO: for each Queue q of this topic: if q.items.size() >= q.depth the queue is full: ++q.lost, and for drop_newest skip this queue (the new message is the one lost);
    // for drop_oldest pop_front() the oldest. Then push_back the payload.
    (void)topic;
    (void)payload;
  }

  // The oldest waiting message for this subscriber. False if there is none, or no such subscriber.
  bool take(Handle handle, std::string& out) {
    // TODO: false for a handle that does not exist or an empty queue. Otherwise out = the front item, pop_front, ++taken, and return true.
    (void)handle;
    (void)out;
    return false;
  }

  std::size_t pending(Handle handle) const {
    if (handle < 0 || handle >= static_cast<Handle>(queues_.size())) return 0;
    return queues_[static_cast<std::size_t>(handle)].items.size();
  }

  int lost(Handle handle) const {
    if (handle < 0 || handle >= static_cast<Handle>(queues_.size())) return 0;
    return queues_[static_cast<std::size_t>(handle)].lost;
  }

  int taken(Handle handle) const {
    if (handle < 0 || handle >= static_cast<Handle>(queues_.size())) return 0;
    return queues_[static_cast<std::size_t>(handle)].taken;
  }

 private:
  struct Queue {
    std::string topic;
    std::size_t depth = 1;
    Overflow overflow = Overflow::drop_oldest;
    std::deque<std::string> items;
    int lost = 0;
    int taken = 0;
  };
  std::vector<Queue> queues_;
};

#endif  // LESSON_SOLUTION_HPP
