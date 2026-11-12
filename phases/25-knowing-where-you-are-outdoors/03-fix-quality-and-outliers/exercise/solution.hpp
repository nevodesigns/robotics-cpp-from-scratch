#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <rc/field/sensors.hpp>
#include <rc/perceive/track.hpp>

// How much a fix of each level is to be believed: the standard deviation of one coordinate, in metres.
inline double fix_sigma(rc::field::Fix level) {
  if (level == rc::field::Fix::fixed) return 0.02;
  if (level == rc::field::Fix::floating) return 0.25;
  if (level == rc::field::Fix::single) return 1.5;
  return 1.0e9;
}

struct GnssFilterConfig {
  double accel_var = 0.25;   // (m/s2)2: how far the velocity may drift
  double gate = 9.21;        // the 99 percent point of a chi-square with two degrees of freedom
  bool use_gate = true;
  int max_refused = 5;       // fixes refused in a row before the next is taken as a new start
  bool weigh_by_level = true;  // false: every fix is taken to be fixed
};

class GnssFilter {
 public:
  explicit GnssFilter(const GnssFilterConfig& c) : cfg_(c) {}

  // dt since the last call; the fix may be none (not valid). Returns true if the fix was used.
  bool update(double dt, const rc::field::GnssFix& fix) {
    if (!started_) {
      if (!fix.valid) return false;
      start(fix);
      return true;
    }
    rc::perceive::kf_predict(kf_, dt, cfg_.accel_var);
    if (!fix.valid) return false;
    const double sigma = cfg_.weigh_by_level ? fix_sigma(fix.fix) : fix_sigma(rc::field::Fix::fixed);
    const double r = sigma * sigma;
    (void)r;  // TODO: the squared distance of the fix from the prediction in units of their combined uncertainty is (d' S^-1 d) with S the top left 2 by 2 of P plus r on its diagonal; if it is above the gate, refuse the fix, and after max_refused refusals in a row start again from this fix
    refused_in_a_row_ = 0;
    rc::perceive::kf_update(kf_, fix.x, fix.y, r);
    return true;
  }

  double x() const { return kf_.x[0]; }
  double y() const { return kf_.x[1]; }
  double vx() const { return kf_.x[2]; }
  double vy() const { return kf_.x[3]; }
  int restarts() const { return restarts_; }

 private:
  void start(const rc::field::GnssFix& fix) {
    const double sigma = cfg_.weigh_by_level ? fix_sigma(fix.fix) : fix_sigma(rc::field::Fix::fixed);
    rc::perceive::kf_init(kf_, fix.x, fix.y, sigma * sigma, 4.0);
    if (started_) ++restarts_;
    started_ = true;
    refused_in_a_row_ = 0;
  }

  GnssFilterConfig cfg_;
  rc::perceive::Kf4 kf_;
  bool started_ = false;
  int refused_in_a_row_ = 0;
  int restarts_ = 0;
};


#endif
