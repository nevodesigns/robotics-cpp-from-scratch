#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <rc/core/clock.hpp>

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

// TODO: keep a loop on a fixed grid of start times.
//
// sleep_needed(now) is called at the END of a cycle's work with the current time,
// and returns how long to sleep so the next cycle starts on the grid. deadline_ is
// when the current cycle was scheduled to start.
//
//   next = deadline_ + period_       from the GRID, never from now
//
//   1. On time, now <= next:
//        wait = next - now.
//        If wait > period_, the clock went backwards or this was called early:
//        set deadline_ = now + period_ and return period_.
//        Otherwise deadline_ = next, and return wait.
//
//   2. Late, now > next:
//        behind = now - next
//        lost   = ceil(behind / period_), which is (behind + period_ - 1) / period_
//        add lost to missed_; deadline_ = next + lost * period_
//        return deadline_ - now
//
//      The slots overrun are DROPPED. Do not return 0 to run them back to back:
//      that is the burst, and the suite measures it.
//
// The stub never sleeps, which is the loop everyone writes first except that it
// does not even pause. The suite names it by the rate it gives.
class Pacer {
 public:
  Pacer(rc::core::Nanoseconds period, rc::core::Nanoseconds start)
      : period_(period > 0 ? period : 1), deadline_(start) {}

  rc::core::Nanoseconds sleep_needed(rc::core::Nanoseconds now) {
    (void)now;
    return 0;
  }

  rc::core::Nanoseconds deadline() const { return deadline_; }

  rc::core::Nanoseconds period() const { return period_; }

  long long missed() const { return missed_; }

 private:
  rc::core::Nanoseconds period_;
  rc::core::Nanoseconds deadline_;
  long long missed_ = 0;
};

#endif  // LESSON_SOLUTION_HPP
