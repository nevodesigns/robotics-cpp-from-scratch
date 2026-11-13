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

unsigned long long g_state = 12345;
double uni() {
  g_state = g_state * 6364136223846793005ULL + 1442695040888963407ULL;
  return static_cast<double>(g_state >> 11) / 9007199254740992.0;
}
double gauss() {
  double s = 0.0;
  for (int i = 0; i < 12; ++i) s += uni();
  return s - 6.0;
}

Pose truth() { return compose(trans(0.55, -0.10, 0.60), compose(rot_z(0.4), compose(rot_y(2.6), rot_x(0.2)))); }

// Points the tool visits, seen by the camera with noise; the other cloud is where the arm's kinematics say they are (taken as exact).
void make_clouds(int n, double noise, bool flat, Cloud3& cam, Cloud3& base) {
  const Pose t = truth();
  const Pose inv = inverse(t);
  for (int i = 0; i < n; ++i) {
    const double b[3] = {0.35 + 0.30 * uni(), -0.25 + 0.5 * uni(), flat ? 0.20 : 0.10 + 0.30 * uni()};
    double c[3];
    apply(inv, b, c);
    cam.add(c[0] + noise * gauss(), c[1] + noise * gauss(), c[2] + noise * gauss());
    base.add(b[0], b[1], b[2]);
  }
}

double rot_err_deg(const Pose& a, const Pose& b) {
  const Pose d = compose(inverse(a), b);
  const double c = 0.5 * (d.r[0][0] + d.r[1][1] + d.r[2][2] - 1.0);
  return std::acos(c > 1.0 ? 1.0 : (c < -1.0 ? -1.0 : c)) * 180.0 / 3.14159265358979323846;
}

double trans_err_mm(const Pose& a, const Pose& b) {
  double s = 0.0;
  for (int k = 0; k < 3; ++k) s += (a.p[k] - b.p[k]) * (a.p[k] - b.p[k]);
  return 1000.0 * std::sqrt(s);
}

}  // namespace

RC_TEST("a pixel and a depth back project through the pinhole") {
  Intrinsics k;
  double p[3];
  back_project(k, 320.0 + 60.0, 240.0, 0.5, p);
  rec("du", 60, 0);
  rec("z", 0.5, 1);
  rec("x_bp", p[0], 3);
  rec("fx", k.fx, 0);
  RC_CHECK_NEAR(p[0], 0.05, 1e-12);
  RC_CHECK_NEAR(p[1], 0.0, 1e-12);
  RC_CHECK_NEAR(p[2], 0.5, 1e-12);
  back_project(k, 320.0, 240.0 - 120.0, 1.0, p);
  RC_CHECK_NEAR(p[1], -0.2, 1e-12);
}

RC_TEST("clouds without noise give back the transform, and a flat set of points gives no fit") {
  Cloud3 cam, base;
  make_clouds(10, 0.0, false, cam, base);
  const FitResult f = fit_rigid(cam, base);
  RC_CHECK(f.ok);
  RC_CHECK(f.rms < 1e-9);
  RC_CHECK(trans_err_mm(f.t, truth()) < 1e-6);
  RC_CHECK(rot_err_deg(f.t, truth()) < 1e-6);
  Cloud3 camf, basef;
  make_clouds(10, 0.0, true, camf, basef);
  RC_CHECK(!fit_rigid(camf, basef).ok);
  RC_CHECK(!fit_rigid(cam, Cloud3()).ok);
}

RC_TEST("more points average the noise out") {
  const double noise = 0.002;
  const int trials = 30;
  double e_few = 0.0, r_few = 0.0, e_many = 0.0, r_many = 0.0;
  for (int t = 0; t < trials; ++t) {
    Cloud3 c4, b4, c24, b24;
    make_clouds(5, noise, false, c4, b4);
    make_clouds(40, noise, false, c24, b24);
    const FitResult a = fit_rigid(c4, b4);
    const FitResult b = fit_rigid(c24, b24);
    RC_CHECK(a.ok && b.ok);
    e_few += trans_err_mm(a.t, truth()) / trials;
    r_few += rot_err_deg(a.t, truth()) / trials;
    e_many += trans_err_mm(b.t, truth()) / trials;
    r_many += rot_err_deg(b.t, truth()) / trials;
  }
  rec("n_few", 5, 0);
  rec("n_many", 40, 0);
  rec("noise_mm", noise * 1000.0, 0);
  rec("err_few", e_few, 1);
  rec("rot_few", r_few, 2);
  rec("err_many", e_many, 1);
  rec("rot_many", r_many, 2);
  RC_CHECK(e_many < e_few);
  RC_CHECK(r_many < r_few);
  RC_CHECK(e_many < 2.0);
}

RC_TEST("a small tilt in the rotation is a larger error at the end of a reach") {
  Pose bad = truth();
  const Pose tilt = rot_x(1.0 * 3.14159265358979323846 / 180.0);
  bad = compose(truth(), tilt);
  const double reach = 0.6;
  const double in[3] = {0.0, 0.0, reach};
  double a[3], b[3];
  camera_to_base(truth(), in, a);
  camera_to_base(bad, in, b);
  double s = 0.0;
  for (int k = 0; k < 3; ++k) s += (a[k] - b[k]) * (a[k] - b[k]);
  rec("tilt", 1, 0);
  rec("reach", reach, 1);
  rec("tip_err", 1000.0 * std::sqrt(s), 1);
  RC_CHECK_NEAR(1000.0 * std::sqrt(s), reach * 1000.0 * 3.14159265358979323846 / 180.0, 0.2);
}
