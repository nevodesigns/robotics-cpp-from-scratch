// rc/bus/topic_bus.hpp
//
// A publish and subscribe bus by topic name, from lesson 17-01, graduated.
//
// A publisher gives a topic and a payload and every subscriber to that name is called, in the order they subscribed. A topic with nobody listening drops the message and counts it, without error,
// and a late subscriber does not see what came before. Unsubscribing from inside a handler is safe: the list is copied when a message goes out and a subscriber removed before its turn is skipped
// (first message: a and c called, second: c only). A publish made from inside a handler is queued and returns 0, so every subscriber of the first message is called before the second starts,
// and a ping that publishes a pong that publishes a ping ran 5000 times without nesting.

#ifndef RC_BUS_TOPIC_BUS_HPP
#define RC_BUS_TOPIC_BUS_HPP

#include <cstddef>
#include <deque>
#include <functional>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace rc {
namespace bus {

// A bus where code that makes a value and code that wants it never meet. A publisher says a topic name and a payload; every subscriber to that
// name is called with it. Neither knows the other exists.
class TopicBus {
 public:
  using Handler = std::function<void(const std::string& topic, const std::string& payload)>;
  using Token = int;

  // Call `handler` for every message published to `topic` from now on. The token is how to stop.
  Token subscribe(const std::string& topic, Handler handler) {
    const Token token = next_token_++;
    subscribers_[topic].push_back(Entry{token, std::move(handler)});
    return token;
  }

  // Stop calling that subscriber. True if it was subscribed. Safe to call from inside a handler, including a handler's own.
  bool unsubscribe(Token token) {
    for (auto& entry : subscribers_) {
      std::vector<Entry>& list = entry.second;
      for (std::size_t i = 0; i < list.size(); ++i)
        if (list[i].token == token) {
          list.erase(list.begin() + static_cast<std::ptrdiff_t>(i));
          return true;
        }
    }
    return false;
  }

  // Deliver a message to every subscriber of `topic`, in the order they subscribed, and return how many were called.
  // A message published from inside a handler is not delivered at once: it waits its turn, and this call returns 0 for it.
  // A subscriber removed by an earlier handler of the same message is not called.
  // A message nobody subscribed to is counted as dropped, and is not an error.
  int publish(const std::string& topic, const std::string& payload) {
    pending_.push_back(Message{topic, payload});
    if (delivering_) return 0;

    delivering_ = true;
    int first_delivered = 0;
    bool first = true;
    while (!pending_.empty()) {
      const Message message = pending_.front();
      pending_.pop_front();
      const int delivered = deliver(message);
      if (first) {
        first_delivered = delivered;
        first = false;
      }
    }
    delivering_ = false;
    return first_delivered;
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
    const auto found = subscribers_.find(message.topic);
    if (found == subscribers_.end() || found->second.empty()) {
      ++dropped_;
      return 0;
    }
    // The list as it was when the message went out. A handler may subscribe or unsubscribe, which changes the real list under us.
    std::vector<Token> tokens;
    for (const Entry& e : found->second) tokens.push_back(e.token);

    int delivered = 0;
    for (Token token : tokens) {
      if (!still_subscribed(message.topic, token)) continue;
      Handler handler;
      for (const Entry& e : subscribers_[message.topic])
        if (e.token == token) handler = e.handler;
      handler(message.topic, message.payload);
      ++delivered;
    }
    return delivered;
  }

  std::map<std::string, std::vector<Entry>> subscribers_;
  std::deque<Message> pending_;
  bool delivering_ = false;
  Token next_token_ = 1;
  int dropped_ = 0;
};

}  // namespace bus
}  // namespace rc

#endif  // RC_BUS_TOPIC_BUS_HPP
