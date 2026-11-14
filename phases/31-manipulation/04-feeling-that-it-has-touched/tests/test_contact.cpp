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
    {"vslow", 0.2},
    {"vfast", 0.6},
    {"t_slow", 15.000000000000124},
    {"t_fast", 10.000000000000009},
    {"f_slow", 1.3143831431022823},
    {"f_fast", 5.198929911547245},
    {"gerr", 5.0},
    {"thr_lo", 0.05},
    {"thr_hi", 0.8},
    {"alarms_lo", 1.0},
    {"alarms_hi", 0.0},
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

unsigned long long g_state = 99;
double uni() {
  g_state = g_state * 6364136223846793005ULL + 1442695040888963407ULL;
  return static_cast<double>(g_state >> 11) / 9007199254740992.0;
}
double gauss() {
  double s = 0.0;
  for (int i = 0; i < 12; ++i) s += uni();
  return s - 6.0;
}

struct Run {
  bool detected = false;
  double t_detect_ms = 0.0;   // after the joint reached the wall
  double wall_torque_at = 0.0;
  double peak_wall_torque = 0.0;
  int alarms_free = 0;        // declared before the wall was reached
};

// One joint (a link of 2 kg, 0.5 m) driven at vdes into a wall; the model's gravity is gerr out; the torque measured has noise.
Run run(double vdes, double thr, double gerr, double noise, bool stop_on_hit) {
  const double J = 0.2, b = 0.05, mgl = 9.81, qw = 0.5, kw = 2000.0 * 0.25;
  const double dt = 0.001, kp = 5.0;
  double q = 0.2, v = 0.0, t_wall = -1.0;
  ContactDetector d;
  d.thr = thr;
  Run out;
  double vcmd = vdes;
  double prev_v = 0.0;
  for (int i = 0; i < 4000; ++i) {
    const double g_true = mgl * std::cos(q), g_model = (1.0 + gerr) * g_true;
    const double tau = kp * (vcmd - v) + g_model;
    const double wall = q > qw ? kw * (q - qw) : 0.0;
    const double acc = (tau - b * v - g_true - wall) / J;
    const double tau_meas = tau + noise * gauss();
    // what the model says the free motion needs: gravity, friction and the inertia times the acceleration
    const double tau_model = g_model + b * v + J * ((v - prev_v) / dt);
    prev_v = v;
    v += acc * dt;
    q += v * dt;
    if (q > qw && t_wall < 0.0) t_wall = i * dt;
    if (wall > out.peak_wall_torque) out.peak_wall_torque = wall;
    const bool hit = contact_update(d, tau_meas, tau_model);
    if (hit && !out.detected) {
      out.detected = true;
      if (t_wall < 0.0) {
        ++out.alarms_free;
      } else {
        out.t_detect_ms = (i * dt - t_wall) * 1000.0;
        out.wall_torque_at = wall;
      }
      if (stop_on_hit) vcmd = 0.0;
    }
    if (hit && t_wall < 0.0 && out.alarms_free == 0) ++out.alarms_free;
  }
  return out;
}

}  // namespace

RC_TEST("the detector declares contact only when the filtered residual has stayed over the threshold") {
  ContactDetector d;
  d.thr = 1.0;
  d.need = 3;
  d.alpha = 1.0;
  RC_CHECK(!contact_update(d, 0.5, 0.0));
  RC_CHECK(!contact_update(d, 2.0, 0.0));
  RC_CHECK(!contact_update(d, 2.0, 0.0));
  RC_CHECK(contact_update(d, 2.0, 0.0));
  ContactDetector e;
  e.thr = 1.0;
  e.need = 3;
  e.alpha = 1.0;
  contact_update(e, 2.0, 0.0);
  contact_update(e, 2.0, 0.0);
  contact_update(e, 0.0, 0.0);  // a dip resets the count
  RC_CHECK(!contact_update(e, 2.0, 0.0));
  RC_CHECK(!e.hit);
  ContactDetector f;
  f.thr = 1.0;
  f.need = 1;
  f.alpha = 1.0;
  RC_CHECK(contact_update(f, -2.0, 0.0));  // a pull as well as a push
}

RC_TEST("a joint driven into a wall is found to have touched, and a faster approach is a harder hit") {
  g_state = 99;
  const Run slow = run(0.2, 0.5, 0.0, 0.02, true);
  g_state = 99;
  const Run fast = run(0.6, 0.5, 0.0, 0.02, true);
  rec("vslow", 0.2, 1);
  rec("vfast", 0.6, 1);
  rec("t_slow", slow.t_detect_ms, 0);
  rec("t_fast", fast.t_detect_ms, 0);
  rec("f_slow", slow.wall_torque_at, 2);
  rec("f_fast", fast.peak_wall_torque, 2);
  RC_CHECK(slow.detected && fast.detected);
  RC_CHECK(slow.alarms_free == 0 && fast.alarms_free == 0);
  RC_CHECK(fast.peak_wall_torque > slow.peak_wall_torque);
  RC_CHECK(slow.t_detect_ms > 0.0 && slow.t_detect_ms < 200.0);
}

RC_TEST("a threshold below what the model gets wrong gives false alarms in free motion") {
  const double gerr = 0.05;
  g_state = 5;
  const Run lo = run(0.2, 0.05, gerr, 0.02, false);
  g_state = 5;
  const Run hi = run(0.2, 0.8, gerr, 0.02, false);
  rec("gerr", gerr * 100.0, 0);
  rec("thr_lo", 0.05, 2);
  rec("thr_hi", 0.8, 1);
  rec("alarms_lo", lo.alarms_free, 0);
  rec("alarms_hi", hi.alarms_free, 0);
  RC_CHECK(lo.alarms_free >= 1);
  RC_CHECK(hi.alarms_free == 0);
  RC_CHECK(hi.detected);
}
