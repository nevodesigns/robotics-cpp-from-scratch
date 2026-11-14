#include <rc/test/rc_test.hpp>
#include <sstream>
#include <cstring>

#include <cmath>
#include <iostream>

#include "solution.hpp"
#include <iomanip>
#include <iostream>

// The numbers this lesson publishes: its page, its header and its quiz quote them, so each is held here as it was
// measured. A change that moves one by more than half a unit of its last printed digit fails the suite.
struct Pin {
  const char* key;
  double value;
};
static const Pin kPins[] = {
    {"pp_final", 6.057398595102324e-10},
    {"st_final", 1.1044960943440635e-05},
    {"cut_short", 0.01452554783952543},
    {"cut_long", 0.14934093867543954},
    {"st_circle", 0.035516366543279006},
    {"osc_short", 9.0},
    {"osc_long", 2.0},
    {"corner_raw", 0.38368662340141824},
    {"corner_smooth", 0.40963581715791003},
};

static void rec(const char* key, double value, int digits = 3) {
  std::cout << "@" << key << " " << std::fixed << std::setprecision(digits) << value << std::endl;
  std::cout.unsetf(std::ios::floatfield);
  for (const Pin& pin : kPins) {
    if (std::strcmp(pin.key, key) != 0) continue;
    double allowed = 0.5;
    for (int i = 0; i < digits; ++i) allowed /= 10.0;
    if (!(std::fabs(value - pin.value) <= allowed)) {
      std::ostringstream message;
      message << "the published number " << key << " was " << pin.value << " and is now " << value;
      rc::test::record(__FILE__, __LINE__, message.str());
    }
  }
}
[[maybe_unused]] static void recs(const char* key, const std::string& value) { std::cout << "@" << key << " " << value << std::endl; }


namespace {

const double kPi = 3.14159265358979323846;

CarPose pose(double x, double y, double th) {
  CarPose p;
  p.x = x;
  p.y = y;
  p.theta = th;
  return p;
}

std::vector<CarPose> straight(double x0, double x1) {
  std::vector<CarPose> v;
  for (double x = x0; x <= x1 + 1e-9; x += 0.1) v.push_back(pose(x, 0.0, 0.0));
  return v;
}

// A straight of 4 m and then half a circle of radius 3 m turning left: the curve starts at once, with no easing in.
std::vector<CarPose> straight_then_circle() {
  std::vector<CarPose> v = straight(0.0, 4.0);
  for (double a = 0.1 / 3.0; a <= kPi + 1e-9; a += 0.1 / 3.0) v.push_back(pose(4.0 + 3.0 * std::sin(a), 3.0 - 3.0 * std::cos(a), a));
  return v;
}

FollowConfig cfg_pp(double lookahead) {
  FollowConfig c;
  c.method = kPurePursuit;
  c.lookahead = lookahead;
  return c;
}

FollowConfig cfg_st(double k) {
  FollowConfig c;
  c.method = kStanley;
  c.k = k;
  return c;
}

}  // namespace

RC_TEST("both followers bring a car that starts to the side onto a straight path") {
  const std::vector<CarPose> path = straight(0.0, 20.0);
  const FollowRun a = follow_path(path, pose(0.0, 1.0, 0.0), cfg_pp(1.0));
  const FollowRun b = follow_path(path, pose(0.0, 1.0, 0.0), cfg_st(1.0));
  rec("pp_final", a.final_cte, 3);
  rec("st_final", b.final_cte, 3);
  RC_CHECK(a.reached && b.reached);
  RC_CHECK(a.final_cte < 0.1);
  RC_CHECK(b.final_cte < 0.1);
}

RC_TEST("cross-track is signed, positive on the left of the path") {
  const std::vector<CarPose> path = straight(0.0, 10.0);
  RC_CHECK_NEAR(cross_track(path, 5.0, 0.7), 0.7, 1e-9);
  RC_CHECK_NEAR(cross_track(path, 5.0, -0.4), -0.4, 1e-9);
}

RC_TEST("pure pursuit cuts a curve, more with a longer lookahead, and Stanley hugs it") {
  const std::vector<CarPose> path = straight_then_circle();
  const FollowRun s = follow_path(path, pose(0.0, 0.0, 0.0), cfg_pp(0.6));
  const FollowRun l = follow_path(path, pose(0.0, 0.0, 0.0), cfg_pp(2.0));
  const FollowRun t = follow_path(path, pose(0.0, 0.0, 0.0), cfg_st(1.0));
  rec("cut_short", s.max_cte, 3);
  rec("cut_long", l.max_cte, 3);
  rec("st_circle", t.max_cte, 3);
  RC_CHECK(l.max_cte > s.max_cte);
  RC_CHECK(l.max_cte > 3.0 * t.max_cte);
}

RC_TEST("a very short lookahead swings across the path far more often than a longer one") {
  const std::vector<CarPose> path = straight(0.0, 30.0);
  FollowConfig a = cfg_pp(0.1), b = cfg_pp(1.5);
  a.speed = 2.0;
  b.speed = 2.0;
  const FollowRun ra = follow_path(path, pose(0.0, 1.0, 0.0), a);
  const FollowRun rb = follow_path(path, pose(0.0, 1.0, 0.0), b);
  rec("osc_short", ra.crossings, 0);
  rec("osc_long", rb.crossings, 0);
  RC_CHECK(ra.crossings > rb.crossings);
  RC_CHECK(ra.crossings >= 2);
}

RC_TEST("a corner of 90 degrees is cut by the lookahead, smoothed or not") {
  std::vector<CarPose> raw;
  raw.push_back(pose(0.0, 0.0, 0.0));
  raw.push_back(pose(6.0, 0.0, 0.0));
  raw.push_back(pose(6.0, 6.0, 0.0));
  const std::vector<CarPose> path = resample(raw, 0.1);
  const std::vector<CarPose> smooth = smooth_path(path, nullptr, SmoothConfig()).path;
  const FollowRun a = follow_path(path, pose(0.0, 0.0, 0.0), cfg_pp(1.0));
  const FollowRun b = follow_path(smooth, pose(0.0, 0.0, 0.0), cfg_pp(1.0));
  // Distance of the car's trace to the raw path's corner at (6, 0).
  double da = 1e9, db = 1e9;
  for (std::size_t i = 0; i < a.trace.size(); ++i) da = std::min(da, std::hypot(a.trace[i].x - 6.0, a.trace[i].y));
  for (std::size_t i = 0; i < b.trace.size(); ++i) db = std::min(db, std::hypot(b.trace[i].x - 6.0, b.trace[i].y));
  rec("corner_raw", da, 2);
  rec("corner_smooth", db, 2);
  RC_CHECK(a.reached && b.reached);
  RC_CHECK(da > 0.3);
  RC_CHECK(db > 0.3);
}

RC_TEST("the steering never passes its limit, even from far off the path") {
  const std::vector<CarPose> path = straight(0.0, 10.0);
  const CarPose far = pose(0.0, 5.0, 1.0);
  RC_CHECK(std::fabs(pure_pursuit_steer(far, path, 0, cfg_pp(1.0))) <= 0.4189 + 1e-12);
  RC_CHECK(std::fabs(stanley_steer(far, path, 0, cfg_st(5.0))) <= 0.4189 + 1e-12);
  RC_CHECK_NEAR(stanley_steer(pose(3.0, 0.0, 0.0), path, 30, cfg_st(1.0)), 0.0, 1e-9);
}

RC_TEST("the nearest-point search only moves forward") {
  // A path that goes out and comes back near where it started.
  std::vector<CarPose> u;
  for (double x = 0.0; x <= 5.0; x += 0.1) u.push_back(pose(x, 0.0, 0.0));
  for (double x = 5.0; x >= 0.0; x -= 0.1) u.push_back(pose(x, 0.3, 0.0));
  const std::size_t outbound = nearest_index(u, 2.0, 0.1, 0);
  const std::size_t after = nearest_index(u, 2.0, 0.1, 60);
  RC_CHECK(outbound < 30);
  RC_CHECK(after >= 60);
}
