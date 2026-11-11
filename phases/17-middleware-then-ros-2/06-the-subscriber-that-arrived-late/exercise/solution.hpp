#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cstddef>
#include <deque>
#include <functional>
#include <map>
#include <string>
#include <vector>

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
    // TODO: set the topic's durability and its keep (at least 1); if more than keep are already remembered, drop the oldest until it fits
    (void)topic;
    (void)durability;
    (void)keep;
  }

  // Listen to `topic`. A transient local topic first gives this subscriber what it remembers, oldest first, except any message
  // older than `max_age` seconds at time `now` (a negative `max_age` means no limit). Those skipped are counted.
  void subscribe(const std::string& topic, Handler handler, double now, double max_age) {
    // TODO: For a transient local topic, first call handler on each remembered message, oldest first, adding one to replayed_ each time.
    // Skip (and add one to stale_skipped_) any message with max_age >= 0 and now - stamp > max_age. Then add the handler to the topic's handlers.
    (void)topic;
    (void)handler;
    (void)now;
    (void)max_age;
    (void)replayed_;
    (void)stale_skipped_;
  }

  // Deliver to the subscribers there are, and remember it if the topic is transient local.
  void publish(const std::string& topic, const std::string& payload, double now) {
    // TODO: Make a Stamped with this payload and time. If the topic is transient local, add it to kept and drop the oldest while there are more than keep.
    // Then call every handler (a copy of the list, so a handler may subscribe) with it.
    (void)topic;
    (void)payload;
    (void)now;
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

#endif  // LESSON_SOLUTION_HPP
