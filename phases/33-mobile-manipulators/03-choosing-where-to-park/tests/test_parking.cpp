#include <rc/test/rc_test.hpp>
#include <sstream>
#include <cstring>

#include <cmath>

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
    {"tx", 2.0},
    {"ty", 0.5},
    {"bearing_free", -170.00000000000003},
    {"d_free", 0.3600000000000002},
    {"marg_free", 0.9999999999999999},
    {"travel_mid", 2.5593163149560083},
    {"travel_near", 1.2449253248420462},
    {"bearing_block", -29.99999999999997},
    {"none_found", 0.0},
    {"n_eval", 252.0},
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

MobileManipulator make() {
  MobileManipulator m;
  m.arm = elbow_arm();
  m.mount = trans(0.15, 0.0, 0.30);
  m.tool = trans(0.0, 0.0, 0.10);
  return m;
}

double bearing_deg(const double target[2], const BasePose& b) {
  return std::atan2(b.y - target[1], b.x - target[0]) * 180.0 / 3.14159265358979323846;
}

}  // namespace

RC_TEST("with nothing in the way the base is put on the near side, in the middle of the ring when margin counts most") {
  const MobileManipulator m = make();
  const Annulus ring = reach_annulus(m, -0.10, 0.01);
  const double target[2] = {2.0, 0.5};
  BasePose cur;
  ParkParams p;
  p.w_travel = 0.05;
  const ParkChoice c = choose_park(m, ring, target, cur, nullptr, 0, p);
  rec("tx", target[0], 1);
  rec("ty", target[1], 1);
  RC_CHECK(c.found);
  const double axis_d = std::hypot(arm_base_in_world(m, c.base).p[0] - target[0], arm_base_in_world(m, c.base).p[1] - target[1]);
  rec("bearing_free", bearing_deg(target, c.base), 0);
  rec("d_free", axis_d, 2);
  rec("marg_free", c.margin, 2);
  RC_CHECK(axis_d > ring.r_in && axis_d < ring.r_out);
  const double tgt3[3] = {target[0], target[1], 0.20};
  RC_CHECK(arm_can_reach(m, c.base, tgt3));
  // the near side is toward the origin: the bearing from the target to the base is about 180 + 14 degrees
  const double toward_origin = std::atan2(-target[1], -target[0]) * 180.0 / 3.14159265358979323846;
  RC_CHECK(std::fabs(bearing_deg(target, c.base) - toward_origin) < 15.0);
  RC_CHECK(c.free_places == c.evaluated);
}

RC_TEST("weighting travel puts the base at the near edge of the ring, weighting margin in the middle") {
  const MobileManipulator m = make();
  const Annulus ring = reach_annulus(m, -0.10, 0.01);
  const double target[2] = {2.0, 0.5};
  BasePose cur;
  ParkParams mid, near;
  mid.w_travel = 0.0;
  mid.w_clear = 0.0;
  near.w_travel = 5.0;
  near.w_margin = 0.2;
  near.w_clear = 0.0;
  const ParkChoice a = choose_park(m, ring, target, cur, nullptr, 0, mid);
  const ParkChoice b = choose_park(m, ring, target, cur, nullptr, 0, near);
  const double ta = std::hypot(a.base.x - cur.x, a.base.y - cur.y), tb = std::hypot(b.base.x - cur.x, b.base.y - cur.y);
  rec("travel_mid", ta, 2);
  rec("travel_near", tb, 2);
  RC_CHECK(a.found && b.found);
  RC_CHECK(tb < ta);
  RC_CHECK(b.margin < a.margin);
  RC_CHECK(a.margin > 0.5);
}

RC_TEST("obstacles move the choice round them, and obstacles all round give none") {
  const MobileManipulator m = make();
  const Annulus ring = reach_annulus(m, -0.10, 0.01);
  const double target[2] = {2.0, 0.5};
  BasePose cur;
  ParkParams p;
  p.w_travel = 0.3;
  std::vector<Obstacle> row;
  for (double y = -0.7; y <= 1.7; y += 0.25) {
    Obstacle o;
    o.x = 1.4;
    o.y = y;
    o.r = 0.2;
    row.push_back(o);
  }
  const ParkChoice free_c = choose_park(m, ring, target, cur, nullptr, 0, p);
  const ParkChoice blocked = choose_park(m, ring, target, cur, row.data(), static_cast<int>(row.size()), p);
  RC_CHECK(free_c.found && blocked.found);
  for (std::size_t k = 0; k < row.size(); ++k) RC_CHECK(std::hypot(blocked.base.x - row[k].x, blocked.base.y - row[k].y) >= row[k].r + p.base_radius + p.clearance - 1e-9);
  rec("bearing_block", bearing_deg(target, blocked.base), 0);
  RC_CHECK(std::fabs(bearing_deg(target, blocked.base) - bearing_deg(target, free_c.base)) > 20.0);
  // a ring of obstacles round the target leaves no place
  std::vector<Obstacle> all;
  for (int i = 0; i < 24; ++i) {
    Obstacle o;
    o.x = target[0] + 0.55 * std::cos(i * 0.2617993877991494);
    o.y = target[1] + 0.55 * std::sin(i * 0.2617993877991494);
    o.r = 0.9;
    all.push_back(o);
  }
  const ParkChoice none = choose_park(m, ring, target, cur, all.data(), static_cast<int>(all.size()), p);
  rec("none_found", none.free_places, 0);
  rec("n_eval", none.evaluated, 0);
  RC_CHECK(!none.found);
  RC_CHECK(none.free_places == 0);
}
