#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <map>
#include <string>
#include <vector>

// What a poll found out.
struct DiscoveryEvent {
  enum class Kind { appeared, restarted, gone };
  Kind kind = Kind::appeared;
  std::string name;
};

// Who is on the bus, as far as anyone has said. A participant announces itself, again and again; each announcement keeps it alive for the
// lease. One that has not been heard from for longer than that is gone. Time is passed in, in seconds, so a test controls it.
class Discovery {
 public:
  explicit Discovery(double lease_seconds) : lease_(lease_seconds) {}

  // `name` is here, at time `now`. `incarnation` changes each time the participant is started, so a restart can be told from a heartbeat.
  void announce(const std::string& name, int incarnation, double now) {
    Entry& e = entries_[name];
    if (!e.known) {
      e.known = true;
      e.alive = true;
      e.incarnation = incarnation;
      e.last_heard = now;
      queue(DiscoveryEvent::Kind::appeared, name);
      return;
    }
    if (!e.alive) {
      e.alive = true;
      e.incarnation = incarnation;
      queue(DiscoveryEvent::Kind::appeared, name);
    } else if (incarnation != e.incarnation) {
      e.incarnation = incarnation;
      queue(DiscoveryEvent::Kind::restarted, name);
    }
    e.last_heard = now;
  }

  // Is `name` alive at `now`: heard from within the lease.
  bool alive(const std::string& name, double now) const {
    const auto found = entries_.find(name);
    if (found == entries_.end() || !found->second.known) return false;
    return found->second.alive && now - found->second.last_heard <= lease_;
  }

  // Everyone alive at `now`, by name.
  std::vector<std::string> peers(double now) const {
    std::vector<std::string> out;
    for (const auto& item : entries_)
      if (alive(item.first, now)) out.push_back(item.first);
    return out;
  }

  // Look at the table at time `now`, declare those whose lease has run out gone, and return every event since the last poll, in order.
  std::vector<DiscoveryEvent> poll(double now) {
    for (auto& item : entries_) {
      Entry& e = item.second;
      if (e.known && e.alive && now - e.last_heard > lease_) {
        e.alive = false;
        queue(DiscoveryEvent::Kind::gone, item.first);
      }
    }
    std::vector<DiscoveryEvent> out;
    out.swap(events_);
    return out;
  }

  double lease() const { return lease_; }

 private:
  struct Entry {
    bool known = false;
    bool alive = false;
    int incarnation = 0;
    double last_heard = 0.0;
  };

  void queue(DiscoveryEvent::Kind kind, const std::string& name) {
    DiscoveryEvent e;
    e.kind = kind;
    e.name = name;
    events_.push_back(e);
  }

  double lease_ = 1.0;
  std::map<std::string, Entry> entries_;
  std::vector<DiscoveryEvent> events_;
};

#endif  // LESSON_SOLUTION_HPP
