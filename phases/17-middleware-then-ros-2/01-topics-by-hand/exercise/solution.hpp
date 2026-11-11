#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cstddef>
#include <deque>
#include <functional>
#include <map>
#include <string>
#include <utility>
#include <vector>

// A bus where code that makes a value and code that wants it never meet. A publisher says a topic name and a payload; every subscriber to that
// name is called with it. Neither knows the other exists.
class TopicBus {
 public:
  using Handler = std::function<void(const std::string& topic, const std::string& payload)>;
  using Token = int;

  // Call `handler` for every message published to `topic` from now on. The token is how to stop.
  Token subscribe(const std::string& topic, Handler handler) {
    // TODO: give the subscriber a new token (next_token_++), store Entry{token, handler} in subscribers_[topic] at the end, and return the token
    (void)topic;
    (void)handler;
    (void)next_token_;
    return 0;
  }

  // Stop calling that subscriber. True if it was subscribed. Safe to call from inside a handler, including a handler's own.
  bool unsubscribe(Token token) {
    // TODO: find the entry with this token in any topic's list and erase it; true if one was found, false otherwise
    (void)token;
    return false;
  }

  // Deliver a message to every subscriber of `topic`, in the order they subscribed, and return how many were called.
  // A message published from inside a handler is not delivered at once: it waits its turn, and this call returns 0 for it.
  // A subscriber removed by an earlier handler of the same message is not called.
  // A message nobody subscribed to is counted as dropped, and is not an error.
  int publish(const std::string& topic, const std::string& payload) {
    // TODO: push Message{topic, payload} onto pending_. If delivering_ is already true (we are inside a handler) return 0: it will be delivered later.
    // Otherwise set delivering_, then while pending_ is not empty take the front message and call deliver() on it; remember the count from the FIRST message.
    // Clear delivering_ and return that first count.
    (void)topic;
    (void)payload;
    (void)delivering_;
    return 0;
  }

  std::size_t subscriber_count(const std::string& topic) const {
    const auto found = subscribers_.find(topic);
    return found == subscribers_.end() ? 0 : found->second.size();
  }

  int dropped() const { return dropped_; }

 private:
  struct Entry {
    Token token = 0;
    Handler handler;
  };
  struct Message {
    std::string topic;
    std::string payload;
  };

  bool still_subscribed(const std::string& topic, Token token) const {
    const auto found = subscribers_.find(topic);
    if (found == subscribers_.end()) return false;
    for (const Entry& e : found->second)
      if (e.token == token) return true;
    return false;
  }

  int deliver(const Message& message) {
    // TODO: no subscribers (or an empty list): ++dropped_ and return 0.
    // Otherwise copy the tokens of the list as it is now. For each token, if still_subscribed() is false skip it; otherwise find that
    // entry's handler, call handler(topic, payload) and count one delivered. Return the count.
    (void)message;
    return 0;
  }

  std::map<std::string, std::vector<Entry>> subscribers_;
  std::deque<Message> pending_;
  bool delivering_ = false;
  Token next_token_ = 1;
  int dropped_ = 0;
};

#endif  // LESSON_SOLUTION_HPP
