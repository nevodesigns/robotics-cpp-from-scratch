// Tests of the library itself, not of a lesson: the three fixes of the hardening pass that follows phase 21 (docs/BUILD-LOG.md, "Hardening pass").
// Each test says what the header did before the fix, from a run of the old header, and what it does now.

#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iomanip>
#include <iostream>

#include <rc/control/pid.hpp>
#include <rc/nav/grid.hpp>
#include <rc/nav/odometry.hpp>
#include <rc/nav/plan.hpp>

namespace {

const double kNaN = std::nan("");
const double kInf = INFINITY;
const double kPi = 3.14159265358979323846;

rc::nav::OccupancyGrid free_grid(int w, int h) {
  rc::nav::OccupancyGrid g(w, h, 1.0, 0.0, 0.0);
  for (int y = 0; y < h; ++y)
    for (int x = 0; x < w; ++x) {
      g.observe(x, y, false);
      g.observe(x, y, false);
    }
  return g;
}

void wall(rc::nav::OccupancyGrid& g, int x, int y) {
  for (int i = 0; i < 4; ++i) g.observe(x, y, true);        // a cell seen free twice needs four sightings as a wall to be decided
}

// The distance from the true end of a half circle of radius 1 m, driven with the wheels `turn` radians a step, for an integration scheme.
double half_circle_error(double turn, rc::nav::Integration scheme) {
  rc::sim::Pose start;
  rc::nav::Odometry odo(0.3, start, scheme);
  const int n = static_cast<int>(std::lround(kPi / turn));
  const double forward = turn;                                  // radius 1 m
  for (int i = 0; i < n; ++i) odo.update(forward - turn * 0.15, forward + turn * 0.15);
  const double theta = turn * n;
  return std::hypot(odo.pose().x - std::sin(theta), odo.pose().y - (1.0 - std::cos(theta)));
}

}  // namespace

RC_TEST("a PID whose gains or limits are not usable says so and outputs nothing, where before a NaN gain gave NaN and inverted limits gave an inverted output") {
  rc::control::Pid ok(2.0, 1.0, 0.1, -1.0, 1.0);
  RC_CHECK(ok.valid());
  rc::control::Pid nan_gain(kNaN, 0.0, 0.0, -1.0, 1.0);
  rc::control::Pid inf_gain(1.0, kInf, 0.0, -1.0, 1.0);
  rc::control::Pid nan_limit(1.0, 0.0, 0.0, kNaN, 1.0);
  rc::control::Pid inf_limit(1.0, 0.0, 0.0, -1.0, kInf);
  rc::control::Pid swapped(2.0, 0.0, 0.0, 1.0, -1.0);
  RC_CHECK(!nan_gain.valid() && !inf_gain.valid() && !nan_limit.valid() && !inf_limit.valid() && !swapped.valid());
  RC_CHECK(nan_gain.update(1.0, 0.0, 0.1) == 0.0);
  RC_CHECK(swapped.update(5.0, 0.0, 0.1) == 0.0);                 // it was -1
  RC_CHECK(swapped.update(-5.0, 0.0, 0.1) == 0.0);                // it was +1
  RC_CHECK(swapped.update(0.0, 0.0, 0.1) == 0.0);                 // it was +1 for no error at all
  rc::control::Pid fixed(2.0, 0.0, 0.0, 0.5, 0.5);                 // equal limits are a controller with one output
  RC_CHECK(fixed.valid());
  RC_CHECK(fixed.update(1.0, 0.0, 0.1) == 0.5);
  std::cout << "  a NaN gain, an infinite gain, a NaN limit, an infinite limit and limits the wrong way round are all invalid and output 0; equal limits are valid\n\n";
}

RC_TEST("a measurement that is not a number is refused and leaves the controller as it was, where before it made the integral NaN for good") {
  rc::control::Pid pid(2.0, 1.0, 0.1, -10.0, 10.0);
  double last = 0.0;
  for (int i = 0; i < 5; ++i) last = pid.update(1.0, 0.2, 0.1);
  const double integral = pid.integral();
  const double held = pid.update(1.0, kNaN, 0.1);
  RC_CHECK(held == last);                                         // the last output is held
  RC_CHECK(pid.integral() == integral);                           // and nothing in the controller moved
  RC_CHECK(pid.rejected_samples() == 1);
  RC_CHECK(pid.update(kNaN, 0.2, 0.1) == last);
  RC_CHECK(pid.update(1.0, kInf, 0.1) == last);
  RC_CHECK(pid.rejected_samples() == 3);
  const double after = pid.update(1.0, 0.3, 0.1);
  RC_CHECK(std::isfinite(after) && std::isfinite(pid.integral()));
  // The same stream with the bad sample taken out gives the same answer: the bad sample left no trace.
  rc::control::Pid a(2.0, 1.0, 0.1, -10.0, 10.0);
  rc::control::Pid b(2.0, 1.0, 0.1, -10.0, 10.0);
  const double ms[] = {0.0, 0.2, 0.5};
  double ya = 0.0;
  double yb = 0.0;
  ya = a.update(1.0, ms[0], 0.1);
  yb = b.update(1.0, ms[0], 0.1);
  ya = a.update(1.0, ms[1], 0.1);
  yb = b.update(1.0, ms[1], 0.1);
  a.update(1.0, kNaN, 0.1);
  ya = a.update(1.0, ms[2], 0.1);
  yb = b.update(1.0, ms[2], 0.1);
  RC_CHECK(ya == yb);
  std::cout << "  three bad samples (NaN measurement, NaN setpoint, infinite measurement) were counted and held; the output after them was " << std::fixed << std::setprecision(3) << after << ", finite\n\n";
}

RC_TEST("integrating a step of the wheels along the heading at its start is wrong by about the size of the turn in a step, and the midpoint and the exact arc are not") {
  using rc::nav::Integration;
  std::cout << "  a half circle of radius 1 m, the wheels turning the robot by this much a step: the distance from the true end\n  turn per step   start heading   midpoint        arc\n";
  const double turns[] = {0.2, 0.1, 0.05};
  double start[3];
  double mid[3];
  double arc[3];
  for (int i = 0; i < 3; ++i) {
    start[i] = half_circle_error(turns[i], Integration::start_heading);
    mid[i] = half_circle_error(turns[i], Integration::midpoint);
    arc[i] = half_circle_error(turns[i], Integration::arc);
    std::cout << "  " << std::fixed << std::setprecision(2) << std::setw(8) << turns[i] << " rad    " << std::setprecision(4) << std::setw(10) << start[i] << " m   " << std::setw(10) << mid[i] << " m   " << std::scientific << std::setprecision(1) << arc[i] << " m\n";
  }
  std::cout << "\n";
  for (int i = 0; i < 3; ++i) {
    RC_CHECK_NEAR(start[i], turns[i], 0.1 * turns[i] + 0.01);     // about the size of the turn: 0.20, 0.10, 0.05
    RC_CHECK(arc[i] < 1e-9);
    RC_CHECK(mid[i] < 0.2 * start[i]);
  }
  RC_CHECK(mid[0] > 2.5 * mid[1] && mid[1] > 2.5 * mid[2]);       // halving the step cuts the midpoint's error by about four, the original's by two
  RC_CHECK(start[0] > 1.7 * start[1] && start[1] > 1.7 * start[2]);
}

RC_TEST("a step that does not turn is the same in every scheme, a turn on the spot moves nothing, and the original is still the default") {
  using rc::nav::Integration;
  rc::sim::Pose p;
  p.theta = 0.7;
  rc::nav::Odometry a(0.3, p, Integration::start_heading);
  rc::nav::Odometry b(0.3, p, Integration::midpoint);
  rc::nav::Odometry c(0.3, p, Integration::arc);
  for (int i = 0; i < 10; ++i) {
    a.update(0.1, 0.1);
    b.update(0.1, 0.1);
    c.update(0.1, 0.1);
  }
  RC_CHECK_NEAR(a.pose().x, b.pose().x, 1e-12);
  RC_CHECK_NEAR(a.pose().y, c.pose().y, 1e-12);
  RC_CHECK_NEAR(a.pose().x, 0.7071067811865476 * 0 + std::cos(0.7) * 1.0, 1e-12);
  rc::nav::Odometry spin(0.3, rc::sim::Pose(), Integration::arc);
  spin.update(-0.05, 0.05);
  RC_CHECK_NEAR(spin.pose().x, 0.0, 1e-12);
  RC_CHECK_NEAR(spin.pose().y, 0.0, 1e-12);
  RC_CHECK(spin.pose().theta > 0.3);
  RC_CHECK(rc::nav::Odometry(0.3, rc::sim::Pose()).integration() == Integration::start_heading);
}

RC_TEST("a diagonal step between two walls that touch at a corner is allowed by default as before, and refused when corner cutting is turned off") {
  rc::nav::OccupancyGrid g = free_grid(3, 3);
  wall(g, 1, 0);
  wall(g, 0, 1);
  rc::nav::PlanOptions allowed;
  rc::nav::PlanOptions refused;
  refused.allow_corner_cutting = false;
  const rc::nav::Plan a = rc::nav::plan_path(g, {0, 0}, {1, 1}, allowed);
  const rc::nav::Plan b = rc::nav::plan_path(g, {0, 0}, {1, 1}, refused);
  std::cout << "  walls at (1, 0) and (0, 1), from (0, 0) to (1, 1): allowed " << (a.found ? "found a path of cost " + std::to_string(a.cost).substr(0, 5) : "no path") << "; refused " << (b.found ? "found a path" : "no path") << "\n";
  RC_CHECK(rc::nav::PlanOptions().allow_corner_cutting);           // the default is the old behaviour
  RC_CHECK(a.found && a.cells.size() == 2);
  RC_CHECK_NEAR(a.cost, std::sqrt(2.0), 1e-9);
  RC_CHECK(!b.found);
}

RC_TEST("when corner cutting is refused the plan goes round, no diagonal step of it passes a wall's corner, and in the open the cost is the same") {
  rc::nav::OccupancyGrid g = free_grid(9, 9);
  wall(g, 4, 3);
  wall(g, 3, 4);
  rc::nav::PlanOptions allowed;
  rc::nav::PlanOptions refused;
  refused.allow_corner_cutting = false;
  const rc::nav::Plan a = rc::nav::plan_path(g, {3, 3}, {4, 4}, allowed);
  const rc::nav::Plan b = rc::nav::plan_path(g, {3, 3}, {4, 4}, refused);
  int squeezes = 0;
  for (std::size_t i = 0; i + 1 < b.cells.size(); ++i) {
    const int dx = b.cells[i + 1].x - b.cells[i].x;
    const int dy = b.cells[i + 1].y - b.cells[i].y;
    if (dx != 0 && dy != 0 && (g.classify(b.cells[i].x + dx, b.cells[i].y) == rc::nav::Cell::occupied || g.classify(b.cells[i].x, b.cells[i].y + dy) == rc::nav::Cell::occupied)) ++squeezes;
  }
  std::cout << "  across a corner of two walls: allowed cost " << std::fixed << std::setprecision(3) << a.cost << " in " << a.cells.size() << " cells; refused cost " << b.cost << " in " << b.cells.size() << " cells, " << squeezes << " squeezes\n";
  RC_CHECK(a.found && b.found);
  RC_CHECK(a.cells.size() == 2);
  RC_CHECK(b.cost > 3.0 * a.cost);                                // round the walls, not through the gap
  RC_CHECK(squeezes == 0);
  int squeezes_allowed = 0;
  for (std::size_t i = 0; i + 1 < a.cells.size(); ++i) {
    const int dx = a.cells[i + 1].x - a.cells[i].x;
    const int dy = a.cells[i + 1].y - a.cells[i].y;
    if (dx != 0 && dy != 0 && (g.classify(a.cells[i].x + dx, a.cells[i].y) == rc::nav::Cell::occupied || g.classify(a.cells[i].x, a.cells[i].y + dy) == rc::nav::Cell::occupied)) ++squeezes_allowed;
  }
  RC_CHECK(squeezes_allowed == 1);
  const rc::nav::OccupancyGrid open = free_grid(9, 9);
  const rc::nav::Plan oa = rc::nav::plan_path(open, {0, 0}, {8, 6}, allowed);
  const rc::nav::Plan ob = rc::nav::plan_path(open, {0, 0}, {8, 6}, refused);
  RC_CHECK(oa.found && ob.found);
  RC_CHECK_NEAR(oa.cost, ob.cost, 1e-9);
  std::cout << "  in the open from (0, 0) to (8, 6): the same cost, " << oa.cost << ", either way\n\n";
}
