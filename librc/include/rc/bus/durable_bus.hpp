// rc/bus/durable_bus.hpp
//
// A bus that remembers some topics, from lesson 17-06, graduated.
//
// A transient local topic keeps its last few messages and gives them, oldest first, to a subscriber that arrives afterwards, before what follows; a volatile one does not. What is kept is bounded by `keep`,
// and a subscriber that was there all along hears nothing twice. A subscriber can limit how old a replayed message may be.
//
// Keep 3 with five published and a sixth after a subscriber arrived: heard 3 4 5 6, replayed 3. 1000 messages to a topic that keeps 1 remembered 1; the late subscriber heard 999. At 100 s, remembered messages
// from 0 s and 98 s were heard as 2 with no limit and 1 with a limit of 5 s, with 1 skipped. A map published at 0 s was seen by 0 of 3 nodes starting at 1, 5 and 9 s when not remembered, and 3 of 3 when it was.

#ifndef RC_BUS_DURABLE_BUS_HPP
#define RC_BUS_DURABLE_BUS_HPP

#include <cstddef>
#include <deque>
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace rc {
namespace bus {

// Whether a topic remembers what it carried.
enum class Durability {
  volatile_,         // no: a subscriber hears what is published after it subscribes
  transient_local,   // yes: the last few messages are kept, and a new subscriber is given them first
};

struct Stamped {
  std::string payload;
  double stamp = 0.0;   // when it was published, in seconds
};

// A bus where some topics are remembered.
class DurableBus {
 public:
  using Handler = std::function<void(const Stamped&)>;

  // Declare a topic. For a transient local one, `keep` is how many of the latest messages are remembered (at least one).
  // A topic never declared is volatile.
  void advertise(const std::string& topic, Durability durability, std::size_t keep) {
    Topic& t = topics_[topic];
    t.durability = durability;
    t.keep = keep < 1 ? 1 : keep;
    while (t.kept.size() > t.keep) t.kept.pop_front();
  }

  // Listen to `topic`. A transient local topic first gives this subscriber what it remembers, oldest first, except any message
  // older than `max_age` seconds at time `now` (a negative `max_age` means no limit). Those skipped are counted.
  void subscribe(const std::string& topic, Handler handler, double now, double max_age) {
    Topic& t = topics_[topic];
    if (t.durability == Durability::transient_local) {
      for (const Stamped& m : t.kept) {
        if (max_age >= 0.0 && now - m.stamp > max_age) {
          ++stale_skipped_;
          continue;
        }
        handler(m);
        ++replayed_;
      }
    }
    t.handlers.push_back(handler);
  }

  // Deliver to the subscribers there are, and remember it if the topic is transient local.
  void publish(const std::string& topic, const std::string& payload, double now) {
    Topic& t = topics_[topic];
    Stamped m;
    m.payload = payload;
    m.stamp = now;
    if (t.durability == Durability::transient_local) {
      t.kept.push_back(m);
      while (t.kept.size() > t.keep) t.kept.pop_front();
    }
    const std::vector<Handler> handlers = t.handlers;
    for (const Handler& h : handlers) h(m);
  }

  std::size_t remembered(const std::string& topic) const {
    const auto found = topics_.find(topic);
    return found == topics_.end() ? 0 : found->second.kept.size();
  }
  int replayed() const { return replayed_; }
  int stale_skipped() const { return stale_skipped_; }

 private:
  struct Topic {
    Durability durability = Durability::volatile_;
    std::size_t keep = 1;
    std::deque<Stamped> kept;
    std::vector<Handler> handlers;
  };
  std::map<std::string, Topic> topics_;
  int replayed_ = 0;
  int stale_skipped_ = 0;
};

}  // namespace bus
}  // namespace rc

#endif  // RC_BUS_DURABLE_BUS_HPP
