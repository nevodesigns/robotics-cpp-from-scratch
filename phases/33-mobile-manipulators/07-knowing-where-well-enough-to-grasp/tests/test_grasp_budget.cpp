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
    {"s_pos_mm", 20.0},
    {"s_head_deg", 1.0},
    {"s_mount_deg", 0.2},
    {"s_arm_mm", 0.1},
    {"sigma_mm", 25.623807777143508},
    {"lever", 0.9},
    {"tol_mm", 10.0},
    {"p_blind", 0.3036572246060789},
    {"n_trials", 20000.0},
    {"p_sim", 0.29785},
    {"z95", 1.9599639845400536},
    {"z99", 2.5758293035488986},
    {"need_mm", 3.8822448312946465},
    {"s_cam_mm", 2.0},
    {"sigma_servo_mm", 2.0024984394500787},
    {"p_servo", 0.9999994078554374},
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

unsigned long long g_state = 31;
double uni() {
  g_state = g_state * 6364136223846793005ULL + 1442695040888963407ULL;
  return static_cast<double>(g_state >> 11) / 9007199254740992.0;
}
double gauss() {
  double s = 0.0;
  for (int i = 0; i < 12; ++i) s += uni();
  return s - 6.0;
}

}  // namespace

RC_TEST("the errors add in quadrature, and the heading's is multiplied by the lever") {
  Sigmas s;
  const double lever = 0.9;
  const double sg = hand_sigma(s, lever);
  rec("s_pos_mm", s.pos * 1000.0, 0);
  rec("s_head_deg", 1, 0);
  rec("s_mount_deg", 0.2, 1);
  rec("s_arm_mm", s.arm * 1000.0, 1);
  rec("sigma_mm", sg * 1000.0, 1);
  rec("lever", lever, 1);
  const double expect = std::sqrt(0.02 * 0.02 + std::pow(0.9 * std::sqrt(0.01745 * 0.01745 + 0.0035 * 0.0035), 2) + 0.0001 * 0.0001);
  RC_CHECK_NEAR(sg, expect, 1e-12);
  Sigmas no_head = s;
  no_head.heading = 0.0;
  no_head.mount = 0.0;
  RC_CHECK_NEAR(hand_sigma(no_head, 5.0), std::sqrt(0.02 * 0.02 + 0.0001 * 0.0001), 1e-12);
  RC_CHECK(hand_sigma(s, 1.2) > hand_sigma(s, 0.5));
}

RC_TEST("the chance of a good grasp follows the error function, and simulated grasps agree") {
  Sigmas s;
  const double sg = hand_sigma(s, 0.9);
  const double tol = 0.010;
  const double p = grasp_probability(sg, tol);
  int ok = 0;
  const int n = 20000;
  g_state = 31;
  for (int i = 0; i < n; ++i)
    if (std::fabs(sg * gauss()) < tol) ++ok;
  rec("tol_mm", tol * 1000.0, 0);
  rec("p_blind", p, 2);
  rec("n_trials", n, 0);
  rec("p_sim", static_cast<double>(ok) / n, 2);
  RC_CHECK_NEAR(static_cast<double>(ok) / n, p, 0.02);
  RC_CHECK_NEAR(grasp_probability(1.0, 1.0), 0.6826894921370859, 1e-9);
  RC_CHECK_NEAR(grasp_probability(0.0, 0.001), 1.0, 1e-12);
  RC_CHECK(p < 0.5);
}

RC_TEST("a wanted chance needs a number of sigmas, and so a sigma at the hand") {
  rec("z95", z_for_probability(0.95), 2);
  rec("z99", z_for_probability(0.99), 2);
  RC_CHECK_NEAR(z_for_probability(0.95), 1.95996, 1e-4);
  RC_CHECK_NEAR(z_for_probability(0.99), 2.57583, 1e-4);
  const double tol = 0.010;
  const double need = sigma_needed(tol, 0.99);
  rec("need_mm", need * 1000.0, 1);
  RC_CHECK_NEAR(grasp_probability(need, tol), 0.99, 1e-6);
  Sigmas s;
  RC_CHECK(hand_sigma(s, 0.9) > 3.0 * need);
}

RC_TEST("a camera on the hand takes the base's errors out of the budget") {
  const double cam = 0.002, arm = 0.0001, tol = 0.010;
  const double sg = sigma_after_servo(cam, arm);
  rec("s_cam_mm", cam * 1000.0, 0);
  rec("sigma_servo_mm", sg * 1000.0, 1);
  rec("p_servo", grasp_probability(sg, tol), 3);
  Sigmas s;
  RC_CHECK(sg < 0.1 * hand_sigma(s, 0.9));
  RC_CHECK(grasp_probability(sg, tol) > 0.99);
  RC_CHECK_NEAR(sigma_after_servo(0.0, 0.0), 0.0, 1e-12);
}
