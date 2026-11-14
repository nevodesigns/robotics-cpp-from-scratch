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
    {"v_probe", 0.5},
    {"u_probe_cm", 14.804666949982664},
    {"iters", 300.0},
    {"bound_cm", 15.0},
    {"h", 0.27},
    {"v_small", 0.2},
    {"kx", 3.0},
    {"kv", 0.9},
    {"v_pd", 0.698298454284668},
    {"v_mpc", 0.709650456905365},
    {"v_limit", 0.9041570660012562},
    {"push", 0.6},
    {"t_pd", 1.05},
    {"t_mpc", 0.65},
    {"t_pd_small", 0.05},
    {"t_mpc_small", 0.4},
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

const double kH = 0.27, kDt = 0.05, kBound = 0.15, kSmall = 0.01;

// The bound on the ZMP at step i: wide for four steps while the support is wide, narrow for four while the swing feet are landing.
double bound_at(int i) { return (i % 8) < 4 ? kBound : kSmall; }

// A push gives the body speed v0 from the middle of the support; the controller's ZMP acts for 4 s; recovered if the body ends at rest near the middle.
bool recovers(bool mpc, double v0, double kx, double kv) {
  double A[2][2], B[2];
  lipm_discretize(kH, kDt, A, B);
  double x = 0.0, v = v0;
  for (int i = 0; i < 80; ++i) {
    double b[15];
    for (int k = 0; k < 15; ++k) b[k] = bound_at(i + k);
    const double p = mpc ? mpc_zmp(x, v, kH, kDt, 15, b, 1.0, 1.0, 0.01, 300, nullptr) : pd_zmp(x, v, kx, kv, b[0]);
    const double nx = A[0][0] * x + A[0][1] * v + B[0] * p;
    const double nv = A[1][0] * x + A[1][1] * v + B[1] * p;
    x = nx;
    v = nv;
    if (std::fabs(x) > 1.0) return false;
  }
  return std::fabs(x) < 0.02 && std::fabs(v) < 0.05;
}

// The number of steps after which the body is within 1 cm of the middle and slower than 5 cm/s (-1 if it does not get there in 8 s).
int settle_steps(bool mpc, double v0, double kx, double kv) {
  double A[2][2], B[2];
  lipm_discretize(kH, kDt, A, B);
  double x = 0.0, v = v0;
  for (int i = 0; i < 160; ++i) {
    double b[15];
    for (int k = 0; k < 15; ++k) b[k] = bound_at(i + k);
    const double p = mpc ? mpc_zmp(x, v, kH, kDt, 15, b, 1.0, 1.0, 0.01, 300, nullptr) : pd_zmp(x, v, kx, kv, b[0]);
    const double nx = A[0][0] * x + A[0][1] * v + B[0] * p;
    const double nv = A[1][0] * x + A[1][1] * v + B[1] * p;
    x = nx;
    v = nv;
    if (std::fabs(x) < 0.01 && std::fabs(v) < 0.05) return i + 1;   // the state after step i is the state at time (i + 1) dt
  }
  return -1;
}

double largest(bool mpc, double kx, double kv) {
  double lo = 0.0, hi = 3.0;
  for (int i = 0; i < 24; ++i) {
    const double mid = 0.5 * (lo + hi);
    if (recovers(mpc, mid, kx, kv)) {
      lo = mid;
    } else {
      hi = mid;
    }
  }
  return lo;
}

}  // namespace

RC_TEST("the discrete model agrees with the pendulum, and with the ZMP under the body the body stays where it is") {
  double A[2][2], B[2];
  lipm_discretize(kH, kDt, A, B);
  const double w = std::sqrt(9.81 / kH);
  // the body at rest over the ZMP stays: x = p = 0.05, v = 0
  const double x = 0.05, v = 0.0, p = 0.05;
  RC_CHECK_NEAR(A[0][0] * x + A[0][1] * v + B[0] * p, x, 1e-12);
  RC_CHECK_NEAR(A[1][0] * x + A[1][1] * v + B[1] * p, v, 1e-12);
  // free motion: x(t) = x0 cosh(w t) + v0 sinh(w t) / w
  RC_CHECK_NEAR(A[0][0], std::cosh(w * kDt), 1e-12);
  RC_CHECK_NEAR(A[0][1], std::sinh(w * kDt) / w, 1e-12);
}

RC_TEST("the controller's commands respect the bound and push the body back") {
  double u[20], b[15], wide[15];
  for (int k = 0; k < 15; ++k) {
    b[k] = bound_at(k);
    wide[k] = kBound;
  }
  const double c = mpc_zmp(0.0, 0.5, kH, kDt, 15, wide, 1.0, 1.0, 0.01, 300, u);
  RC_CHECK(c > 0.0 && c <= kBound + 1e-12);   // the body is moving out, the ZMP goes out ahead of it to push it back
  for (int i = 0; i < 15; ++i) RC_CHECK(std::fabs(u[i]) <= kBound + 1e-12);
  double u2[20];
  mpc_zmp(0.0, 0.5, kH, kDt, 15, b, 1.0, 1.0, 0.01, 300, u2);
  for (int i = 0; i < 15; ++i) RC_CHECK(std::fabs(u2[i]) <= b[i] + 1e-12);   // each command within its own bound
  RC_CHECK_NEAR(mpc_zmp(0.0, 0.0, kH, kDt, 15, wide, 1.0, 1.0, 0.01, 300, nullptr), 0.0, 1e-9);   // at rest in the middle: nothing to do
  const double probe = 0.5;
  const double u0 = mpc_zmp(0.0, probe, kH, kDt, 15, wide, 1.0, 1.0, 0.01, 300, nullptr);
  rec("v_probe", probe, 1);
  rec("u_probe_cm", u0 * 100.0, 1);
  rec("iters", 300, 0);
}

RC_TEST("when the bound on the ZMP changes with the support, the predictive controller recovers from pushes as large as a clipped fixed gain law does, and settles sooner") {
  const double kx = 3.0, kv = 0.9;
  RC_CHECK(recovers(false, 0.2, kx, kv) && recovers(true, 0.2, kx, kv));
  const double v_pd = largest(false, kx, kv), v_mpc = largest(true, kx, kv);
  const double v_limit = std::sqrt(9.81 / kH) * kBound;   // the capture point at the bound: x + v / w = bound
  rec("bound_cm", kBound * 100.0, 0);
  rec("h", kH, 2);
  rec("v_small", 0.2, 1);
  rec("kx", kx, 1);
  rec("kv", kv, 1);
  rec("v_pd", v_pd, 2);
  rec("v_mpc", v_mpc, 2);
  rec("v_limit", v_limit, 2);
  const double push = 0.6;
  const int sp = settle_steps(false, push, kx, kv), sm = settle_steps(true, push, kx, kv);
  rec("push", push, 1);
  rec("t_pd", sp * kDt, 2);
  rec("t_mpc", sm * kDt, 2);
  const int sp_small = settle_steps(false, 0.3, kx, kv), sm_small = settle_steps(true, 0.3, kx, kv);
  rec("t_pd_small", sp_small * kDt, 2);
  rec("t_mpc_small", sm_small * kDt, 2);
  RC_CHECK(v_mpc >= v_pd);
  RC_CHECK(v_mpc <= v_limit + 0.02);
  RC_CHECK(sm >= 0 && sp >= 0 && sm < sp);
  RC_CHECK(sp_small <= sm_small);   // for a small push the fixed gain law is as quiet or quieter
}

RC_TEST("each weight does its own work: a dear command is a small one, and the weights on position and velocity are not interchangeable") {
  double wide[15];
  for (int k = 0; k < 15; ++k) wide[k] = kBound;
  const double cheap = mpc_zmp(0.0, 0.3, kH, kDt, 15, wide, 1.0, 1.0, 0.01, 300, nullptr);
  const double dear = mpc_zmp(0.0, 0.3, kH, kDt, 15, wide, 1.0, 1.0, 1.0e6, 300, nullptr);
  RC_CHECK(cheap > 0.0 && dear > 0.0);
  RC_CHECK(dear < 0.1 * cheap);
  const double pos_only = mpc_zmp(0.02, 0.1, kH, kDt, 15, wide, 1.0, 0.0, 1.0, 300, nullptr);
  const double vel_only = mpc_zmp(0.02, 0.1, kH, kDt, 15, wide, 0.0, 1.0, 1.0, 300, nullptr);
  // held as measured: swapping the two weights inside the solver swaps these two numbers
  RC_CHECK_NEAR(pos_only, 0.0670151093, 1e-6);
  RC_CHECK_NEAR(vel_only, 0.0638202826, 1e-6);
}
