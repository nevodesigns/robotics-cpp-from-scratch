#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <vector>

#include "solution.hpp"

namespace {

const double kNaN = std::numeric_limits<double>::quiet_NaN();

std::vector<Point> square(double s) { return {{0, 0}, {s, 0}, {s, s}, {0, s}}; }

// An L shape: a 10 x 10 square with the top right 5 x 5 taken out. The reflex corner is at (5, 5).
std::vector<Point> ell() { return {{0, 0}, {10, 0}, {10, 5}, {5, 5}, {5, 10}, {0, 10}}; }

std::vector<Point> diamond() { return {{2, 0}, {4, 2}, {2, 4}, {0, 2}}; }

// The first version everyone writes: count an edge if the point's height is between its ends, inclusive, and the crossing is to the right.
bool naive_inside(const Point& p, const std::vector<Point>& area) {
  int crossings = 0;
  const std::size_t n = area.size();
  for (std::size_t i = 0; i < n; ++i) {
    const Point& a = area[i];
    const Point& b = area[(i + 1) % n];
    if ((a.y <= p.y && p.y <= b.y) || (b.y <= p.y && p.y <= a.y)) {
      if (a.y == b.y) continue;
      const double x_at = (b.x - a.x) * (p.y - a.y) / (b.y - a.y) + a.x;
      if (p.x < x_at) ++crossings;
    }
  }
  return crossings % 2 == 1;
}

const char* name(FenceState s) {
  switch (s) {
    case FenceState::inside: return "inside";
    case FenceState::warning: return "warning";
    case FenceState::outside: return "outside";
    case FenceState::unknown: return "unknown";
  }
  return "?";
}

}  // namespace

RC_TEST("points are inside or outside a square and an L, and a point on the boundary is not inside") {
  const std::vector<Point> sq = square(10);
  const std::vector<Point> l = ell();
  RC_CHECK(inside({5, 5}, sq));
  RC_CHECK(!inside({11, 5}, sq));
  RC_CHECK(!inside({-0.1, 5}, sq));
  RC_CHECK(!inside({5, 0}, sq));          // on an edge
  RC_CHECK(!inside({0, 0}, sq));          // on a vertex
  RC_CHECK(!inside({10, 10}, sq));
  RC_CHECK(inside({2, 8}, l));            // in the upright of the L
  RC_CHECK(inside({8, 2}, l));            // in the foot
  RC_CHECK(!inside({8, 8}, l));           // in the notch
  RC_CHECK(!inside({5, 5}, l));           // the reflex corner
  RC_CHECK(!inside({kNaN, 1.0}, sq));
  RC_CHECK(!inside({1.0, 1.0}, std::vector<Point>{{0, 0}, {1, 1}}));   // not a polygon
  std::cout << "  square and L: inside, outside, on an edge, on a vertex, in the notch and at the reflex corner all gave the answers above\n\n";
}

RC_TEST("the first version of the test, which counts an end on the ray twice, calls an inside point outside") {
  const std::vector<Point> d = diamond();
  const Point p{1.0, 2.0};         // inside, on the same height as the diamond's right corner (4, 2)
  std::cout << "  diamond (2,0) (4,2) (2,4) (0,2), the point (1, 2): the naive count says " << (naive_inside(p, d) ? "inside" : "OUTSIDE") << ", the half open rule says " << (inside(p, d) ? "inside" : "outside") << "\n\n";
  RC_CHECK(!naive_inside(p, d));
  RC_CHECK(inside(p, d));
  RC_CHECK(inside({2, 2}, d));
  RC_CHECK(!inside({5, 2}, d));
  RC_CHECK(!inside({-1, 2}, d));
  RC_CHECK(inside({1.9, 3.9 - 1.0}, d));
}

RC_TEST("the distance to an edge is to the segment, and past the end of a segment it is to the end") {
  const std::vector<Point> l = ell();
  RC_CHECK_NEAR(distance_to_edge({5, 5}, square(10)), 5.0, 1e-12);
  RC_CHECK_NEAR(distance_to_segment({15, 0}, {0, 0}, {10, 0}), 5.0, 1e-12);        // not 0, which is the distance to the line
  RC_CHECK_NEAR(distance_to_segment({5, 3}, {0, 0}, {10, 0}), 3.0, 1e-12);
  RC_CHECK_NEAR(distance_to_segment({3, 4}, {0, 0}, {0, 0}), 5.0, 1e-12);          // a segment of no length
  const double d = distance_to_edge({4, 4}, l);
  std::cout << "  (4, 4) in the L, near the reflex corner (5, 5): " << std::fixed << std::setprecision(3) << d << " m from the nearest edge; to the infinite line of the edge x = 5 it would be 1.000\n\n";
  RC_CHECK_NEAR(d, std::hypot(1.0, 1.0), 1e-12);
  RC_CHECK_NEAR(distance_to_edge({6, 6}, l), 1.0, 1e-12);         // in the notch: the edge x = 5 runs from 5 to 10, so it is square on
  RC_CHECK(std::isinf(distance_to_edge({1, 1}, std::vector<Point>())));
}

RC_TEST("the state is stop inside the buffer, a warning before it, and unknown when the position is not a number") {
  const std::vector<Point> sq = square(20);
  std::cout << "  a 20 m square, stop buffer 2 m, warning 1 m more\n";
  struct Case { const char* what; Point p; FenceState want; };
  const Case cases[] = {{"centre, 10 m from the edge", {10, 10}, FenceState::inside}, {"5 m from the edge", {5, 10}, FenceState::inside}, {"2.5 m from the edge", {2.5, 10}, FenceState::warning},
                        {"1.5 m from the edge", {1.5, 10}, FenceState::outside}, {"on the edge", {0, 10}, FenceState::outside}, {"outside the square", {-3, 10}, FenceState::outside},
                        {"position not a number", {kNaN, kNaN}, FenceState::unknown}};
  for (const Case& c : cases) {
    const FenceState s = fence_state(sq, c.p, 2.0, 1.0);
    std::cout << "  " << c.what << std::string(28 - std::string(c.what).size(), ' ') << name(s) << "\n";
    RC_CHECK(s == c.want);
  }
  std::cout << "\n";
}

RC_TEST("a position that drifts needs a bigger buffer, and when it is bigger than the area there is no inside left") {
  std::cout << "  stopping distance 1.2 m, dead reckoning drift 2 percent of distance since the last fix\n  metres since the fix   buffer\n";
  const double dist[] = {0.0, 50.0, 100.0, 200.0};
  for (double d : dist) std::cout << "  " << std::setw(18) << std::fixed << std::setprecision(0) << d << "   " << std::setprecision(2) << required_buffer(1.2, 0.02, d) << " m\n";
  RC_CHECK_NEAR(required_buffer(1.2, 0.02, 0.0), 1.2, 1e-12);
  RC_CHECK_NEAR(required_buffer(1.2, 0.02, 100.0), 3.2, 1e-12);
  RC_CHECK_NEAR(required_buffer(1.2, -1.0, -5.0), 1.2, 1e-12);
  const std::vector<Point> strip = {{0, 0}, {50, 0}, {50, 6}, {0, 6}};         // a corridor 6 m wide: 3 m from each side at the middle
  const FenceState early = fence_state(strip, {25, 3}, required_buffer(1.2, 0.02, 10.0), 1.0);
  const FenceState late = fence_state(strip, {25, 3}, required_buffer(1.2, 0.02, 100.0), 1.0);
  std::cout << "  in a corridor 6 m wide, in the middle: after 10 m of dead reckoning " << name(early) << ", after 100 m " << name(late) << "\n\n";
  RC_CHECK(early == FenceState::inside);
  RC_CHECK(late == FenceState::outside);
}

RC_TEST("the speed near the edge falls to zero at the margin, and is zero outside") {
  const std::vector<Point> sq = square(40);
  std::cout << "  deceleration 2, latency 0.5 s, margin 0.3 m, position error 0.5 m\n  distance from the edge   allowed speed\n";
  double previous = -1.0;
  const double xs[] = {20.0, 10.0, 4.0, 2.0, 1.0, 0.8};
  double s10 = 0.0;
  for (double x : xs) {
    const double v = fence_speed(sq, {x, 20}, 0.5, 2.0, 0.5, 0.3);
    std::cout << "  " << std::setw(20) << std::fixed << std::setprecision(1) << x << "   " << std::setprecision(3) << v << " m/s\n";
    if (x == 10.0) s10 = v;
    if (previous >= 0.0) RC_CHECK(v <= previous);
    previous = v;
  }
  std::cout << "\n";
  RC_CHECK(s10 > 2.0);
  RC_CHECK(fence_speed(sq, {0.8, 20}, 0.5, 2.0, 0.5, 0.3) == 0.0);
  RC_CHECK(fence_speed(sq, {-1, 20}, 0.5, 2.0, 0.5, 0.3) == 0.0);
  RC_CHECK(fence_speed(sq, {kNaN, 20}, 0.5, 2.0, 0.5, 0.3) == 0.0);
  RC_CHECK(fence_speed(sq, {20, 20}, -3.0, 2.0, 0.5, 0.3) == fence_speed(sq, {20, 20}, 0.0, 2.0, 0.5, 0.3));
}
