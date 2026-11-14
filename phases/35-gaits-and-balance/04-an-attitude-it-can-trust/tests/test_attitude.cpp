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
    {"bias", 0.5},
    {"secs", 60.0},
    {"err_gyro", 23.056987406294294},
    {"shake", 2.5},
    {"err_acc", 10.159786571096271},
    {"accepted", 100.0},
    {"err_f_acc", 1.022722482317865},
    {"err_f_feet", 0.3967710164554667},
    {"err_trot", 1.022722482317865},
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

unsigned long long g_state = 13;
double uni() {
  g_state = g_state * 6364136223846793005ULL + 1442695040888963407ULL;
  return static_cast<double>(g_state >> 11) / 9007199254740992.0;
}
double gauss() {
  double s = 0.0;
  for (int i = 0; i < 12; ++i) s += uni();
  return s - 6.0;
}

const double kDeg = 3.14159265358979323846 / 180.0;
const double kTwoPi = 6.283185307179586;

struct Result {
  double err_gyro = 0.0, err_acc = 0.0, err_f_acc = 0.0, err_f_feet = 0.0;
  double accepted = 0.0;
};

// A body that rocks as it walks (roll 3 degrees and pitch 2 degrees at 2 Hz) with a gyro biased by 0.5 degrees/s and an accelerometer shaken by the gait; `feet` says whether three or more feet are always down (a walk) or never (a trot).
Result simulate(bool feet, double secs) {
  g_state = 13;
  const double dt = 0.005, bias = 0.5 * kDeg, shake = 2.5;
  AttitudeFilter acc_only, both;
  acc_only.k_feet = 0.0;
  both.k_feet = feet ? 1.0 : 0.0;
  double gyro_roll = 0.0, gyro_pitch = 0.0;
  (void)gyro_pitch;   // integrated for symmetry with the roll; only the roll is reported here
  double s_gyro = 0.0, s_acc = 0.0, s_fa = 0.0, s_ff = 0.0;
  int n = 0, ok = 0, total = 0;
  for (double t = 0.0; t < secs; t += dt) {
    const double roll = 3.0 * kDeg * std::sin(kTwoPi * 2.0 * t), pitch = 2.0 * kDeg * std::sin(kTwoPi * 2.0 * t + 1.0);
    const double rr = 3.0 * kDeg * kTwoPi * 2.0 * std::cos(kTwoPi * 2.0 * t), pr = 2.0 * kDeg * kTwoPi * 2.0 * std::cos(kTwoPi * 2.0 * t + 1.0);
    const double gx = rr + bias + 0.2 * kDeg * gauss(), gy = pr + bias + 0.2 * kDeg * gauss();
    const double a[3] = {-9.81 * std::sin(pitch) + shake * std::sin(kTwoPi * 4.0 * t), 9.81 * std::sin(roll) * std::cos(pitch) + shake * std::sin(kTwoPi * 4.0 * t + 0.7), 9.81 * std::cos(roll) * std::cos(pitch)};
    const double fr = roll + 0.9 * kDeg * gauss(), fp = pitch + 0.65 * kDeg * gauss();
    gyro_roll += gx * dt;
    gyro_pitch += gy * dt;
    attitude_update(acc_only, gx, gy, a, fr, fp, false, dt);
    attitude_update(both, gx, gy, a, fr, fp, feet, dt);
    double ar, ap;
    accel_angles(a, &ar, &ap);
    const double mag = std::sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]);
    ++total;
    if (std::fabs(mag - 9.81) < 0.15 * 9.81) ++ok;
    if (t > 0.5 * secs) {
      ++n;
      s_gyro += (gyro_roll - roll) * (gyro_roll - roll);
      s_acc += (ar - roll) * (ar - roll);
      s_fa += (acc_only.roll - roll) * (acc_only.roll - roll);
      s_ff += (both.roll - roll) * (both.roll - roll);
    }
  }
  Result r;
  r.err_gyro = std::sqrt(s_gyro / n) / kDeg;
  r.err_acc = std::sqrt(s_acc / n) / kDeg;
  r.err_f_acc = std::sqrt(s_fa / n) / kDeg;
  r.err_f_feet = std::sqrt(s_ff / n) / kDeg;
  r.accepted = 100.0 * ok / total;
  return r;
}

}  // namespace

RC_TEST("a level, still body is read as level, and the gate refuses an accelerometer that feels too much") {
  AttitudeFilter f;
  const double level[3] = {0.0, 0.0, 9.81};
  for (int i = 0; i < 2000; ++i) attitude_update(f, 0.0, 0.0, level, 0.0, 0.0, false, 0.01);
  RC_CHECK_NEAR(f.roll, 0.0, 1e-9);
  AttitudeFilter g;
  const double tilted[3] = {0.0, 9.81 * std::sin(0.2), 9.81 * std::cos(0.2)};
  for (int i = 0; i < 4000; ++i) attitude_update(g, 0.0, 0.0, tilted, 0.0, 0.0, false, 0.01);
  RC_CHECK_NEAR(g.roll, 0.2, 1e-3);
  AttitudeFilter h;
  const double shaken[3] = {0.0, 9.81 * std::sin(0.2) + 6.0, 9.81 * std::cos(0.2)};
  for (int i = 0; i < 1000; ++i) attitude_update(h, 0.0, 0.0, shaken, 0.0, 0.0, false, 0.01);
  RC_CHECK_NEAR(h.roll, 0.0, 1e-12);   // a magnitude far from g is not believed
  AttitudeFilter k;
  for (int i = 0; i < 3000; ++i) attitude_update(k, 0.0, 0.0, shaken, 0.1, 0.0, true, 0.01);
  RC_CHECK_NEAR(k.roll, 0.1, 5e-3);    // but the feet can still pull it
}

RC_TEST("the gyro alone drifts, the accelerometer alone is wrong at every step, and the filter is better than both") {
  const double secs = 60.0;
  const Result walk = simulate(true, secs);
  rec("bias", 0.5, 1);
  rec("secs", secs, 0);
  rec("err_gyro", walk.err_gyro, 1);
  rec("shake", 2.5, 1);
  rec("err_acc", walk.err_acc, 1);
  rec("accepted", walk.accepted, 0);
  rec("err_f_acc", walk.err_f_acc, 2);
  rec("err_f_feet", walk.err_f_feet, 2);
  RC_CHECK(walk.err_gyro > 10.0);
  RC_CHECK(walk.err_acc > 3.0);
  RC_CHECK(walk.err_f_acc < 0.5 * walk.err_acc);
  RC_CHECK(walk.err_f_acc < 0.2 * walk.err_gyro);
  RC_CHECK(walk.err_f_feet < walk.err_f_acc);
}

RC_TEST("in a trot the feet cannot be used, and the filter falls back on the gyro and the accelerometer") {
  const Result trot = simulate(false, 60.0);
  const Result walk = simulate(true, 60.0);
  rec("err_trot", trot.err_f_feet, 2);
  RC_CHECK_NEAR(trot.err_f_feet, trot.err_f_acc, 1e-9);
  RC_CHECK(trot.err_f_feet > walk.err_f_feet);
}

RC_TEST("pitch is handled as roll is: the gyro, the accelerometer and the feet each move it, and the defaults are the ones written down") {
  const double g = 9.81, r = 0.3, p = 0.2;
  // gravity seen by a body rolled r and pitched p
  const double acc[3] = {-g * std::sin(p), g * std::cos(p) * std::sin(r), g * std::cos(p) * std::cos(r)};
  double ar = 0.0, ap = 0.0;
  accel_angles(acc, &ar, &ap);
  RC_CHECK_NEAR(ar, r, 1e-12);
  RC_CHECK_NEAR(ap, p, 1e-12);
  const double none[3] = {0.0, 0.0, 0.0};   // an accelerometer reading the gate refuses
  // the gyro alone: a pitch rate turns the pitch and leaves the roll
  AttitudeFilter a;
  for (int i = 0; i < 100; ++i) attitude_update(a, 0.0, 0.1, none, 0.0, 0.0, false, 0.01);
  RC_CHECK_NEAR(a.pitch, 0.1, 1e-9);
  RC_CHECK_NEAR(a.roll, 0.0, 1e-12);
  // the accelerometer alone pulls both angles to the ones gravity shows
  AttitudeFilter b;
  for (int i = 0; i < 4000; ++i) attitude_update(b, 0.0, 0.0, acc, 0.0, 0.0, false, 0.01);
  RC_CHECK_NEAR(b.roll, r, 1e-3);
  RC_CHECK_NEAR(b.pitch, p, 1e-3);
  // the feet alone pull each angle to its own
  AttitudeFilter c;
  for (int i = 0; i < 3000; ++i) attitude_update(c, 0.0, 0.0, none, -0.05, 0.1, true, 0.01);
  RC_CHECK_NEAR(c.roll, -0.05, 1e-3);
  RC_CHECK_NEAR(c.pitch, 0.1, 1e-3);
  // one step with the default gains: the feet pull at 1 per second
  AttitudeFilter d;
  attitude_update(d, 0.0, 0.0, none, 0.0, 1.0, true, 0.01);
  RC_CHECK_NEAR(d.pitch, 0.01, 1e-12);
  // the default gate is 15 percent: a reading 17 percent too large is refused
  const double big[3] = {-1.17 * g * std::sin(p), 0.0, 1.17 * g * std::cos(p)};
  AttitudeFilter e;
  attitude_update(e, 0.0, 0.0, big, 0.0, 0.0, false, 0.01);
  RC_CHECK_NEAR(e.pitch, 0.0, 1e-12);
}
