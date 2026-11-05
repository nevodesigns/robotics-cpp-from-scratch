#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <atomic>
#include <chrono>
#include <cstddef>
#include <thread>

#include <rc/rt/blocking_queue.hpp>
#include <rc/rt/histogram.hpp>
#include <rc/rt/pacer.hpp>
#include <rc/rt/scoped_thread.hpp>
#include <rc/rt/spsc_queue.hpp>
#include <rc/rt/stop.hpp>

// Phase 07 ends with a number: how long, from a sensor producing a reading to a controller having
// it, and how that number is distributed. Every earlier lesson built a piece. This wires them into
// one rig and measures it, and the measurement is the lesson.
//
//   a producer thread       paced on a fixed grid (07-08), stamping each reading with the time
//   a queue between         a lock free one (07-02) or a blocking one (07-06)
//   a consumer thread       takes each reading, and records now minus the stamp in a histogram (07-03)
//   a stop token            asks both to finish (07-14), and both are joined (07-07)
//
// Measured on this curriculum's machine, gcc 11, a 200 Hz producer for 2 seconds, 400 readings, run
// twice, the consumer waiting three ways. Latency from stamp to taken, and the consumer thread's CPU:
//
//                           p50            p99              max              consumer CPU
//   spins, with yield     0.4 us         1.3 to 1.6 us    12 to 50 us      1999 ms of 2000
//   polls every 1 ms      514 to 549 us  1080 to 1146 us  1087 to 1254 us  31 to 32 ms
//   sleeps on a condition 39 to 65 us    167 to 245 us    473 to 1548 us   4.7 to 8.9 ms
//
// Nothing is free. Spinning is a thousand times faster at the median and uses a whole core to do it,
// which is lesson 07-06's finding in a different light. Polling is cheap and adds about half the poll
// interval to every reading. Sleeping on a condition costs a few tens of microseconds of wakeup and
// almost no CPU, and has the worst tail: one reading in the run took 1.5 ms, which is the scheduler
// and not the queue.
//
// Which is right depends on the budget. A 200 Hz loop has 5000 microseconds per cycle. Sleeping's p99
// of 245 us is 5 percent of it. Polling's 1100 us is 22 percent. Spinning's 1.6 us is nothing, and
// it needs a core that nothing else may have. A 1 kHz loop with a 1000 us budget cannot afford to poll
// at 1 ms at all, and a robot with four cores and a vision pipeline cannot afford to spin.
//
// What the suite asserts is not those numbers, which belong to the machine that day. It asserts the
// laws that make any such measurement trustworthy:
//
//   conservation   every reading produced was either delivered or dropped, and the counts add up
//   slots          readings produced plus slots the pacer dropped is the number of cycles in the run
//   drops          a consumer too slow for the producer loses readings, counts them, and never blocks it
//   order          the percentiles are ordered, and the worst is at least the median
//   stop           a request ends the run promptly, with the books still balanced
//   character      polling goes round at most once a millisecond and sleeping about once per
//                  reading, bounds that hold on a busy machine. How often a SPINNER goes round
//                  is printed and not asserted: it depends on the CPU the scheduler grants it
//
// A latency number without its conservation law is a number about whatever got through.
//
// The rule that falls out: measure from a stamp taken when the reading is made, not when it is
// looked at; count what was lost; report a tail and not a mean; and say what the CPU paid.

enum class WaitStyle { kSpin, kPoll, kSleep };

struct RigOptions {
  WaitStyle style = WaitStyle::kSleep;
  int rate_hz = 200;
  std::chrono::milliseconds duration{300};
  std::size_t queue_capacity = 256;
  // How long the consumer spends working on each reading, in microseconds. Zero for none. A
  // consumer that is slower than the producer is how the suite makes readings get dropped.
  int consumer_work_us = 0;
};

struct Report {
  long produced = 0;       // readings the producer made
  long delivered = 0;      // readings the consumer took
  long dropped = 0;        // readings the full queue refused
  long missed_slots = 0;   // cycles the producer's pacer had to drop
  long wakeups = 0;        // times the consumer's loop went round
  long overflows = 0;      // latencies beyond the histogram's range
  double p50_us = 0.0;
  double p99_us = 0.0;
  double max_us = 0.0;
};

struct Reading {
  long long stamp_ns;
};

inline long long now_ns() {
  return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch())
      .count();
}

// TODO: wire the rig. One function, and every piece is one you built.
//
//   Setup
//     period_ns from rate_hz. A SpscQueue<Reading> of options.queue_capacity (lesson 07-02) for the
//     spin and poll styles, a BlockingQueue<Reading> (07-06) for the sleep style. A Histogram with
//     1 us buckets and 20000 of them (07-03). An atomic<bool> producer_done. A StopSource, and a
//     StopCallback on `outside` that requests it, so a caller's request ends the run (07-14).
//
//   The producer, in a ScopedThread (07-07)
//     A Pacer(period_ns, now_ns()) (07-08). Loop while stop is not requested and the run has not
//     lasted options.duration:
//       make a Reading{now_ns()}, ++produced,
//       push it (blocking.push, or lock_free.push, counting a refusal as dropped),
//       sleep for pacer.sleep_needed(now_ns()) nanoseconds if that is positive.
//     At the end: report.missed_slots = pacer.missed(), producer_done = true, and close the blocking
//     queue if that is the style.
//
//   The consumer, in a ScopedThread
//     Loop, counting each round in `wakeups`:
//       sleep style: blocking.pop_for(reading, 50 ms). kGot: take it. kClosed: stop. kTimeout: go round.
//       the others: lock_free.pop. Got one: take it. Nothing and the producer is done: look once more,
//       then stop. Nothing and it is not: yield for the spin style, sleep 1 ms for the poll style.
//     "Take it" is: do consumer_work_us of busy work if asked, then record
//       (now_ns() - reading.stamp_ns) / 1000.0 microseconds in the histogram and ++delivered.
//
//   Join both before reading anything. They are scoped, so closing the braces does it. Then fill in
//   overflows, p50_us, p99_us (percentile 50 and 99) and max_us (worst()).
//
// Latency is measured from the stamp taken when the reading was MADE, not when it was looked at. That
// is the whole difference between a latency and a loop time.
//
// The stub returns an empty report, so every law in the suite is broken by it and nothing hangs.
inline Report run_rig(const RigOptions& options, const rc::rt::StopToken& outside = rc::rt::StopToken()) {
  (void)options;
  (void)outside;
  return Report{};
}

#endif  // LESSON_SOLUTION_HPP
