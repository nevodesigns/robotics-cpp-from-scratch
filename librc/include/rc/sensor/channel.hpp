// rc/sensor/channel.hpp
//
// One sensor's whole story in one place, from lesson 15-04.
//
// The three corrections of phase 15 applied in the order that matters, plus the
// two things a driver has to say and usually does not: whether the reading can
// be trusted, and how late it is.
//
// Measured end to end, on a sensor 1.5 percent short and 42 cm long, jittering
// by 5 cm, dropping one reading in two hundred, arriving 20 ms behind, on a
// robot driving at 1 m/s:
//
//   raw, believed to be about now       0.2414 m
//   checked, calibrated, filtered       0.0378 m
//   carried forward by its age          0.0199 m
//   carried forward by the whole lag    0.0123 m
//
// at a cost of 0.035 s of lag, of which 0.015 is the filter and 0.020 the
// transport. That last line is the point of the class: the cost is a number the
// channel reports rather than something the next person has to guess.

#ifndef RC_SENSOR_CHANNEL
#define RC_SENSOR_CHANNEL

#include <cstddef>

#include <rc/core/calibration.hpp>
#include <rc/core/clock.hpp>
#include <rc/core/filters.hpp>
#include <rc/sensor/stamped.hpp>

namespace rc {
namespace sensor {

// What a channel knows about itself.
//
// A driver that returns only a number forces every caller to guess at the
// difference between these, and they mostly guess "ok".
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
//
// The order is: check, then calibrate, then filter. Only the first of those
// three positions is forced, and it is forced hard. A moving average turns an
// obviously invalid reading into a plausible one, so a range check placed after
// the filter never fires: a dropout that is 0.43 m outside the range moves a
// sixteen sample average by 0.34 m, which is inside it, and then spoils sixteen
// outputs instead of none.
//
// Calibrate and filter, by contrast, commute. Measured over five hundred
// readings the two orders differ by 1.8e-15, because a straight line through an
// average is the average of a straight line. Calibrating first is still the
// better habit: it puts the range check and everything downstream in metres
// rather than in counts, and it survives the day the calibration stops being
// straight.
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

  // A raw reading from the driver, with the moment it describes.
  //
  // Returns whether it was accepted. A rejected reading changes nothing: the
  // filter never sees it, the timestamp is not advanced, and the channel goes
  // stale on its own if they keep being rejected, which is the honest outcome.
  bool submit(double raw, rc::core::Nanoseconds sampled_at) {
    const double calibrated = calibration_.apply(raw);

    // The requirement, negated. A nan fails it rather than passing it, which is
    // E-SENSE-0007, and a dropout code fails it before it can reach the filter.
    if (!(calibrated >= lowest_ && calibrated <= highest_)) {
      ++rejected_;
      return false;
    }

    if (received_ > 0 && calibrated == last_accepted_) {
      ++repeats_;
    } else {
      repeats_ = 0;
    }
    last_accepted_ = calibrated;
    ++received_;

    reading_.value = filter_.update(calibrated);
    reading_.sampled_at = sampled_at;
    reading_.valid = true;
    return true;
  }

  const Stamped<double>& reading() const { return reading_; }
  int rejected() const { return rejected_; }
  int received() const { return received_; }

  Health health(rc::core::Nanoseconds now) const {
    if (!reading_.valid) return Health::no_data;
    if (!fresh(reading_, now, max_age_)) return Health::stale;

    // Only a backstop, and only meaningful when the sensor's noise is larger
    // than the smallest step it can report. A parked sensor whose noise is
    // below its resolution repeats for ever and is perfectly healthy, so this
    // threshold has to come from the sensor rather than from a habit.
    if (stuck_after_ > 0 && repeats_ >= stuck_after_) return Health::stuck;
    return Health::ok;
  }

  // What this channel costs the loop downstream, in seconds.
  //
  // The filter's own lag, which is half its window, plus however old the newest
  // reading already was when it arrived. Both are lag and the loop cannot tell
  // them apart, so a channel that reports the sum is a channel whose cost can
  // be budgeted rather than guessed.
  double lag_seconds(rc::core::Nanoseconds now, double sample_interval) const {
    const double filter_lag = rc::core::MovingAverage::lag_samples(window_) * sample_interval;
    if (!reading_.valid) return filter_lag;
    return filter_lag + age_seconds(reading_, now);
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

}  // namespace sensor
}  // namespace rc

#endif  // RC_SENSOR_CHANNEL
