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
  // TODO: FNV-1a 64 bit: h starts at 14695981039346656037; for each byte b of previous, seq, time, code and value in that order (the bytes of each as it is in memory): h ^= b; h *= 1099511628211. Return h
  (void)previous;
  (void)seq;
  (void)time;
  (void)code;
  (void)value;
  return 0;
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
    // TODO: Nothing if the capacity is 0. Make an Event: seq = total_, the time, code and value, chain = chain_next(last_chain_, ...).
    // If the ring is full (count_ == capacity): base_chain_ = the chain of the oldest event, which sits at head_; overwrite it with the new one and move head_ on by one (wrapping). Otherwise store it at (head_ + count_) % capacity and count_++.
    // Then last_chain_ = its chain and total_++
    (void)time;
    (void)code;
    (void)value;
    (void)head_;
    (void)count_;
    (void)total_;
    (void)last_chain_;
    (void)base_chain_;
  }

  std::size_t size() const { return count_; }
  std::size_t capacity() const { return events_.size(); }
  std::uint64_t total() const { return total_; }

  Dump dump() const {
    // TODO: base_chain from base_chain_, dropped = total_ - count_, the events from the oldest (at head_) to the newest, wrapping at the capacity
    (void)head_;
    (void)count_;
    (void)total_;
    (void)base_chain_;
    return Dump();
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
  // TODO: previous = d.base_chain, expected = d.dropped. For each event i: count a time regression if its time is below the one before. Until a fault is found: if seq != expected it is a gap at i; else if chain != chain_next(previous, seq, time, code, value) it is altered at i.
  // Then previous = the event's own chain (as stored) and expected++. Keep counting the regressions to the end
  (void)d;
  return Verified();
}

// Up to n events held immediately before the first event with the given code, oldest first: what the robot was doing on the way to a stop. Empty if there is no such event.
inline std::vector<Event> window_before(const Dump& d, int code, std::size_t n) {
  // TODO: find the first event with this code; return the up to n events just before it, oldest first (fewer if it is near the start); empty if there is none
  (void)d;
  (void)code;
  (void)n;
  return std::vector<Event>();
}

#endif  // LESSON_SOLUTION_HPP
