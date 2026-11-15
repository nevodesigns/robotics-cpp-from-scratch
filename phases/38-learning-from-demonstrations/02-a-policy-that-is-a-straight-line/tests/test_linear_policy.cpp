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
    {"sat_narrow", 0.0},
    {"w0_narrow", -1.999999999999996},
    {"w1_narrow", -2.999999999999994},
    {"mse_narrow", 1.3149511660501115e-32},
    {"sat_wide", 8.366666666666667},
    {"w0_wide", -1.0440102099030812},
    {"w1_wide", -1.5762239375097413},
    {"mse_wide", 0.018158366788034258},
    {"ridge", 100.0},
    {"w0_ridge", -0.5917010822165621},
    {"w1_ridge", -0.6137803032004377},
    {"mse_ridge", 0.04677039165564232},
    {"mse_train", 0.002848177305982025},
    {"mse_test", 0.0031619001250110905},
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


RC_TEST("from demonstrations that never reach the limit, least squares finds the expert's own gains") {
  LaneTask t;
  const std::vector<Sample> d = demonstrations(t, 30, 100, 0.2, 0.1, 0.0, 0.0, 21);
  const LinearPolicy p = fit_linear(d, 0.0);
  rec("sat_narrow", saturated_share(t, d) * 100.0, 1);
  rec("w0_narrow", p.w[0], 3);
  rec("w1_narrow", p.w[1], 3);
  rec("mse_narrow", linear_mse(p, d), 6);
  RC_CHECK(saturated_share(t, d) == 0.0);
  RC_CHECK(std::fabs(p.w[0] + t.k_offset) < 1e-6);
  RC_CHECK(std::fabs(p.w[1] + t.k_heading) < 1e-6);
  RC_CHECK(std::fabs(p.b) < 1e-9);
}

RC_TEST("where the expert's turn was clipped, the line's gains shrink and an error is left that no line removes, and a ridge shrinks them further") {
  LaneTask t;
  const std::vector<Sample> d = demonstrations(t, 30, 100, 0.6, 0.8, 0.0, 0.0, 22);
  const LinearPolicy p = fit_linear(d, 0.0);
  rec("sat_wide", saturated_share(t, d) * 100.0, 1);
  rec("w0_wide", p.w[0], 3);
  rec("w1_wide", p.w[1], 3);
  rec("mse_wide", linear_mse(p, d), 5);
  RC_CHECK(saturated_share(t, d) > 0.05);
  RC_CHECK(std::fabs(p.w[0]) < t.k_offset && std::fabs(p.w[1]) < t.k_heading);
  RC_CHECK(linear_mse(p, d) > 1e-3);
  // the least squares line is the best line: moving any weight makes the error larger
  LinearPolicy q = p;
  q.w[0] *= 1.05;
  RC_CHECK(linear_mse(q, d) > linear_mse(p, d));
  q = p;
  q.w[1] *= 0.95;
  RC_CHECK(linear_mse(q, d) > linear_mse(p, d));
  const double ridge = 100.0;
  const LinearPolicy r = fit_linear(d, ridge);
  rec("ridge", ridge, 0);
  rec("w0_ridge", r.w[0], 3);
  rec("w1_ridge", r.w[1], 3);
  rec("mse_ridge", linear_mse(r, d), 5);
  RC_CHECK(std::fabs(r.w[0]) < std::fabs(p.w[0]) && std::fabs(r.w[1]) < std::fabs(p.w[1]));
  RC_CHECK(linear_mse(r, d) > linear_mse(p, d));
}

RC_TEST("on demonstrators with their own styles the line is tested on episodes it has not seen, and an empty set gives the zero policy") {
  LaneTask t;
  const std::vector<Sample> d = demonstrations(t, 40, 100, 0.6, 0.4, 0.1, 0.3, 1);
  std::vector<Sample> train, test;
  split_by_episode(d, 5, train, test);
  const LinearPolicy p = fit_linear(train, 0.0);
  rec("mse_train", linear_mse(p, train), 5);
  rec("mse_test", linear_mse(p, test), 5);
  RC_CHECK(linear_mse(p, test) > 0.0 && linear_mse(p, train) > 0.0);
  const LinearPolicy z = fit_linear(std::vector<Sample>(), 0.0);
  RC_CHECK(z.w[0] == 0.0 && z.w[1] == 0.0 && z.b == 0.0);
  // a set in which only one state was ever seen cannot fix three numbers
  std::vector<Sample> one(10);
  const LinearPolicy s = fit_linear(one, 0.0);
  RC_CHECK(s.w[0] == 0.0 && s.b == 0.0);
}
