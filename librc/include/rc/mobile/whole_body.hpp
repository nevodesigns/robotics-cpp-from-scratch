// rc/mobile/whole_body.hpp
//
// The base and the arm as one, from lesson 33-04.
//
// wb_jacobian is the 6 by 8 matrix that gives the hand's velocity (linear then angular, in the world) for the base's forward speed and turning rate and the six joint rates: the first column is the base driving along its heading, the second is the base turning about its centre (which sweeps the hand round it), and the other six are the arm's Jacobian with the arm's base in the world (28-03). wb_rates gives the rates that produce a wanted hand velocity at least cost: with W the diagonal of the costs of the eight rates (the base's two rates cost w_base each, the joints one), the rates are W inverse J transposed (J W inverse J transposed + lambda squared I) inverse times the velocity, which is the weighted damped least squares of 28-05 with a price on the base. run_line draws a straight line with the hand at a speed: each tick it works out a velocity from the line and the error, finds the rates, and moves the base (forward and turning only) and the joints, which are held to their limits. It reports whether the hand reached the end, the base's travel, the largest error from the line, the smallest manipulability of the arm on the way and the distance the hand ended from the goal.
//
// A line of 1.5 m was drawn at 0.15 m/s. The arm alone ended 1.18 m short with its manipulability down to 0.0000; arm and base together reached the end to 0.000 m, the base driving 1.43 m and the manipulability never below 0.0316. With the base made 30 times as costly to move it drove 1.20 m, and made 33 times cheaper 1.50 m.

#ifndef RC_RC_MOBILE_WHOLE_BODY_HPP
#define RC_RC_MOBILE_WHOLE_BODY_HPP

#include <algorithm>
#include <cmath>
#include <rc/arm/ik_dls.hpp>
#include <rc/arm/jacobian.hpp>
#include <rc/arm/singular.hpp>
#include <rc/mobile/mm_frames.hpp>
#include <vector>

namespace rc {
namespace mobile {

using rc::arm::Jacobian;
using rc::arm::Pose;
using rc::arm::forward;
using rc::arm::jacobian;

struct WbJacobian {
  double m[6][8];
  WbJacobian() {
    for (int i = 0; i < 6; ++i)
      for (int j = 0; j < 8; ++j) m[i][j] = 0.0;
  }
};

// The hand's velocity in the world for [base speed, base turn rate, six joint rates].
inline WbJacobian wb_jacobian(const MobileManipulator& mm, const BasePose& b, const std::vector<double>& q) {
  WbJacobian w;
  const Pose axis = arm_base_in_world(mm, b);
  const Pose tool = forward(mm.arm, q, axis, mm.tool);
  // driving along the heading
  w.m[0][0] = std::cos(b.heading);
  w.m[1][0] = std::sin(b.heading);
  // turning about the base's centre: the hand goes round it and turns with it
  w.m[0][1] = -(tool.p[1] - b.y);
  w.m[1][1] = tool.p[0] - b.x;
  w.m[5][1] = 1.0;
  const Jacobian a = jacobian(mm.arm, q, axis, mm.tool, mm.arm.convention);
  for (int r = 0; r < 6; ++r)
    for (std::size_t c = 0; c < a.n && c < 6; ++c) w.m[r][2 + c] = a.m[r][c];
  return w;
}

// The rates (8) that give the hand the velocity xi (6) at least cost, the base's two rates costing w_base each.
inline std::vector<double> wb_rates(const WbJacobian& j, const double xi[6], double w_base, double lambda) {
  std::vector<double> rates(8, 0.0);
  double winv[8];
  for (int c = 0; c < 8; ++c) winv[c] = c < 2 ? 1.0 / w_base : 1.0;
  double a[6][6], y[6];
  for (int r = 0; r < 6; ++r) {
    for (int s = 0; s < 6; ++s) {
      double sum = r == s ? lambda * lambda : 0.0;
      for (int c = 0; c < 8; ++c) sum += j.m[r][c] * winv[c] * j.m[s][c];
      a[r][s] = sum;
    }
  }
  double b[6];
  for (int r = 0; r < 6; ++r) b[r] = xi[r];
  if (!rc::arm::solve6(a, b, y)) return rates;
  for (int c = 0; c < 8; ++c) {
    double sum = 0.0;
    for (int r = 0; r < 6; ++r) sum += j.m[r][c] * y[r];
    rates[c] = winv[c] * sum;
  }
  return rates;
}

struct WbRun {
  bool reached = false;
  double base_travel = 0.0;
  double max_err = 0.0;
  double min_manip = 1e18;
  double end_gap = 0.0;
};

// The hand draws a straight line from where it is to `goal` (world x, y, z) at `speed`, with the base and the arm (use_base false: the arm alone), the tool held at its starting orientation.
inline WbRun run_line(const MobileManipulator& mm, BasePose b, std::vector<double> q, const double goal[3], double speed, double w_base, bool use_base, double dt) {
  WbRun out;
  const Pose start = forward(mm.arm, q, arm_base_in_world(mm, b), mm.tool);
  double dir[3], len = 0.0;
  for (int k = 0; k < 3; ++k) {
    dir[k] = goal[k] - start.p[k];
    len += dir[k] * dir[k];
  }
  len = std::sqrt(len);
  for (int k = 0; k < 3; ++k) dir[k] /= len;
  const double t_end = len / speed + 4.0;
  double s = 0.0;
  for (double t = 0.0; t < t_end; t += dt) {
    s = s + speed * dt < len ? s + speed * dt : len;
    const Pose axis = arm_base_in_world(mm, b);
    const Pose now = forward(mm.arm, q, axis, mm.tool);
    Pose want = start;
    for (int k = 0; k < 3; ++k) want.p[k] = start.p[k] + dir[k] * s;
    double e[6];
    pose_error(want, now, e);
    double xi[6];
    const double ff = s < len ? speed : 0.0;
    for (int k = 0; k < 3; ++k) xi[k] = ff * dir[k] + 3.0 * e[k];
    for (int k = 3; k < 6; ++k) xi[k] = 3.0 * e[k];
    WbJacobian j = wb_jacobian(mm, b, q);
    if (!use_base)
      for (int r = 0; r < 6; ++r) {
        j.m[r][0] = 0.0;
        j.m[r][1] = 0.0;
      }
    const std::vector<double> rates = wb_rates(j, xi, use_base ? w_base : 1.0, 0.05);
    // the line's error: the distance of the hand from the line
    double rel[3], along = 0.0;
    for (int k = 0; k < 3; ++k) {
      rel[k] = now.p[k] - start.p[k];
      along += rel[k] * dir[k];
    }
    double lat = 0.0;
    for (int k = 0; k < 3; ++k) lat += (rel[k] - along * dir[k]) * (rel[k] - along * dir[k]);
    out.max_err = std::fmax(out.max_err, std::sqrt(lat));
    out.min_manip = std::fmin(out.min_manip, rc::arm::manipulability(jacobian(mm.arm, q, axis, mm.tool, mm.arm.convention)));
    if (use_base) {
      b.x += rates[0] * std::cos(b.heading) * dt;
      b.y += rates[0] * std::sin(b.heading) * dt;
      b.heading += rates[1] * dt;
      out.base_travel += std::fabs(rates[0]) * dt;
    }
    for (std::size_t i = 0; i < 6; ++i) {
      q[i] += rates[2 + i] * dt;
      if (q[i] < mm.arm.rows[i].lo) q[i] = mm.arm.rows[i].lo;
      if (q[i] > mm.arm.rows[i].hi) q[i] = mm.arm.rows[i].hi;
    }
  }
  const Pose end = forward(mm.arm, q, arm_base_in_world(mm, b), mm.tool);
  double g = 0.0;
  for (int k = 0; k < 3; ++k) g += (end.p[k] - goal[k]) * (end.p[k] - goal[k]);
  out.end_gap = std::sqrt(g);
  out.reached = out.end_gap < 0.01;
  return out;
}


}  // namespace mobile
}  // namespace rc

#endif
