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
    {"h", 0.3},
    {"roll_deg", 5.0},
    {"pitch_deg", -3.0},
    {"err_h", 0.0},
    {"err_r", 1.0336802714756429e-14},
    {"err_p", 4.373262687012335e-15},
    {"noise_mm", 5.0},
    {"sd_h4", 2.547213698298638},
    {"sd_r4", 0.915726879365676},
    {"sd_p4", 0.6472702528023825},
    {"sd_h3", 3.5660710433351714},
    {"sd_r3", 1.2924012220768493},
    {"sd_p3", 0.9462639379052583},
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

unsigned long long g_state = 77;
double uni() {
  g_state = g_state * 6364136223846793005ULL + 1442695040888963407ULL;
  return static_cast<double>(g_state >> 11) / 9007199254740992.0;
}
double gauss() {
  double s = 0.0;
  for (int i = 0; i < 12; ++i) s += uni();
  return s - 6.0;
}

// Four feet standing on level ground, seen from a body at height h, rolled and pitched, in the body's frame.
void feet_of(double h, double roll, double pitch, double noise, double feet[4][3]) {
  const double hx[4] = {0.19, 0.19, -0.19, -0.19}, hy[4] = {0.12, -0.12, 0.12, -0.12};
  const double cr = std::cos(roll), sr = std::sin(roll), cp = std::cos(pitch), sp = std::sin(pitch);
  for (int i = 0; i < 4; ++i) {
    // a foot on the ground a little out from under its hip, in the world (the body's origin above (0, 0, h))
    const double pw[3] = {hx[i] * 1.1, hy[i] * 1.3, 0.0 - h};
    // R^T (p - t) with R = Ry(pitch) Rx(roll), t the body's place (the height is in pw already)
    const double a = cp * pw[0] - sp * pw[2];
    const double b = pw[1];
    const double c = sp * pw[0] + cp * pw[2];
    feet[i][0] = a;
    feet[i][1] = cr * b + sr * c;
    feet[i][2] = -sr * b + cr * c;
    for (int k = 0; k < 3; ++k) feet[i][k] += noise * gauss();
  }
}

const double kDeg = 3.14159265358979323846 / 180.0;

}  // namespace

RC_TEST("the height, roll and pitch of the body are found from four feet on level ground") {
  double feet[4][3];
  const bool all[4] = {true, true, true, true};
  const double h = 0.30, roll = 5.0 * kDeg, pitch = -3.0 * kDeg;
  feet_of(h, roll, pitch, 0.0, feet);
  const BodyPose p = body_pose_from_feet(feet, all);
  rec("h", h, 2);
  rec("roll_deg", 5, 0);
  rec("pitch_deg", -3, 0);
  rec("err_h", std::fabs(p.height - h), 9);
  RC_CHECK(p.ok);
  // the distance of the body's origin from level ground is its height
  RC_CHECK_NEAR(p.height, h, 1e-9);
  rec("err_r", std::fabs(p.roll - roll) / kDeg, 9);
  rec("err_p", std::fabs(p.pitch - pitch) / kDeg, 9);
  RC_CHECK_NEAR(p.roll, roll, 1e-9);
  RC_CHECK_NEAR(p.pitch, pitch, 1e-9);
}

RC_TEST("three feet are enough, fewer are not, and three in a line give no plane") {
  double feet[4][3];
  feet_of(0.30, 4.0 * kDeg, 2.0 * kDeg, 0.0, feet);
  const bool three[4] = {true, true, true, false}, two[4] = {true, true, false, false};
  const BodyPose p3 = body_pose_from_feet(feet, three);
  RC_CHECK(p3.ok);
  RC_CHECK_NEAR(p3.roll, 4.0 * kDeg, 1e-9);
  RC_CHECK(!body_pose_from_feet(feet, two).ok);
  double line[4][3] = {{0, 0, -0.3}, {0.1, 0, -0.3}, {0.2, 0, -0.3}, {0.3, 0, -0.3}};
  const bool all[4] = {true, true, true, true};
  RC_CHECK(!body_pose_from_feet(line, all).ok);
}

RC_TEST("noise in the feet's places comes through to the pose, and a fourth foot averages some of it out") {
  const double noise = 0.005, h = 0.30;
  const int trials = 400;
  double s4[3] = {0, 0, 0}, s3[3] = {0, 0, 0};
  const bool all[4] = {true, true, true, true}, three[4] = {true, true, true, false};
  g_state = 77;
  for (int t = 0; t < trials; ++t) {
    double feet[4][3];
    feet_of(h, 0.0, 0.0, noise, feet);
    const BodyPose a = body_pose_from_feet(feet, all), b = body_pose_from_feet(feet, three);
    RC_CHECK(a.ok && b.ok);
    s4[0] += (a.height - h) * (a.height - h);
    s4[1] += a.roll * a.roll;
    s4[2] += a.pitch * a.pitch;
    s3[0] += (b.height - h) * (b.height - h);
    s3[1] += b.roll * b.roll;
    s3[2] += b.pitch * b.pitch;
  }
  rec("noise_mm", noise * 1000.0, 0);
  rec("sd_h4", 1000.0 * std::sqrt(s4[0] / trials), 1);
  rec("sd_r4", std::sqrt(s4[1] / trials) / kDeg, 2);
  rec("sd_p4", std::sqrt(s4[2] / trials) / kDeg, 2);
  rec("sd_h3", 1000.0 * std::sqrt(s3[0] / trials), 1);
  rec("sd_r3", std::sqrt(s3[1] / trials) / kDeg, 2);
  rec("sd_p3", std::sqrt(s3[2] / trials) / kDeg, 2);
  RC_CHECK(std::sqrt(s4[0] / trials) < 0.005);
  RC_CHECK(std::sqrt(s4[1] / trials) < std::sqrt(s3[1] / trials));
  RC_CHECK(std::sqrt(s4[2] / trials) < std::sqrt(s3[2] / trials));
}
