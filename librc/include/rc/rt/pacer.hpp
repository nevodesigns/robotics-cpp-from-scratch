// rc/rt/pacer.hpp
//
// Holding a loop to a rate, from lesson 07-08, graduated.
//
// Lesson 07-03 measured how late a loop is. It did not say how to write one that
// is not. The loop everybody writes first is
//
//   while (running) { work(); sleep(period); }
//
// and it does not hold a rate. Each cycle takes the work PLUS the period.
//
// Measured on this curriculum's machine, gcc 11, 2 ms of work in a 10 ms period,
// real threads and a real clock, run three times with the same result to within
// a few milliseconds. A 100 cycle run, wanting 100 Hz:
//
//   sleep_for(period) after the work       1210 ms     82.6 Hz
//   sleep_until a fixed grid                1000 ms     99.99 Hz
//
// A 20 percent error from a line that looks right. A controller written for 100 Hz
// and run at 82.6 integrates, differentiates and filters over the wrong dt, with
// nothing to say so.
//
// The tempting repair is to sleep for what is left of the period. 300 cycles:
//
//   sleep_for(period - work)                3032 ms     about 105 to 110 us late
//                                                        per cycle
//   sleep_until a fixed grid                3000 ms     about 1 us late per cycle
//
// 1.1 percent slow. The sleep overshoots a little each cycle, as every sleep does,
// and nothing ever corrects it, so the error is added up. The grid never asks how
// long a sleep took. It asks where the next deadline is, and that is a number that
// does not drift because it is computed from the start and not from the last
// result. The error of one cycle is gone by the next, instead of being kept.
//
// The grid has its own failure, and it is the part worth writing down. After a
// stall, with one 45 ms cycle in a 10 ms loop, a plain fixed deadline ran five late
// cycles back to back with no sleep to catch up. Total time came out right and the
// intervals did not: several updates in a few milliseconds, each with a dt far
// below the period. A controller sees a burst of tiny steps, then silence.
//
// So when a cycle overruns, the slots it lost are DROPPED and the loop waits for
// the next grid point. The loop reports how many it dropped, because a loop that
// silently loses cycles is a loop nobody knows is failing. It never bursts.
//
// The arithmetic is pure integers and takes the time as an argument. That is the
// same idea as lesson 03-05's injectable clock: a loop that reads the time itself
// cannot be tested without waiting, and one that is handed it can be tested in
// no time at all, with a stall of any length.
//
// The rule that falls out: sleep until a deadline computed from the start; when
// late, skip the slots that were lost and count them; never catch up with a burst;
// and hand the clock in.

#ifndef RC_RT_PACER_HPP
#define RC_RT_PACER_HPP

#include <rc/core/clock.hpp>

namespace rc {
namespace rt {

// Keeps a loop on a fixed grid of start times, one every period, beginning at start.
//
// Call sleep_needed(now) at the END of each cycle's work, with the current time. It
// says how long to sleep so the next cycle starts on the grid.
class Pacer {
 public:
  // A period of zero or less would mean dividing by it, so it becomes one
  // nanosecond. A loop that cannot hold a rate should say so, not crash.
  Pacer(core::Nanoseconds period, core::Nanoseconds start)
      : period_(period > 0 ? period : 1), deadline_(start) {}

  core::Nanoseconds sleep_needed(core::Nanoseconds now) {
    // Where the cycle that is about to begin should start, worked out from the grid
    // and not from now. This is the line that does not drift.
    const core::Nanoseconds next = deadline_ + period_;

    if (now <= next) {
      const core::Nanoseconds wait = next - now;

      // More than a whole period to wait means the clock went backwards, or this
      // was called early. Waiting that long would stop the loop, so wait one
      // period and start the grid again from here.
      if (wait > period_) {
        deadline_ = now + period_;
        return period_;
      }
      deadline_ = next;
      return wait;
    }

    // Late. Count the whole slots that were overrun, rounding up, and move the
    // grid to the first point at or after now. Those cycles are dropped and not
    // run in a burst.
    const core::Nanoseconds behind = now - next;
    const core::Nanoseconds lost = (behind + period_ - 1) / period_;
    missed_ += lost;
    deadline_ = next + lost * period_;
    return deadline_ - now;
  }

  // The time the current cycle was scheduled to start.
  core::Nanoseconds deadline() const { return deadline_; }

  core::Nanoseconds period() const { return period_; }

  // Slots dropped so far because a cycle overran.
  long long missed() const { return missed_; }

 private:
  core::Nanoseconds period_;
  core::Nanoseconds deadline_;
  long long missed_ = 0;
};

}  // namespace rt
}  // namespace rc

#endif  // RC_RT_PACER_HPP
