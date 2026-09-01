// rc/core/clock.hpp
//
// The injectable clock from lesson 03-05, graduated.
//
// Time arrives as an argument rather than being read from a global, which is
// what lets a test make an hour pass instantly and a timeout be checked rather
// than hoped about. Every timed thing in this curriculum takes its time this
// way for that reason.

#ifndef RC_CORE_CLOCK
#define RC_CORE_CLOCK

#include <chrono>
#include <cstdint>

namespace rc {
namespace core {

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

  void advance(Nanoseconds by) { now_ += by; }
  void set(Nanoseconds to) { now_ = to; }

 private:
  Nanoseconds now_ = 0;
};

// The one conversion at the edge, where a duration becomes an input to the
// control mathematics. Doing it here and nowhere else keeps the units in the
// type everywhere upstream.
inline double seconds_between(Nanoseconds from, Nanoseconds to) {
  return std::chrono::duration<double>(std::chrono::nanoseconds(to - from)).count();
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
    const Nanoseconds now = clock.now();

    if (!started_) {
      started_ = true;
      last_ = now;
      // Nothing to subtract from, so no interval exists. Reporting zero rather
      // than the time since some arbitrary origin is what stops the first
      // update of a controller integrating a nonsense value.
      return TickResult{0.0, TickQuality::First};
    }

    const double dt = seconds_between(last_, now);
    last_ = now;

    // Zero or negative means no time passed or the clock moved. Neither can be
    // integrated or differentiated, so the caller is told to skip the update.
    if (dt <= 0.0) return TickResult{0.0, TickQuality::Backwards};

    // A stall integrated as though it were real produces one large correction,
    // which on hardware is a lurch. Clamping is the usual answer and it must be
    // a decision rather than an accident, which is why it is reported.
    if (dt > max_dt_) return TickResult{max_dt_, TickQuality::Stalled};

    return TickResult{dt, TickQuality::Good};
  }

  void reset() { started_ = false; }

 private:
  double max_dt_ = 0.1;
  Nanoseconds last_ = 0;
  bool started_ = false;
};

}  // namespace core
}  // namespace rc

#endif  // RC_CORE_CLOCK
