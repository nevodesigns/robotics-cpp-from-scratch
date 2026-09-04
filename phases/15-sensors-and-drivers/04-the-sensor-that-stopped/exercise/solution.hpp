#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cstddef>

#include <rc/core/calibration.hpp>
#include <rc/core/clock.hpp>
#include <rc/core/filters.hpp>
#include <rc/sensor/stamped.hpp>

using rc::sensor::age_seconds;
using rc::sensor::fresh;
using rc::sensor::Stamped;

// What a channel knows about itself.
enum class Health {
  no_data,  // nothing has arrived yet
  ok,       // a recent reading that is changing
  stale,    // the newest reading is older than this channel will vouch for
  stuck,    // readings are arriving and are all identical
};

inline const char* describe(Health health) {
  switch (health) {
    case Health::no_data: return "no reading has arrived";
    case Health::ok: return "ok";
    case Health::stale: return "the newest reading is too old to act on";
    case Health::stuck: return "readings are arriving unchanged";
  }
  return "unknown";
}

// One sensor, with the three corrections of this phase applied in one place.
class Channel {
 public:
  Channel(rc::core::Calibration calibration, int window, double max_age_seconds,
          double lowest, double highest, int stuck_after)
      : calibration_(calibration),
        filter_(window < 1 ? 1 : window),
        window_(window < 1 ? 1 : window),
        max_age_(max_age_seconds),
        lowest_(lowest),
        highest_(highest),
        stuck_after_(stuck_after) {}

  // TODO 1: accept a raw reading, or refuse it.
  //
  // In this order, and the first step is the one that matters:
  //
  //   1. calibrate the raw reading
  //   2. refuse it unless it is within [lowest_, highest_], counting the
  //      refusal in rejected_ and changing nothing else
  //   3. count how many accepted readings in a row have been identical, in
  //      repeats_, resetting to zero when one differs
  //   4. put it through filter_ and store the result in reading_, stamped at
  //      sampled_at and marked valid
  //
  // Write the range test as the requirement and negate it, so that a nan is
  // refused rather than accepted. That is E-SENSE-0007 again.
  //
  // The check has to come before the filter rather than after it. A moving
  // average turns an obviously invalid reading into a plausible one, and the
  // test in this lesson measures exactly how much of one.
  //
  // Return whether the reading was accepted.
  bool submit(double raw, rc::core::Nanoseconds sampled_at) {
    (void)raw;
    (void)sampled_at;
    return true;
  }

  const Stamped<double>& reading() const { return reading_; }
  int rejected() const { return rejected_; }
  int received() const { return received_; }

  // TODO 2: what this channel is willing to say about itself.
  //
  //   no_data  nothing has ever arrived
  //   stale    the newest reading is not fresh, by max_age_
  //   stuck    stuck_after_ is positive and repeats_ has reached it
  //   ok       none of the above
  //
  // In that order: a channel that has gone quiet is stale rather than stuck,
  // because the last thing it said is old either way and stale is the more
  // useful thing for a caller to hear.
  Health health(rc::core::Nanoseconds now) const {
    (void)now;
    return Health::ok;
  }

  // TODO 3: what this channel costs the loop downstream, in seconds.
  //
  // The filter's own lag, which rc::core::MovingAverage::lag_samples gives you
  // in samples and which sample_interval turns into seconds, plus however old
  // the newest reading already was when it arrived.
  //
  // With no reading yet, the filter's lag is all there is to report.
  //
  // Both halves are lag and the loop cannot tell them apart, which is the whole
  // of lesson 15-03. A channel that reports the sum is a channel whose cost can
  // be budgeted rather than guessed at.
  double lag_seconds(rc::core::Nanoseconds now, double sample_interval) const {
    (void)now;
    (void)sample_interval;
    return 0.0;
  }

 private:
  rc::core::Calibration calibration_;
  rc::core::MovingAverage filter_;
  int window_ = 1;
  double max_age_ = 0.0;
  double lowest_ = 0.0;
  double highest_ = 0.0;
  int stuck_after_ = 0;

  Stamped<double> reading_;
  double last_accepted_ = 0.0;
  int repeats_ = 0;
  int received_ = 0;
  int rejected_ = 0;
};

#endif  // LESSON_SOLUTION_HPP
