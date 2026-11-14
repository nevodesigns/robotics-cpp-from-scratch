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
    {"mass", 12.0},
    {"fx", 0.19},
    {"fy", 0.15},
    {"f_each", 29.43},
    {"mx_ok", 10.0},
    {"fmin_ok", 12.763333333333332},
    {"fmax_ok", 46.096666666666664},
    {"mx_max", 17.65799999999995},
    {"expect_max", 17.657999999999998},
    {"push_ok", 10.0},
    {"push_t", 0.3},
    {"t_settle", 0.122},
    {"peak_deg", 5.7346322718899545},
    {"push_big", 25.0},
    {"fell", 1.0},
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

const double kMass = 12.0, kW = 12.0 * 9.81, kFx = 0.19, kFy = 0.15;

void feet4(double feet[4][2]) {
  feet[0][0] = kFx;  feet[0][1] = kFy;
  feet[1][0] = kFx;  feet[1][1] = -kFy;
  feet[2][0] = -kFx; feet[2][1] = kFy;
  feet[3][0] = -kFx; feet[3][1] = -kFy;
}

}  // namespace

RC_TEST("the weight is shared equally with no moment wanted, and wanted moments are given exactly with four or three feet") {
  double feet[4][2], f[4], mx = 0.0, my = 0.0;
  feet4(feet);
  const bool all[4] = {true, true, true, true};
  RC_CHECK(foot_forces(feet, all, kW, 0.0, 0.0, f));
  for (int i = 0; i < 4; ++i) RC_CHECK_NEAR(f[i], kW / 4.0, 1e-9);
  RC_CHECK(foot_forces(feet, all, kW, 6.0, -4.0, f));
  realized_moments(feet, f, &mx, &my);
  RC_CHECK_NEAR(f[0] + f[1] + f[2] + f[3], kW, 1e-9);
  RC_CHECK_NEAR(mx, 6.0, 1e-9);
  RC_CHECK_NEAR(my, -4.0, 1e-9);
  const bool three[4] = {true, true, true, false};
  double f3[4];
  RC_CHECK(foot_forces(feet, three, kW, 0.0, 0.0, f3));
  RC_CHECK_NEAR(f3[3], 0.0, 1e-12);
  RC_CHECK_NEAR(f3[0] + f3[1] + f3[2], kW, 1e-9);
  realized_moments(feet, f3, &mx, &my);
  RC_CHECK_NEAR(mx, 0.0, 1e-9);
  RC_CHECK_NEAR(my, 0.0, 1e-9);
  const bool two[4] = {true, false, false, true};
  RC_CHECK(!foot_forces(feet, two, kW, 0.0, 0.0, f));
}

RC_TEST("a foot can only push, so a moment beyond the weight times the half width cannot be given") {
  double feet[4][2], f[4];
  feet4(feet);
  const bool all[4] = {true, true, true, true};
  rec("mass", kMass, 0);
  rec("fx", kFx, 2);
  rec("fy", kFy, 2);
  rec("f_each", kW / 4.0, 1);
  RC_CHECK(foot_forces(feet, all, kW, 10.0, 0.0, f));
  rec("mx_ok", 10.0, 0);
  rec("fmin_ok", std::fmin(std::fmin(f[0], f[1]), std::fmin(f[2], f[3])), 1);
  rec("fmax_ok", std::fmax(std::fmax(f[0], f[1]), std::fmax(f[2], f[3])), 1);
  double lo = 0.0, hi = 100.0;
  for (int i = 0; i < 50; ++i) {
    const double mid = 0.5 * (lo + hi);
    if (foot_forces(feet, all, kW, mid, 0.0, f)) {
      lo = mid;
    } else {
      hi = mid;
    }
  }
  rec("mx_max", lo, 1);
  rec("expect_max", kW * kFy, 1);
  RC_CHECK_NEAR(lo, kW * kFy, 0.05);
  RC_CHECK(!foot_forces(feet, all, kW, kW * kFy * 1.2, 0.0, f));
  double mx, my;
  realized_moments(feet, f, &mx, &my);
  RC_CHECK_NEAR(mx, kW * kFy, 0.1);   // the moment was scaled down to the most that can be given
  RC_CHECK_NEAR(f[0] + f[1] + f[2] + f[3], kW, 1e-6);   // and the forces still add to the weight
}

RC_TEST("a PD law on the attitude answers a push the feet can answer and rolls over for one they cannot") {
  double feet[4][2];
  feet4(feet);
  const bool all[4] = {true, true, true, true};
  const double inertia = 0.05, kp = 100.0, kd = 4.0, dt = 0.001, push_t = 0.3;
  struct Out { bool fell; double peak; double settle; };
  Out outs[2];
  const double pushes[2] = {10.0, 25.0};
  for (int k = 0; k < 2; ++k) {
    double th = 0.0, om = 0.0, peak = 0.0, settle = -1.0;
    bool fell = false;
    for (int i = 0; i < 4000; ++i) {
      const double t = i * dt;
      const double want = balance_moment(th, om, kp, kd);
      double f[4], mx, my;
      foot_forces(feet, all, kW, want, 0.0, f);
      realized_moments(feet, f, &mx, &my);
      const double d = t < push_t ? pushes[k] : 0.0;
      om += (mx + d) / inertia * dt;
      th += om * dt;
      if (std::fabs(th) > peak) peak = std::fabs(th);
      if (std::fabs(th) > 0.52) {
        fell = true;
        break;
      }
      if (t > push_t && std::fabs(th) < 0.5 * 3.14159265358979323846 / 180.0 && std::fabs(om) < 0.05 && settle < 0.0) settle = t - push_t;
    }
    outs[k].fell = fell;
    outs[k].peak = peak;
    outs[k].settle = settle;
  }
  rec("push_ok", pushes[0], 0);
  rec("push_t", push_t, 1);
  rec("t_settle", outs[0].settle, 2);
  rec("peak_deg", outs[0].peak * 180.0 / 3.14159265358979323846, 1);
  rec("push_big", pushes[1], 0);
  rec("fell", outs[1].fell ? 1 : 0, 0);
  RC_CHECK(!outs[0].fell && outs[0].settle > 0.0 && outs[0].settle < 1.5);
  RC_CHECK(outs[1].fell);
}
