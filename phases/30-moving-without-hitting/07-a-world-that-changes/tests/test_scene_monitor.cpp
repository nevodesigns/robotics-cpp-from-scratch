#include <rc/test/rc_test.hpp>
#include <sstream>
#include <cstring>

#include <cmath>
#include <iostream>

#include "rc/arm/ik_closed.hpp"
#include "rc/arm/motion_check.hpp"
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
    {"v0", 0.0},
    {"v3", 4.0},
    {"age", 2.0},
    {"speed", 0.5},
    {"grown", 0.5},
    {"clear0", 0.30999999999999994},
    {"clear1", 0.10999999999999993},
    {"far", 0.5},
    {"plen", 0.25},
    {"clear_arm", 0.08999999999999996},
    {"clear_part", -0.09},
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

std::vector<double> radii() {
  std::vector<double> r(6, 0.05);
  r[0] = 0.08;
  r[1] = 0.06;
  r[3] = 0.05;
  r[5] = 0.04;
  return r;
}

SceneObject ball(int id, double x, double y, double z, double r, double stamp, double speed) {
  SceneObject o;
  o.id = id;
  o.kind = 0;
  o.c[0] = x;
  o.c[1] = y;
  o.c[2] = z;
  o.r = r;
  o.stamp = stamp;
  o.speed = speed;
  return o;
}

}  // namespace

RC_TEST("every change raises the version and a plan made before it is stale after") {
  PlanningScene ps;
  const unsigned v0 = ps.version();
  ps.add(ball(1, 1.0, 1.0, 1.0, 0.1, 0.0, 0.5));
  ps.add(ball(2, -1.0, 1.0, 1.0, 0.1, 0.0, 0.0));
  const unsigned plan_v = ps.version();
  RC_CHECK(!plan_is_stale(plan_v, ps));
  const double by[3] = {0.1, 0.0, 0.0};
  RC_CHECK(ps.move(1, by, 1.0));
  RC_CHECK(plan_is_stale(plan_v, ps));
  RC_CHECK(ps.remove(2));
  RC_CHECK(!ps.remove(99));
  rec("v0", v0, 0);
  rec("v3", ps.version(), 0);
  RC_CHECK(ps.version() == v0 + 4);
  RC_CHECK(ps.objects().size() == 1);
}

RC_TEST("an object not seen for a while is treated as bigger by how far it could have gone, up to a cap") {
  PlanningScene ps;
  ps.add(ball(1, 0.5, 0.0, 0.2, 0.05, 0.0, 0.5));
  SceneObject fixed = ball(2, 0.0, 0.5, 0.2, 0.05, 0.0, 0.0);
  ps.add(fixed);
  const Scene now0 = scene_at(ps, 0.0, 0.5), now2 = scene_at(ps, 2.0, 0.5), later = scene_at(ps, 100.0, 0.5);
  rec("age", 2.0, 1);
  rec("speed", 0.5, 1);
  rec("grown", now2.spheres[0].r - 0.05, 2);
  RC_CHECK_NEAR(now0.spheres[0].r, 0.05, 1e-12);
  RC_CHECK_NEAR(now2.spheres[0].r, 0.55, 1e-12);  // 0.5 m/s for 2 s is a metre, and the cap is half of that
  RC_CHECK_NEAR(later.spheres[0].r, 0.55, 1e-12);
  RC_CHECK_NEAR(later.spheres[1].r, 0.05, 1e-12);  // furniture does not grow
  // Seen again, it is back to its size.
  ps.seen(1, 100.0);
  RC_CHECK_NEAR(scene_at(ps, 100.0, 0.5).spheres[0].r, 0.05, 1e-12);
  // The clearance of a pose from it falls as it grows.
  const ArmTable t = elbow_arm();
  std::vector<double> q(6, 0.0);
  q[1] = 0.5;
  q[2] = 0.3;
  const std::vector<Capsule> caps = arm_capsules(t, q, radii(), 0.10);
  PlanningScene one;
  SceneObject o = ball(1, 0.0, 0.0, 0.0, 0.05, 0.0, 0.1);
  for (int k = 0; k < 3; ++k) o.c[k] = caps.back().b[k];
  o.c[0] += 0.4;
  one.add(o);
  const double c0 = scene_distance(caps, scene_at(one, 0.0, 0.3), false).d;
  const double c1 = scene_distance(caps, scene_at(one, 2.0, 0.3), false).d;
  rec("clear0", c0, 2);
  rec("clear1", c1, 2);
  RC_CHECK(c1 < c0 - 0.1);
}

RC_TEST("a path made before an object moved is still free if it moved away and not if it moved onto the path") {
  const ArmTable t = elbow_arm();
  const std::vector<double> r = radii();
  std::vector<double> a(6, 0.0), b(6, 0.0);
  a[0] = -0.5;
  a[1] = 0.6;
  a[2] = 0.3;
  b[0] = 0.5;
  b[1] = 0.6;
  b[2] = 0.3;
  const std::vector<double> bounds = sweep_bounds(t, r, 0.10);
  // The ball starts well to one side of the sweep of the tool.
  std::vector<double> m(6);
  for (std::size_t i = 0; i < 6; ++i) m[i] = 0.5 * (a[i] + b[i]);
  const std::vector<Capsule> half = arm_capsules(t, m, r, 0.10);
  PlanningScene ps;
  SceneObject o = ball(1, 0.0, 0.0, 0.0, 0.06, 0.0, 0.0);
  for (int k = 0; k < 3; ++k) o.c[k] = half.back().b[k];
  o.c[1] += 0.6;  // well to the side of where the tool goes
  ps.add(o);
  const unsigned plan_v = ps.version();
  Scene s0 = scene_at(ps, 0.0, 0.0);
  ArmCheck c;
  c.table = &t;
  c.radii = &r;
  c.tool_length = 0.10;
  c.scene = &s0;
  c.self = false;
  RC_CHECK(edge_conservative(c, a, b, bounds, 0.02).free);
  // Moved a long way off: still free, but the plan is marked stale all the same.
  const double away[3] = {0.0, 0.5, 0.3};
  ps.move(1, away, 1.0);
  rec("far", 0.5, 1);
  Scene s1 = scene_at(ps, 1.0, 0.0);
  c.scene = &s1;
  RC_CHECK(plan_is_stale(plan_v, ps));
  RC_CHECK(edge_conservative(c, a, b, bounds, 0.02).free);
  // Moved onto the path: not free.
  const double onto[3] = {0.0, -1.1, -0.3};  // from where it is to the tool's place in the middle of the move
  ps.move(1, onto, 2.0);
  Scene s2 = scene_at(ps, 2.0, 0.0);
  c.scene = &s2;
  RC_CHECK(!edge_conservative(c, a, b, bounds, 0.02).free);
}

RC_TEST("a part in the gripper makes the arm longer and a pose that was clear is not") {
  const ArmTable t = elbow_arm();
  std::vector<double> q(6, 0.0);
  q[1] = 0.5;
  q[2] = 0.3;
  const std::vector<double> r = radii();
  const std::vector<Capsule> bare = arm_capsules(t, q, r, 0.10);
  PlanningScene ps;
  SceneObject o = ball(1, 0.0, 0.0, 0.0, 0.05, 0.0, 0.0);
  const Pose flange = forward(t, q, Pose(), Pose());
  for (int k = 0; k < 3; ++k) o.c[k] = bare.back().b[k] + 0.18 * flange.r[k][2];  // a little way beyond the tool tip, along the way it points
  ps.add(o);
  const double clear_arm = scene_distance(bare, scene_at(ps, 0.0, 0.0), false).d;
  const HeldArm h = held_arm(r, 0.10, 0.25, 0.04);
  const std::vector<Capsule> held = arm_capsules(t, q, h.radii, h.tool_length);
  const double clear_part = scene_distance(held, scene_at(ps, 0.0, 0.0), false).d;
  rec("plen", 0.25, 2);
  rec("clear_arm", clear_arm, 3);
  rec("clear_part", clear_part, 3);
  RC_CHECK(clear_arm > 0.0);
  RC_CHECK(clear_part < 0.0);
  RC_CHECK_NEAR(h.tool_length, 0.35, 1e-12);
  RC_CHECK_NEAR(h.radii.back(), 0.04, 1e-12);
  RC_CHECK_NEAR(held_arm(r, 0.10, 0.25, 0.01).radii.back(), r.back(), 1e-12);
}
