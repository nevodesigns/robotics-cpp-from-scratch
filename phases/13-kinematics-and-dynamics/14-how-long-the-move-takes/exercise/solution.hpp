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
  // TODO: invalid (return the default Trapezoid) if vmax or amax is not above 0. Otherwise valid = true, distance, accel = amax; d = |distance|, and a distance of 0 is a move with no time.
  // ramp_distance = vmax^2 / (2 amax). If 2 ramp_distance >= d it is a triangle: v_peak = sqrt(amax d), t_accel = v_peak / amax, t_cruise = 0. Otherwise v_peak = vmax, t_accel = vmax / amax,
  // t_cruise = (d - 2 ramp_distance) / vmax. t_total = 2 t_accel + t_cruise.
  (void)distance;
  (void)vmax;
  (void)amax;
  Trapezoid t;
  t.t_total = -1.0;
  return t;
}

// Where the joint is at time `t` after the start, relative to where it started (signed, like the distance). Before the start it is at the start, and after the end at the end.
inline double position_at(const Trapezoid& p, double t) {
  // TODO: 0 if not valid or t_total is 0, or t <= 0. At or after t_total, the whole distance. Otherwise s along |distance|: in the first ramp 0.5 a t^2; in the cruise ramp_covered + v_peak (t - t_accel)
  // with ramp_covered = 0.5 a t_accel^2; in the last ramp d - 0.5 a (t_total - t)^2. The sign is that of the distance.
  (void)p;
  (void)t;
  return -99.0;
}

struct Synchronised {
  double duration = 0.0;               // how long the whole move takes
  std::vector<Trapezoid> joints;       // one profile for each joint, all of this duration
};

// Profiles for several joints that all take the same time: that of the slowest joint on its own. A joint with more time than it needs goes at a lower peak speed, with the same acceleration
// of its ramps, so that it covers its distance in exactly that time: v = (a T - sqrt(a^2 T^2 - 4 a d)) / 2. A joint that does not move stays still. A joint with limits that are not positive
// is not valid and the whole move has duration 0.
inline Synchronised synchronise(const std::vector<double>& distances, const std::vector<double>& vmax, const std::vector<double>& amax) {
  // TODO: plan each joint alone with plan_trapezoid (a joint that is not valid makes the whole move duration 0: return an empty Synchronised). duration = the largest t_total. For each joint with a
  // distance above 0, replan it to take exactly that duration T at its own acceleration a: v = (a T - sqrt(max(0, a^2 T^2 - 4 a d))) / 2; t_accel = v / a; t_cruise = T - 2 t_accel;
  // t_total = T. A joint that does not move keeps its profile, which is nothing.
  (void)distances;
  (void)vmax;
  (void)amax;
  Synchronised s;
  s.duration = -1.0;
  return s;
}

#endif  // LESSON_SOLUTION_HPP
