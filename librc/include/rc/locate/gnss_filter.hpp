// rc/locate/gnss_filter.hpp
//
// A GNSS position filter, from lesson 25-03.
//
// GnssFilter keeps a Kf4 (position and velocity, lesson 24-09). Each fix is taken with the standard deviation of its level (fixed, float or single; none has no fix), so that a fixed fix of 2 cm pulls hard and a single one of 1.5 m barely at all. Before it is used a fix is tested against the filter: the squared distance of the fix from the prediction, in units of the uncertainty of both, must be at most the gate (9.21, the 99 percent point of a chi-square with two degrees of freedom); a fix that fails is refused. After max_refused refusals in a row the filter takes the next fix as a new start, because a filter that refuses everything is not safe, it is lost.
//
// A run of 120 s with 1 percent of fixes thrown 8 m, in four fix levels, ended with a worst position error of 6.04 m for a filter that took every fix and 2.29 m with the gate (about the noise of the plain fixes themselves), which refused 0.2 percent of the fixes that were right; a filter that took every fix as fixed had an rms error of 2.81 m on the single fixes where one that weighed them had 0.67 m; and a robot carried 20 m was found again 68 fixes later by a gate that never gave up and 5 fixes later by one that did.

#ifndef RC_RC_LOCATE_GNSS_FILTER_HPP
#define RC_RC_LOCATE_GNSS_FILTER_HPP

#include <cmath>
#include <rc/field/sensors.hpp>
#include <rc/perceive/track.hpp>

namespace rc {
namespace locate {

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
    const double s00 = kf_.P[0][0] + r, s01 = kf_.P[0][1], s10 = kf_.P[1][0], s11 = kf_.P[1][1] + r;
    const double det = s00 * s11 - s01 * s10;
    const double dx = fix.x - kf_.x[0], dy = fix.y - kf_.x[1];
    const double d2 = det > 0.0 ? (s11 * dx * dx - (s01 + s10) * dx * dy + s00 * dy * dy) / det : 0.0;
    if (cfg_.use_gate && d2 > cfg_.gate) {
      if (++refused_in_a_row_ >= cfg_.max_refused) {
        start(fix);
        return true;
      }
      return false;
    }
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


}  // namespace locate
}  // namespace rc

#endif
