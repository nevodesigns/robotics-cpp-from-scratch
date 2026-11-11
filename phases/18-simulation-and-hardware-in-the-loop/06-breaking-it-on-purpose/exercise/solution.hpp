#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>

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
    // TODO: No fault, or a time before the fault starts: remember the reading as last_good_ and return it unchanged.
    // After the start: stuck returns the last good reading from before the fault, the same one every time (freeze it on the first call: frozen_ and frozen_value_);
    // offset returns reading + magnitude_; drift returns reading + magnitude_ * (t - start_); spike counts readings since the start (since_) and adds magnitude_ to every 25th, leaving the others right.
    (void)reading;
    (void)t;
    (void)kind_;
    (void)start_;
    (void)magnitude_;
    (void)frozen_;
    (void)frozen_value_;
    (void)last_good_;
    (void)last_;
    (void)since_;
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
    // TODO: if the absolute value of the residual is over the threshold add one to run_, otherwise run_ goes back to zero. Return alarm(): whether run_ has reached persist_
    (void)residual;
    (void)threshold_;
    (void)run_;
    return false;
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
    // TODO: if there was a previous reading, motion is expected and the reading is within eps_ of the previous one: add one to run_; otherwise run_ goes back to zero.
    // Remember the reading as previous_ (and that there is one). Alarm when run_ has reached window_.
    (void)reading;
    (void)expected_motion;
    (void)window_;
    (void)eps_;
    (void)previous_;
    (void)have_previous_;
    (void)run_;
    return false;
  }

 private:
  int window_ = 2;
  double eps_ = 0.0;
  double previous_ = 0.0;
  bool have_previous_ = false;
  int run_ = 0;
};

#endif  // LESSON_SOLUTION_HPP
