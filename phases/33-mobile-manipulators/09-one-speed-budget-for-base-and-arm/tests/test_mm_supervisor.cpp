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
    {"tip_brake", 5.19},
    {"margin", 0.8},
    {"stop_d", 0.5},
    {"v_tip", 2.0376457003120048},
    {"sep_far", 4.0},
    {"status_far", 0.0},
    {"sep_mid", 1.0},
    {"total_mid", 1.2331666624997917},
    {"base_ask", 1.0},
    {"hand_ask", 0.5},
    {"base_mid", 0.8221111083331945},
    {"hand_mid", 0.4110555541665972},
    {"sep_near", 0.2},
    {"status_near", 2.0},
    {"vh", 1.6},
    {"rest", 0.14178842225346733},
    {"rel_speed", 3.1},
    {"kmargin", 0.17},
    {"short_mm", 28.211577746532683},
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

struct Meet {
  double sep = 0.0;       // the separation at rest, or at the moment they met
  bool met = false;
  double rel_speed = 0.0; // how fast they closed on each other at that moment
};

// A person walks at the robot at vh until the robot is at rest (or they meet); the robot (base and hand together) goes toward them at the speed it is allowed, worked out from the separation a reaction time ago and followed at a bounded deceleration.
Meet run(bool supervised, double base_ask, double hand_ask) {
  const SsmParams ssm;
  const double dt = 0.001, d0 = 4.0;
  const int lag = static_cast<int>(ssm.t_r / dt);
  std::vector<double> seps;
  double xr = 0.0, xh = d0, v = 0.0;
  bool moved = false;
  Meet out;
  for (int i = 0; i < 40000; ++i) {
    const double s = xh - xr;
    seps.push_back(s);
    const double seen = i >= lag ? seps[i - lag] : d0;
    double want = base_ask + hand_ask;
    if (supervised) {
      const SpeedLimits l = supervise(ssm, seen, base_ask, hand_ask, 5.19, 0.8, 0.5);
      want = l.base + l.hand;
    }
    if (v > want) {
      v -= ssm.decel * dt;
      if (v < want) v = want;
    } else {
      v += 3.0 * dt;
      if (v > want) v = want;
    }
    if (v > 0.05) moved = true;
    xr += v * dt;
    xh -= ssm.v_h * dt;
    if (xh - xr <= 0.0) {
      out.sep = xh - xr;
      out.met = true;
      out.rel_speed = v + ssm.v_h;
      return out;
    }
    if (moved && v < 1e-6) {
      out.sep = xh - xr;
      return out;
    }
  }
  out.sep = xh - xr;
  return out;
}

}  // namespace

RC_TEST("the speed to stop in a distance by braking at a fraction of the tipping deceleration") {
  const double a = 5.19, margin = 0.8, d = 0.5;
  const double v = tip_speed_limit(a, margin, d);
  rec("tip_brake", a, 2);
  rec("margin", margin, 1);
  rec("stop_d", d, 1);
  rec("v_tip", v, 2);
  RC_CHECK_NEAR(v * v / (2.0 * margin * a), d, 1e-12);
  RC_CHECK(tip_speed_limit(a, margin, 1.0) > v);
  RC_CHECK(tip_speed_limit(2.0, margin, d) < v);
}

RC_TEST("the base is held to its own limit, and base and hand are scaled together to the person's total") {
  const SpeedLimits free_l = share_budget(0.5, 0.2, 2.0, 5.0);
  RC_CHECK(free_l.status == 0 && free_l.binding == 0);
  RC_CHECK_NEAR(free_l.base, 0.5, 1e-12);
  RC_CHECK_NEAR(free_l.hand, 0.2, 1e-12);
  const SpeedLimits capped = share_budget(3.0, 0.1, 2.0, 5.0);
  RC_CHECK(capped.status == 1 && capped.binding == 1);
  RC_CHECK_NEAR(capped.base, 2.0, 1e-12);
  RC_CHECK_NEAR(capped.hand, 0.1, 1e-12);
  const SpeedLimits scaled = share_budget(0.8, 0.4, 2.0, 0.6);
  RC_CHECK(scaled.status == 1 && scaled.binding == 2);
  RC_CHECK_NEAR(scaled.base + scaled.hand, 0.6, 1e-12);
  RC_CHECK_NEAR(scaled.base / scaled.hand, 2.0, 1e-9);
  const SpeedLimits stop = share_budget(0.8, 0.4, 2.0, 0.0);
  RC_CHECK(stop.status == 2 && stop.base == 0.0 && stop.hand == 0.0);
  // moving away from a person (a negative speed toward them) is not scaled
  const SpeedLimits away = share_budget(-0.5, 0.2, 2.0, 0.6);
  RC_CHECK_NEAR(away.base, -0.5, 1e-12);
  RC_CHECK_NEAR(away.hand, 0.2, 1e-12);
}

RC_TEST("the supervisor follows the person's distance, from free to scaled to stopped") {
  const SsmParams ssm;
  const double ba = 1.0, ha = 0.5;
  const SpeedLimits far_l = supervise(ssm, 4.0, ba, ha, 5.19, 0.8, 0.5);
  const SpeedLimits mid = supervise(ssm, 1.0, ba, ha, 5.19, 0.8, 0.5);
  const SpeedLimits near_l = supervise(ssm, 0.2, ba, ha, 5.19, 0.8, 0.5);
  rec("sep_far", 4.0, 1);
  rec("status_far", far_l.status, 0);
  rec("sep_mid", 1.0, 1);
  rec("total_mid", max_speed(ssm, 1.0), 2);
  rec("base_ask", ba, 1);
  rec("hand_ask", ha, 1);
  rec("base_mid", mid.base, 2);
  rec("hand_mid", mid.hand, 2);
  rec("sep_near", 0.2, 1);
  rec("status_near", near_l.status, 0);
  RC_CHECK(far_l.status == 0);
  RC_CHECK(mid.status == 1);
  RC_CHECK_NEAR(mid.base + mid.hand, max_speed(ssm, 1.0), 1e-9);
  RC_CHECK(near_l.status == 2);
}

RC_TEST("against a person walking at the robot the supervised robot stops short and an unsupervised one does not") {
  const Meet sup = run(true, 1.0, 0.5);
  const Meet free_r = run(false, 1.0, 0.5);
  const SsmParams ssm;
  rec("vh", ssm.v_h, 1);
  rec("rest", sup.sep, 2);
  rec("rel_speed", free_r.rel_speed, 1);
  const double margins = ssm.c + ssm.z_d + ssm.z_r;
  rec("kmargin", margins, 2);
  rec("short_mm", (margins - sup.sep) * 1000.0, 0);
  RC_CHECK(!sup.met);
  RC_CHECK(sup.sep > 0.5 * margins);   // well short of contact
  RC_CHECK(sup.sep < margins);          // but inside the margins it was meant to keep
  RC_CHECK(free_r.met);
}
