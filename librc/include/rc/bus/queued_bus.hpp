// rc/bus/queued_bus.hpp
//
// A bus with a queue for each subscriber, from lesson 17-02, graduated.
//
// Every subscriber has its own bounded queue and takes messages when it is ready. A full queue loses a message by its own rule, counted: keep the newest (drop the oldest) for a position the controller
// wants now, or keep what is waiting (drop the newest) for a log that must not have holes at the start. Depth 3 with five messages took 3 4 5 and lost 2 under the first rule, took 1 2 3 and lost 2 under the second.
//
// A slow subscriber loses messages and the fast one beside it does not (fast lost 0, slow lost 8). Depth absorbs a burst of 8: 7, 4, 0 and 0 lost at depths 1, 4, 8 and 10. It does little for a subscriber that
// is slower all the time: at 100 Hz against 10 Hz over 10 s the loss was 900, 891 and 801 of 1000 at depths 1, 10 and 100.

#ifndef RC_BUS_QUEUED_BUS_HPP
#define RC_BUS_QUEUED_BUS_HPP

#include <cstddef>
#include <deque>
#include <map>
#include <string>
#include <vector>

namespace rc {
namespace bus {

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
    const Handle h = static_cast<Handle>(queues_.size());
    Queue q;
    q.topic = topic;
    q.depth = depth < 1 ? 1 : depth;
    q.overflow = overflow;
    queues_.push_back(q);
    return h;
  }

  // Put a copy in the queue of every subscriber of the topic. A full queue loses a message by its own rule and counts it.
  void publish(const std::string& topic, const std::string& payload) {
    for (Queue& q : queues_) {
      if (q.topic != topic) continue;
      if (q.items.size() >= q.depth) {
        ++q.lost;
        if (q.overflow == Overflow::drop_newest) continue;   // the new one is the one that goes
        q.items.pop_front();                                  // the oldest goes, and the new one takes its place
      }
      q.items.push_back(payload);
    }
  }

  // The oldest waiting message for this subscriber. False if there is none, or no such subscriber.
  bool take(Handle handle, std::string& out) {
    if (handle < 0 || handle >= static_cast<Handle>(queues_.size())) return false;
    Queue& q = queues_[static_cast<std::size_t>(handle)];
    if (q.items.empty()) return false;
    out = q.items.front();
    q.items.pop_front();
    ++q.taken;
    return true;
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

}  // namespace bus
}  // namespace rc

#endif  // RC_BUS_QUEUED_BUS_HPP
