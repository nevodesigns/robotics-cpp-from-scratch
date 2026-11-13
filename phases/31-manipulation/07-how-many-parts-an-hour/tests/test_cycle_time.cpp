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


RC_TEST("the cycle is the sum of its pieces, and slower speed overrides make it longer") {
  CycleParams p;
  const double t1 = cycle_time(p, 1.0), th = cycle_time(p, 0.5), tq = cycle_time(p, 0.25);
  const double dwell = p.close_s + p.open_s + 2.0 * p.settle_s;
  const double slow = 2.0 * trapezoid_time(p.d_app, p.v_contact, p.a);
  rec("t1", t1, 2);
  rec("pph1", throughput(t1), 0);
  rec("t_half", th, 2);
  rec("t_q", tq, 2);
  rec("dwell", dwell, 1);
  rec("slow", slow, 2);
  RC_CHECK_NEAR(dwell, 0.8, 1e-12);
  RC_CHECK(t1 < th && th < tq);
  RC_CHECK(t1 > dwell + slow);
  RC_CHECK(th < 2.0 * t1);  // the hand's time and the contact speed do not stretch
  RC_CHECK_NEAR(throughput(2.0), 1800.0, 1e-9);
}

RC_TEST("the override for a target cycle is found, and a target faster than the rated speed is refused") {
  CycleParams p;
  const double t1 = cycle_time(p, 1.0);
  const double target = 1.4 * t1;
  const double k = scale_for_cycle(p, target);
  rec("target", target, 2);
  rec("k_t", k, 3);
  RC_CHECK(k > 0.0 && k < 1.0);
  RC_CHECK_NEAR(cycle_time(p, k), target, 1e-6);
  const double bad = 0.8 * t1;
  rec("target_bad", bad, 2);
  rec("k_bad", scale_for_cycle(p, bad), 0);
  RC_CHECK_NEAR(scale_for_cycle(p, bad), -1.0, 1e-12);
  RC_CHECK_NEAR(scale_for_cycle(p, t1), 1.0, 1e-6);
}

RC_TEST("a person near the robot caps its speed and lengthens the cycle, and one too close stops it") {
  CycleParams p;
  SsmParams s;
  const double free_t = cycle_time(p, 1.0);
  const double near = cycle_with_person(p, s, 1.0), far = cycle_with_person(p, s, 2.0);
  rec("d_near", 1.0, 1);
  rec("d_far", 2.0, 1);
  rec("v_near", max_speed(s, 1.0), 2);
  rec("v_far", max_speed(s, 2.0), 2);
  rec("t_near", near, 2);
  rec("t_far", far, 2);
  RC_CHECK(near > far);
  RC_CHECK(far >= free_t - 1e-12);
  RC_CHECK_NEAR(cycle_with_person(p, s, 0.2), -1.0, 1e-12);
  RC_CHECK_NEAR(cycle_with_person(p, s, 100.0), free_t, 1e-9);
}
