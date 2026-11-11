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
    // TODO: Look up (or create) the entry for this name.
    // - A name never heard before: it is known and alive, remember the incarnation and the time, queue an `appeared` event, and stop.
    // - Known but marked gone: it is alive again, remember the new incarnation, queue `appeared`.
    // - Alive, with a different incarnation from the one remembered: remember the new one and queue `restarted`.
    // In every case except the first, last_heard becomes `now`.
    (void)name;
    (void)incarnation;
    (void)now;
  }

  // Is `name` alive at `now`: heard from within the lease.
  bool alive(const std::string& name, double now) const {
    // TODO: false for a name not known; otherwise it is marked alive AND now - last_heard is no more than the lease
    (void)name;
    (void)now;
    return false;
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
    // TODO: For each entry that is known, marked alive and not heard from within the lease (now - last_heard > lease_): mark it not alive and queue a `gone` event.
    // Return the queued events and leave the queue empty.
    (void)now;
    (void)lease_;
    return std::vector<DiscoveryEvent>();
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
