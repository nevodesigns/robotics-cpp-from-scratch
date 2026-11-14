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
    {"min_abrupt", 2.0},
    {"rate", 0.2},
    {"min_smooth", 2.0},
    {"t_change", 1.2499999999998093},
    {"jump_abrupt", 0.25},
    {"jump_smooth", 0.00019999999999997797},
    {"v_lo", 0.3},
    {"v_hi", 0.7},
    {"v_mid", 0.5},
    {"flaps_one", 175.0},
    {"flaps_two", 0.0},
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


RC_TEST("the offsets of the four gaits give the contact patterns they should") {
  double o[4];
  gait_offsets(kTrot, o);
  RC_CHECK_NEAR(o[0], o[3], 1e-12);   // the diagonals together
  RC_CHECK_NEAR(o[1], o[2], 1e-12);
  gait_offsets(kPace, o);
  RC_CHECK_NEAR(o[0], o[2], 1e-12);   // the same side together
  gait_offsets(kBound, o);
  RC_CHECK_NEAR(o[0], o[1], 1e-12);   // the front pair together
  GaitTiming g;
  g.duty = 0.75;
  gait_offsets(kWalk, g.offset);
  int least = 4;
  for (int i = 0; i < 1000; ++i) {
    const int n = count_stance(g, i * 0.0005 + 1e-6);
    if (n < least) least = n;
  }
  RC_CHECK(least >= 3);
}

RC_TEST("the offsets move toward the target by the shortest way round and no faster than the rate") {
  double cur[4] = {0.95, 0.5, 0.25, 0.0};
  const double target[4] = {0.05, 0.5, 0.70, 0.0};
  step_offsets_toward(cur, target, 0.1, 0.5);   // at most 0.05 a step
  RC_CHECK_NEAR(cur[0], 0.0, 1e-12);            // 0.95 to 0.0: forward through the wrap, not back by 0.9
  RC_CHECK_NEAR(cur[1], 0.5, 1e-12);
  RC_CHECK_NEAR(cur[2], 0.30, 1e-12);
  for (int i = 0; i < 20; ++i) step_offsets_toward(cur, target, 0.1, 0.5);
  for (int k = 0; k < 4; ++k) {
    double d = cur[k] - target[k];
    d -= std::floor(d + 0.5);
    RC_CHECK_NEAR(d, 0.0, 1e-9);
  }
}

RC_TEST("a change of gait keeps feet down either way, but an abrupt one jumps the legs' phases, and the speed switch has a gap") {
  const double duty = 0.6, period = 0.5;
  int worst_abrupt = 4, worst_smooth = 4;
  double t_change = 0.0;
  for (int k = 0; k < 50; ++k) {
    const double ts = 1.0 + 0.01 * k;
    double tc = 0.0;
    const int a = min_feet_down(kWalk, kTrot, 1e6, ts, duty, period, &tc);
    const int s = min_feet_down(kWalk, kTrot, 0.2, ts, duty, period, &t_change);
    if (a < worst_abrupt) worst_abrupt = a;
    if (s < worst_smooth) worst_smooth = s;
  }
  rec("min_abrupt", worst_abrupt, 0);
  rec("rate", 0.2, 1);
  rec("min_smooth", worst_smooth, 0);
  rec("t_change", t_change, 2);
  RC_CHECK(worst_smooth >= 2 && worst_abrupt >= 2);
  // the harm of an abrupt change is the jump of a leg's phase in one tick
  double cur_a[4], cur_s[4], target[4];
  gait_offsets(kWalk, cur_a);
  gait_offsets(kWalk, cur_s);
  gait_offsets(kTrot, target);
  double jump_a = 0.0, jump_s = 0.0;
  {
    double before[4];
    for (int i = 0; i < 4; ++i) before[i] = cur_a[i];
    step_offsets_toward(cur_a, target, 1e6, 0.001);
    for (int i = 0; i < 4; ++i) {
      double d = cur_a[i] - before[i];
      d -= std::floor(d + 0.5);
      if (std::fabs(d) > jump_a) jump_a = std::fabs(d);
    }
  }
  for (int n = 0; n < 5000; ++n) {
    double before[4];
    for (int i = 0; i < 4; ++i) before[i] = cur_s[i];
    step_offsets_toward(cur_s, target, 0.2, 0.001);
    for (int i = 0; i < 4; ++i) {
      double d = cur_s[i] - before[i];
      d -= std::floor(d + 0.5);
      if (std::fabs(d) > jump_s) jump_s = std::fabs(d);
    }
  }
  rec("jump_abrupt", jump_a, 3);
  rec("jump_smooth", jump_s, 4);
  RC_CHECK(jump_a > 0.2);
  RC_CHECK_NEAR(jump_s, 0.2 * 0.001, 1e-9);
  int c1 = kWalk, c2 = kWalk, flaps_one = 0, flaps_two = 0;
  rec("v_lo", 0.3, 1);
  rec("v_hi", 0.7, 1);
  rec("v_mid", 0.5, 1);
  unsigned long long st = 3;
  for (int i = 0; i < 400; ++i) {
    st = st * 6364136223846793005ULL + 1442695040888963407ULL;
    const double v = 0.5 + 0.1 * (static_cast<double>(st >> 11) / 9007199254740992.0 - 0.5) * 2.0;
    const int n1 = gait_for_speed(v, c1, 0.5, 0.5);
    const int n2 = gait_for_speed(v, c2, 0.6, 0.4);
    if (n1 != c1) ++flaps_one;
    if (n2 != c2) ++flaps_two;
    c1 = n1;
    c2 = n2;
  }
  rec("flaps_one", flaps_one, 0);
  rec("flaps_two", flaps_two, 0);
  RC_CHECK(flaps_one > flaps_two);
  RC_CHECK(gait_for_speed(0.7, kWalk, 0.6, 0.4) == kTrot);
  RC_CHECK(gait_for_speed(0.3, kTrot, 0.6, 0.4) == kWalk);
}
