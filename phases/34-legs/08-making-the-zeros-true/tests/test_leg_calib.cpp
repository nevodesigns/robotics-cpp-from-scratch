#include <rc/test/rc_test.hpp>

#include <cmath>

#include "solution.hpp"
#include <iomanip>
#include <iostream>

static void rec(const char* key, double value, int digits = 3) {
  std::cout << "@" << key << " " << std::fixed << std::setprecision(digits) << value << std::endl;
  std::cout.unsetf(std::ios::floatfield);
}
[[maybe_unused]] static void recs(const char* key, const std::string& value) { std::cout << "@" << key << " " << value << std::endl; }


namespace {

unsigned long long g_state = 21;
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

// The poses the leg is put in for the measurement: a spread of leans, hip pitches and knee bends.
int poses(double q[][3]) {
  const double q0s[3] = {-0.25, 0.0, 0.25}, q1s[2] = {0.45, 0.9}, q2s[2] = {-1.0, -1.7};
  int n = 0;
  for (int a = 0; a < 3; ++a)
    for (int b = 0; b < 2; ++b)
      for (int c = 0; c < 2; ++c) {
        q[n][0] = q0s[a];
        q[n][1] = q1s[b];
        q[n][2] = q2s[c];
        ++n;
      }
  return n;
}

}  // namespace

RC_TEST("offsets in the joint zeros are found from where the foot was measured") {
  LegGeom g;
  double q[12][3], p[12][3];
  const int n = poses(q);
  const double truth[3] = {2.0 * kDeg, -3.0 * kDeg, 4.0 * kDeg};
  const double noise = 0.002;
  g_state = 21;
  for (int i = 0; i < n; ++i) {
    const FootPos f = leg_fk(g, q[i][0] + truth[0], q[i][1] + truth[1], q[i][2] + truth[2]);
    p[i][0] = f.x + noise * gauss();
    p[i][1] = f.y + noise * gauss();
    p[i][2] = f.z + noise * gauss();
  }
  double d[3], rms = 0.0;
  RC_CHECK(calibrate_offsets(g, q, p, n, 20, d, &rms));
  // how far the foot was from where it was commanded, before and after
  double before = 0.0, after = 0.0;
  for (int i = 0; i < n; ++i) {
    const FootPos c = leg_fk(g, q[i][0], q[i][1], q[i][2]);
    before += std::pow(p[i][0] - c.x, 2) + std::pow(p[i][1] - c.y, 2) + std::pow(p[i][2] - c.z, 2);
  }
  after = rms * rms * n;
  rec("n", n, 0);
  rec("noise_mm", noise * 1000.0, 0);
  rec("o0", 2, 0);
  rec("o1", -3, 0);
  rec("o2", 4, 0);
  rec("f0", d[0] / kDeg, 2);
  rec("f1", d[1] / kDeg, 2);
  rec("f2", d[2] / kDeg, 2);
  rec("before_mm", 1000.0 * std::sqrt(before / n), 1);
  rec("after_mm", 1000.0 * std::sqrt(after / n), 1);
  for (int k = 0; k < 3; ++k) RC_CHECK_NEAR(d[k], truth[k], 0.5 * kDeg);
  RC_CHECK(std::sqrt(after / n) < 0.5 * std::sqrt(before / n));
  RC_CHECK(std::sqrt(after / n) < 3.0 * noise);
}

RC_TEST("noise-free measurements give the offsets exactly, and poses with a straight leg give no fit") {
  LegGeom g;
  double q[12][3], p[12][3];
  const int n = poses(q);
  const double truth[3] = {-1.5 * kDeg, 2.5 * kDeg, 3.5 * kDeg};
  for (int i = 0; i < n; ++i) {
    const FootPos f = leg_fk(g, q[i][0] + truth[0], q[i][1] + truth[1], q[i][2] + truth[2]);
    p[i][0] = f.x;
    p[i][1] = f.y;
    p[i][2] = f.z;
  }
  double d[3], rms = 1.0;
  RC_CHECK(calibrate_offsets(g, q, p, n, 20, d, &rms));
  for (int k = 0; k < 3; ++k) RC_CHECK_NEAR(d[k], truth[k], 1e-9);
  RC_CHECK(rms < 1e-9);
  // a straight leg in every pose: the knee's offset cannot be told from the hip's
  double st_q[4][3], st_p[4][3];
  for (int i = 0; i < 4; ++i) {
    st_q[i][0] = 0.1 * i;
    st_q[i][1] = 0.5;
    st_q[i][2] = 0.0;
    const FootPos f = leg_fk(g, st_q[i][0] + truth[0], st_q[i][1] + truth[1], st_q[i][2] + truth[2]);
    st_p[i][0] = f.x;
    st_p[i][1] = f.y;
    st_p[i][2] = f.z;
  }
  RC_CHECK(!calibrate_offsets(g, st_q, st_p, 4, 20, d, &rms));
}

RC_TEST("a link that is the wrong length is not an offset, and the residual says so") {
  LegGeom g, wrong = g;
  wrong.l1 += 0.010;   // the real thigh is 10 mm longer
  double q[12][3], p[12][3];
  const int n = poses(q);
  const double truth[3] = {2.0 * kDeg, -3.0 * kDeg, 4.0 * kDeg};
  for (int i = 0; i < n; ++i) {
    const FootPos f = leg_fk(wrong, q[i][0] + truth[0], q[i][1] + truth[1], q[i][2] + truth[2]);
    p[i][0] = f.x;
    p[i][1] = f.y;
    p[i][2] = f.z;
  }
  double d[3], rms = 0.0;
  RC_CHECK(calibrate_offsets(g, q, p, n, 30, d, &rms));
  rec("len_mm", 10, 0);
  rec("resid_len_mm", rms * 1000.0, 1);
  RC_CHECK(rms > 0.002);
}
