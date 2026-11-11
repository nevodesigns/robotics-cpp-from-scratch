// rc/sim/faults.hpp
//
// Injected sensor faults and the monitors that must notice them, from lesson 18-06, graduated.
//
// A FaultInjector switches on one fault at a time (stuck, offset, drift, or a spike on every 25th reading) from a given time, leaving earlier readings alone. A ResidualMonitor raises an alarm when a sensor has been further than a threshold from a model for a run of readings,
// and a StuckMonitor when a reading has stopped changing while it was expected to.
//
// In a position loop with noise of 0.002 and a threshold of 0.02 over 3 readings, a healthy run raised no alarm; an offset of 0.3 was caught 0.02 s after it began, a drift of 0.05 a second after 0.46 s, a spike of 0.4 one reading in 25 never, and a stuck sensor after 2.85 s,
// while the stuck monitor caught it when the robot was asked to move. Over 200 healthy runs a threshold of 0.004 gave false alarms in 200 (persist 1) and 16 (persist 3), and 0.02 in none. A drift of 0.05 a second was caught after 0.46, 1.06, 2.05 and 4.04 s at thresholds of 0.02, 0.05, 0.1 and 0.2.

#ifndef RC_SIM_FAULTS_HPP
#define RC_SIM_FAULTS_HPP

#include <cmath>

namespace rc {
namespace sim {

enum class FaultKind { none, stuck, offset, drift, spike };

// A fault that is switched on at a time, and what it does to every reading after that. Readings before the time are untouched.
//   stuck:  the reading stops changing and repeats the last good one
//   offset: every reading is higher by `magnitude`
//   drift:  every reading is higher by `magnitude` for each second since the fault began
//   spike:  every 25th reading after the start is higher by `magnitude`, and the rest are right
class FaultInjector {
 public:
  void set(FaultKind kind, double start_time, double magnitude) {
    kind_ = kind;
    start_ = start_time;
    magnitude_ = magnitude;
    frozen_ = false;
    since_ = 0;
  }

  // The reading as the faulty sensor gives it, at time `t`, when a healthy one would give `reading`.
  double distort(double reading, double t) {
    last_ = reading;
    if (kind_ == FaultKind::none || t < start_) {
      last_good_ = reading;
      return reading;
    }
    switch (kind_) {
      case FaultKind::stuck:
        if (!frozen_) {
          frozen_ = true;
          frozen_value_ = last_good_;
        }
        return frozen_value_;
      case FaultKind::offset:
        return reading + magnitude_;
      case FaultKind::drift:
        return reading + magnitude_ * (t - start_);
      case FaultKind::spike:
        ++since_;
        return (since_ % 25 == 0) ? reading + magnitude_ : reading;
      case FaultKind::none:
        break;
    }
    return reading;
  }

 private:
  FaultKind kind_ = FaultKind::none;
  double start_ = 0.0;
  double magnitude_ = 0.0;
  bool frozen_ = false;
  double frozen_value_ = 0.0;
  double last_good_ = 0.0;
  double last_ = 0.0;
  int since_ = 0;
};

// Watches how far a sensor is from what a model says it should read. An alarm is raised when the distance has been over `threshold` for `persist`
// readings in a row, and stays raised until the distance has come back within it. One reading over the line is not an alarm.
class ResidualMonitor {
 public:
  ResidualMonitor(double threshold, int persist) : threshold_(threshold), persist_(persist < 1 ? 1 : persist) {}

  bool update(double residual) {
    if (std::fabs(residual) > threshold_) ++run_;
    else run_ = 0;
    return alarm();
  }
  bool alarm() const { return run_ >= persist_; }
  void reset() { run_ = 0; }

 private:
  double threshold_ = 1.0;
  int persist_ = 1;
  int run_ = 0;
};

// Watches for a reading that has stopped changing while it is expected to be. It counts consecutive readings that differ from the one before
// by no more than `eps` while `expected_motion` is true, and alarms at `window` of them. Anything that does change, or a time when no motion
// is expected, starts the count again.
class StuckMonitor {
 public:
  StuckMonitor(int window, double eps) : window_(window < 2 ? 2 : window), eps_(eps) {}

  bool update(double reading, bool expected_motion) {
    if (have_previous_ && expected_motion && std::fabs(reading - previous_) <= eps_) ++run_;
    else run_ = 0;
    previous_ = reading;
    have_previous_ = true;
    return run_ >= window_;
  }

 private:
  int window_ = 2;
  double eps_ = 0.0;
  double previous_ = 0.0;
  bool have_previous_ = false;
  int run_ = 0;
};

}  // namespace sim
}  // namespace rc

#endif  // RC_SIM_FAULTS_HPP
