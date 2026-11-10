#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

#include "solution.hpp"

namespace {

const double kCx = 1.0;
const double kCy = -1.0;
const double kR = 2.0;

// Mean error of the radius and the root mean square error of the centre, over many noisy arcs.
struct Stats {
  double radius_bias = 0.0;
  double radius_rms = 0.0;
  double centre_rms = 0.0;
  int failures = 0;
};

Stats measure(double arc_degrees, bool refine, int runs) {
  Stats s;
  double radius_sum = 0, radius_sq = 0, centre_sq = 0;
  int used = 0;
  for (int seed = 1; seed <= runs; ++seed) {
    const std::vector<Pt> pts = arc_points(static_cast<unsigned>(seed), 40, kCx, kCy, kR, arc_degrees, 0.02);
    Circle c = fit_circle_algebraic(pts);
    if (refine) c = refine_circle(pts, c, 10);
    // A fit of a very short arc can run off to a circle of enormous radius, or to a number that is not one: count it and go on.
    if (!c.valid || !std::isfinite(c.r) || !std::isfinite(c.cx) || !std::isfinite(c.cy) || c.r > 1e4) {
      ++s.failures;
      continue;
    }
    ++used;
    const double e = c.r - kR;
    radius_sum += e;
    radius_sq += e * e;
    centre_sq += (c.cx - kCx) * (c.cx - kCx) + (c.cy - kCy) * (c.cy - kCy);
  }
  if (used > 0) {
    s.radius_bias = radius_sum / used;
    s.radius_rms = std::sqrt(radius_sq / used);
    s.centre_rms = std::sqrt(centre_sq / used);
  }
  return s;
}

}  // namespace

RC_TEST("three points on a circle, and exact points, give that circle") {
  std::vector<Pt> three;
  three.push_back(Pt{3.0, -1.0});       // a circle centred (1, -1) of radius 2
  three.push_back(Pt{1.0, 1.0});
  three.push_back(Pt{-1.0, -1.0});
  const Circle c = fit_circle_algebraic(three);
  RC_CHECK(c.valid);
  RC_CHECK_NEAR(c.cx, 1.0, 1e-9);
  RC_CHECK_NEAR(c.cy, -1.0, 1e-9);
  RC_CHECK_NEAR(c.r, 2.0, 1e-9);

  const std::vector<Pt> exact = arc_points(1, 30, kCx, kCy, kR, 120.0, 0.0);
  const Circle e = fit_circle_algebraic(exact);
  RC_CHECK(e.valid);
  RC_CHECK_NEAR(e.r, kR, 1e-8);
  RC_CHECK_NEAR(e.cx, kCx, 1e-8);

  RC_CHECK(!fit_circle_algebraic(std::vector<Pt>(2, Pt{1.0, 1.0})).valid);
}

RC_TEST("points on a straight line are not on any circle, and the fit says so") {
  std::vector<Pt> line;
  for (int i = 0; i < 10; ++i) line.push_back(Pt{static_cast<double>(i), 2.0 * static_cast<double>(i) + 1.0});
  const Circle c = fit_circle_algebraic(line);
  std::cout << "  ten points on a line: the fit is " << (c.valid ? "valid with a radius of " : "not valid");
  if (c.valid) std::cout << c.r;
  std::cout << "\n\n";
  RC_CHECK(!c.valid || c.r > 1e6);       // either no answer or a circle so large it is a line

  // And the same function does fit a real circle, so refusing everything would not pass.
  RC_CHECK(fit_circle_algebraic(arc_points(2, 20, 0.0, 0.0, 1.0, 270.0, 0.0)).valid);
}

RC_TEST("refining from the quick fit reaches the real distances, and a bad start is returned as it is when it is not valid") {
  const std::vector<Pt> pts = arc_points(4, 60, kCx, kCy, kR, 100.0, 0.02);
  const Circle quick = fit_circle_algebraic(pts);
  const Circle better = refine_circle(pts, quick, 10);
  double sq_quick = 0.0;
  double sq_better = 0.0;
  for (const Pt& p : pts) {
    const double dq = std::hypot(p.x - quick.cx, p.y - quick.cy) - quick.r;
    const double db = std::hypot(p.x - better.cx, p.y - better.cy) - better.r;
    sq_quick += dq * dq;
    sq_better += db * db;
  }
  std::cout << "  an arc of 100 degrees, 60 points: the sum of squared distances to the circle was " << std::scientific << std::setprecision(4)
            << sq_quick << " for the quick fit and " << sq_better << " after refining\n";
  RC_CHECK(quick.valid);
  RC_CHECK(better.valid);
  RC_CHECK(sq_better <= sq_quick);

  Circle none;
  const Circle same = refine_circle(pts, none, 10);
  RC_CHECK(!same.valid);
}

RC_TEST("on a short arc the quick fit makes the circle too small, and refining removes most of it") {
  const int runs = 300;
  std::cout << "\n  a circle of radius 2, noise 0.02, 40 points, " << runs << " runs: mean error of the radius\n"
            << "    arc      quick fit     refined\n";
  const double arcs[] = {360.0, 180.0, 90.0, 60.0};
  double quick_bias_60 = 0.0;
  double refined_bias_60 = 0.0;
  double quick_bias_360 = 0.0;
  for (double arc : arcs) {
    const Stats quick = measure(arc, false, runs);
    const Stats refined = measure(arc, true, runs);
    std::cout << "    " << std::fixed << std::setprecision(0) << std::setw(4) << arc << " deg" << std::setprecision(4) << std::setw(12)
              << quick.radius_bias << std::setw(12) << refined.radius_bias << "\n";
    if (arc == 60.0) {
      quick_bias_60 = quick.radius_bias;
      refined_bias_60 = refined.radius_bias;
    }
    if (arc == 360.0) quick_bias_360 = quick.radius_bias;
  }
  std::cout << "\n";
  RC_CHECK(std::fabs(quick_bias_360) < 0.002);                     // all the way round, the quick fit is fine
  RC_CHECK(quick_bias_60 < -0.005);                                // on 60 degrees it is too small
  RC_CHECK(std::fabs(refined_bias_60) < 0.5 * std::fabs(quick_bias_60));
}

RC_TEST("the shorter the arc the less the radius is known, whatever the method") {
  const int runs = 300;
  const Stats long_arc = measure(180.0, true, runs);
  const Stats short_arc = measure(40.0, true, runs);
  const Stats tiny_arc = measure(15.0, true, runs);
  std::cout << "  scatter of the radius (rms error), refined fit: 180 degrees " << std::fixed << std::setprecision(3) << long_arc.radius_rms
            << ", 40 degrees " << short_arc.radius_rms << ", 15 degrees " << tiny_arc.radius_rms << "\n\n";
  RC_CHECK(short_arc.radius_rms > 3.0 * long_arc.radius_rms);
  RC_CHECK(tiny_arc.radius_rms > 3.0 * short_arc.radius_rms);
}
