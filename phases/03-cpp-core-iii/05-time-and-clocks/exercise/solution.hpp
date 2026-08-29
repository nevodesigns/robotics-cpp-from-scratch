#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <chrono>
#include <cstdint>

// Nanoseconds since some starting point that means nothing on its own. Only the
// difference between two of them is meaningful, which is the whole idea of a
// steady clock.
using Nanoseconds = std::int64_t;

class Clock {
 public:
  virtual ~Clock() = default;   // deleted through a base pointer, so this matters
  virtual Nanoseconds now() const = 0;
};

// The real one. steady_clock rather than system_clock, because system_clock can
// be set by an administrator or stepped by a time synchronisation daemon, and a
// controller that sees time move backwards does something sudden.
class SteadyClock : public Clock {
 public:
  Nanoseconds now() const override {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
  }
};

// The test one. Time is whatever the test says, so a suite can cover hours of
// behaviour in microseconds of real time and can arrange a leap on demand.
class TestClock : public Clock {
 public:
  explicit TestClock(Nanoseconds start = 0) : now_(start) {}

  Nanoseconds now() const override { return now_; }

  // TODO: move the clock forward by this many nanoseconds.
  void advance(Nanoseconds by) { (void)by; }

  // TODO: put the clock at exactly this reading, which is how a test arranges
  // a backwards step.
  void set(Nanoseconds to) { (void)to; }

 private:
  Nanoseconds now_ = 0;
};

// The one conversion at the edge, where a duration becomes an input to the
// control mathematics. Doing it here and nowhere else keeps the units in the
// type everywhere upstream.
inline double seconds_between(Nanoseconds from, Nanoseconds to) {
  // TODO: the interval in seconds, as a double. This is the one place a
  // duration becomes a plain number, so everything upstream keeps its units.
  (void)from;
  (void)to;
  return 0.0;
}

enum class TickQuality {
  Good,
  First,       // no previous reading to subtract from
  Backwards,   // zero or negative interval
  Stalled,     // far longer than expected, clamped
};

struct TickResult {
  double dt = 0.0;
  TickQuality quality = TickQuality::Good;

  bool usable() const { return quality == TickQuality::Good || quality == TickQuality::Stalled; }
};

// Turns a clock into the dt that every controller in this curriculum takes.
class LoopTimer {
 public:
  explicit LoopTimer(double max_dt) : max_dt_(max_dt) {}

  TickResult tick(const Clock& clock) {
    // TODO
    //
    //   the first tick has nothing to subtract from, so report First and 0.0
    //   an interval of zero or less reports Backwards and 0.0
    //   an interval above max_dt_ reports Stalled and a dt clamped to max_dt_
    //   anything else is Good
    //
    // Remember to record the new reading for next time in every case.
    (void)clock;
    return TickResult{};
  }

  void reset() { started_ = false; }

 private:
  double max_dt_ = 0.1;
  Nanoseconds last_ = 0;
  bool started_ = false;
};

#endif  // LESSON_SOLUTION_HPP
