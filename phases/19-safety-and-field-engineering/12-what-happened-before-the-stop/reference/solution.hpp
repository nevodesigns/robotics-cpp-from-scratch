#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

struct Event {
  std::uint64_t seq = 0;      // the number of this event since the log began: 0, 1, 2 ...
  double time = 0.0;          // the robot's own clock
  std::int32_t code = 0;
  double value = 0.0;
  std::uint64_t chain = 0;    // a hash of this event and the one before, so that a change anywhere shows
};

// The next link of the chain: FNV-1a over the bytes of the previous link, the sequence number, the time, the code and the value, in that order (each in the machine's own byte order).
inline std::uint64_t chain_next(std::uint64_t previous, std::uint64_t seq, double time, std::int32_t code, double value) {
  std::uint64_t h = 14695981039346656037ULL;
  auto mix = [&h](const void* p, std::size_t n) {
    const unsigned char* b = static_cast<const unsigned char*>(p);
    for (std::size_t i = 0; i < n; ++i) {
      h ^= b[i];
      h *= 1099511628211ULL;
    }
  };
  mix(&previous, sizeof previous);
  mix(&seq, sizeof seq);
  mix(&time, sizeof time);
  mix(&code, sizeof code);
  mix(&value, sizeof value);
  return h;
}

// What is read out of the box: the events still held, oldest first, the chain value of the last event that was overwritten (the starting point to check the rest from), and how many have been overwritten.
struct Dump {
  std::uint64_t base_chain = 0;
  std::uint64_t dropped = 0;
  std::vector<Event> events;
};

// A black box: a ring of fixed size. Recording never allocates and never fails, and when the ring is full the oldest event is overwritten.
class BlackBox {
 public:
  explicit BlackBox(std::size_t capacity) : events_(capacity) {}

  void record(double time, int code, double value) noexcept {
    const std::size_t cap = events_.size();
    if (cap == 0) return;
    Event e;
    e.seq = total_;
    e.time = time;
    e.code = code;
    e.value = value;
    e.chain = chain_next(last_chain_, e.seq, time, code, value);
    if (count_ == cap) {
      base_chain_ = events_[head_].chain;
      events_[head_] = e;
      head_ = (head_ + 1) % cap;
    } else {
      events_[(head_ + count_) % cap] = e;
      ++count_;
    }
    last_chain_ = e.chain;
    ++total_;
  }

  std::size_t size() const { return count_; }
  std::size_t capacity() const { return events_.size(); }
  std::uint64_t total() const { return total_; }

  Dump dump() const {
    Dump d;
    d.base_chain = base_chain_;
    d.dropped = total_ - count_;
    const std::size_t cap = events_.size();
    for (std::size_t i = 0; i < count_; ++i) d.events.push_back(events_[(head_ + i) % cap]);
    return d;
  }

 private:
  std::vector<Event> events_;
  std::size_t head_ = 0;
  std::size_t count_ = 0;
  std::uint64_t total_ = 0;
  std::uint64_t last_chain_ = 0;
  std::uint64_t base_chain_ = 0;
};

enum class Fault { none, gap, altered };

struct Verified {
  bool ok = true;
  std::size_t index = 0;          // of the first bad event, when not ok
  Fault fault = Fault::none;
  int time_regressions = 0;       // how many events have a time earlier than the one before: not an error, but worth knowing
};

// Check a dump. The events must run on in sequence from `dropped` (a missing or reordered one is a gap, reported at the first event that is out of place), and each event's chain must follow from the one before it,
// starting from base_chain (a changed event is altered). The gap is looked for first: a deleted event breaks both, and the better answer is that something is missing.
inline Verified verify(const Dump& d) {
  Verified v;
  std::uint64_t previous = d.base_chain;
  std::uint64_t expected = d.dropped;
  for (std::size_t i = 0; i < d.events.size(); ++i) {
    const Event& e = d.events[i];
    if (i > 0 && e.time < d.events[i - 1].time) ++v.time_regressions;
    if (v.ok) {
      if (e.seq != expected) {
        v.ok = false;
        v.index = i;
        v.fault = Fault::gap;
      } else if (e.chain != chain_next(previous, e.seq, e.time, e.code, e.value)) {
        v.ok = false;
        v.index = i;
        v.fault = Fault::altered;
      }
    }
    previous = e.chain;
    ++expected;
  }
  return v;
}

// Up to n events held immediately before the first event with the given code, oldest first: what the robot was doing on the way to a stop. Empty if there is no such event.
inline std::vector<Event> window_before(const Dump& d, int code, std::size_t n) {
  std::vector<Event> out;
  for (std::size_t i = 0; i < d.events.size(); ++i) {
    if (d.events[i].code != code) continue;
    const std::size_t from = i > n ? i - n : 0;
    for (std::size_t k = from; k < i; ++k) out.push_back(d.events[k]);
    break;
  }
  return out;
}

#endif  // LESSON_SOLUTION_HPP
