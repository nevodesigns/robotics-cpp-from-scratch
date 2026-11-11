#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

// A path says where an arm goes, and nothing about when. A joint that is told to be somewhere else at once is a motor that is asked for infinite acceleration, and what it does is trip or
// shake. A move has to have a time: the joint speeds up, goes at a steady speed, and slows down, staying within what the motor and the mechanics allow. The usual shape is a trapezoid of
// speed against time: a ramp up at the greatest acceleration, a level top at the greatest speed, a ramp down.
//
// Two things follow. A move that is short never gets up to speed: the ramps meet before the top, and the shape is a triangle with a lower peak. And an arm has several joints that move
// together, each with its own limits and its own distance, and a move in which they finish at different times is one in which the tool wanders: they have to be given times that make them arrive at once.

struct Trapezoid {
  bool valid = false;        // false if the limits are not positive
  double distance = 0.0;     // signed: how far the joint goes, in the direction it goes
  double v_peak = 0.0;       // the greatest speed it reaches
  double accel = 0.0;        // the acceleration of the ramps
  double t_accel = 0.0;      // how long each ramp takes
  double t_cruise = 0.0;     // how long it goes at v_peak (0 for a triangle)
  double t_total = 0.0;
  bool triangular() const { return t_cruise <= 0.0; }
};

// The quickest move of `distance` that never goes faster than `vmax` or accelerates harder than `amax`, from rest to rest. With a distance too short to reach vmax before it is time to slow
// down, it is a triangle: it accelerates for half the time, decelerates for half, and the peak speed is sqrt(amax * distance).
inline Trapezoid plan_trapezoid(double distance, double vmax, double amax) {
  Trapezoid t;
  if (!(vmax > 0.0) || !(amax > 0.0)) return t;
  t.valid = true;
  t.distance = distance;
  t.accel = amax;
  const double d = std::fabs(distance);
  if (d == 0.0) return t;
  const double ramp_distance = vmax * vmax / (2.0 * amax);        // the distance covered getting up to vmax
  if (2.0 * ramp_distance >= d) {
    t.v_peak = std::sqrt(amax * d);
    t.t_accel = t.v_peak / amax;
    t.t_cruise = 0.0;
  } else {
    t.v_peak = vmax;
    t.t_accel = vmax / amax;
    t.t_cruise = (d - 2.0 * ramp_distance) / vmax;
  }
  t.t_total = 2.0 * t.t_accel + t.t_cruise;
  return t;
}

// Where the joint is at time `t` after the start, relative to where it started (signed, like the distance). Before the start it is at the start, and after the end at the end.
inline double position_at(const Trapezoid& p, double t) {
  if (!p.valid || p.t_total <= 0.0) return 0.0;
  const double sign = p.distance < 0.0 ? -1.0 : 1.0;
  const double d = std::fabs(p.distance);
  if (t <= 0.0) return 0.0;
  if (t >= p.t_total) return sign * d;
  const double ramp = 0.5 * p.accel * p.t_accel * p.t_accel;
  double s;
  if (t <= p.t_accel) s = 0.5 * p.accel * t * t;
  else if (t <= p.t_accel + p.t_cruise) s = ramp + p.v_peak * (t - p.t_accel);
  else {
    const double left = p.t_total - t;
    s = d - 0.5 * p.accel * left * left;
  }
  return sign * s;
}

struct Synchronised {
  double duration = 0.0;               // how long the whole move takes
  std::vector<Trapezoid> joints;       // one profile for each joint, all of this duration
};

// Profiles for several joints that all take the same time: that of the slowest joint on its own. A joint with more time than it needs goes at a lower peak speed, with the same acceleration
// of its ramps, so that it covers its distance in exactly that time: v = (a T - sqrt(a^2 T^2 - 4 a d)) / 2. A joint that does not move stays still. A joint with limits that are not positive
// is not valid and the whole move has duration 0.
inline Synchronised synchronise(const std::vector<double>& distances, const std::vector<double>& vmax, const std::vector<double>& amax) {
  Synchronised s;
  const std::size_t n = distances.size();
  std::vector<Trapezoid> alone(n);
  double slowest = 0.0;
  for (std::size_t i = 0; i < n; ++i) {
    alone[i] = plan_trapezoid(distances[i], i < vmax.size() ? vmax[i] : 0.0, i < amax.size() ? amax[i] : 0.0);
    if (!alone[i].valid) return s;
    slowest = std::max(slowest, alone[i].t_total);
  }
  s.duration = slowest;
  for (std::size_t i = 0; i < n; ++i) {
    Trapezoid t = alone[i];
    const double d = std::fabs(distances[i]);
    if (d > 0.0 && slowest > 0.0) {
      const double a = amax[i];
      const double root = std::sqrt(std::max(0.0, a * a * slowest * slowest - 4.0 * a * d));
      t.v_peak = (a * slowest - root) / 2.0;
      t.t_accel = t.v_peak / a;
      t.t_cruise = slowest - 2.0 * t.t_accel;
      t.t_total = slowest;
    }
    s.joints.push_back(t);
  }
  return s;
}

#endif  // LESSON_SOLUTION_HPP
