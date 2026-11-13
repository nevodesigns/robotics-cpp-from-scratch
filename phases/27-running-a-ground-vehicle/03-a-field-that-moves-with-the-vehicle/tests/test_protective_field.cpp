#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iostream>

#include "solution.hpp"
#include <iomanip>
#include <iostream>

static void rec(const char* key, double value, int digits = 3) {
  std::cout << "@" << key << " " << std::fixed << std::setprecision(digits) << value << std::endl;
  std::cout.unsetf(std::ios::floatfield);
}
[[maybe_unused]] static void recs(const char* key, const std::string& value) { std::cout << "@" << key << " " << value << std::endl; }


namespace {

const double kPi = 3.14159265358979323846;

// A vehicle driving straight at v0 towards a thing D metres ahead. Each tick the sensor reports the thing and the field is evaluated;
// when it first says stop, the brakes take hold after `real_latency` and then slow the vehicle at the deceleration.
// Returns the gap left between the vehicle and the thing (negative if it went through).
double approach(double v0, double distance, const FieldConfig& field, double real_latency) {
  const double dt = 0.0005;
  double x = 0.0, v = v0, t = 0.0, t_stop = -1.0;
  for (; t < 30.0; t += dt) {
    std::vector<FieldPoint> pts(1);
    pts[0].x = distance - x;
    pts[0].y = 0.0;
    if (t_stop < 0.0 && pts[0].x > 0.0 && evaluate_field(pts, v0, 0.0, field) == kStop) t_stop = t;
    if (t_stop >= 0.0 && t >= t_stop + real_latency) v = std::max(0.0, v - 2.0 * dt);
    x += v * dt;
    if (v == 0.0) break;
  }
  return distance - x;
}

}  // namespace

RC_TEST("the field is as long as the vehicle needs to stop, and it stops short of a thing by the margin") {
  FieldConfig field;
  const double v = 1.5;
  RC_CHECK_NEAR(field_length(v, field), 1.5 * 0.3 + 1.5 * 1.5 / 4.0 + 0.2, 1e-12);
  const double gap = approach(v, 12.0, field, field.latency);
  rec("gap_ok", gap, 2);
  RC_CHECK_NEAR(gap, field.margin, 0.01);
}

RC_TEST("a field worked out with no latency lets the vehicle through a thing") {
  FieldConfig wrong;
  wrong.latency = 0.0;
  const double gap = approach(1.5, 12.0, wrong, 0.3);
  rec("overrun_nolat", -gap, 2);
  RC_CHECK(gap < 0.0);
  RC_CHECK_NEAR(gap, 0.2 - 1.5 * 0.3, 0.01);
}

RC_TEST("on a turn the straight field misses a thing on the arc, and the swept field finds it") {
  FieldConfig field;
  const double R = 2.0, k = 1.0 / R, a = 40.0 * kPi / 180.0;
  std::vector<FieldPoint> pts(1);
  pts[0].x = R * std::sin(a);
  pts[0].y = R * (1.0 - std::cos(a));
  RC_CHECK(evaluate_field(pts, 1.8, 0.0, field) == kClear);
  RC_CHECK(evaluate_field(pts, 1.8, k, field) == kStop);
  // The same thing on the other side of the turn is not in the corridor of a turn to the left.
  pts[0].y = -pts[0].y;
  RC_CHECK(evaluate_field(pts, 1.8, k, field) == kClear);
  RC_CHECK(evaluate_field(pts, 1.8, -k, field) == kStop);
}

RC_TEST("points behind, beside or past the end of the field are not in it") {
  RC_CHECK(in_swept_field(1.0, 0.0, 2.0, 0.4, 0.0));
  RC_CHECK(!in_swept_field(-0.1, 0.0, 2.0, 0.4, 0.0));
  RC_CHECK(!in_swept_field(1.0, 0.5, 2.0, 0.4, 0.0));
  RC_CHECK(!in_swept_field(2.1, 0.0, 2.0, 0.4, 0.0));
  RC_CHECK(!in_swept_field(1.0, 0.0, 0.5, 0.4, 0.0));
}

RC_TEST("a thing in the warning field slows the vehicle to a creep and one in the stop field stops it") {
  FieldConfig field;
  std::vector<FieldPoint> pts(1);
  pts[0].y = 0.0;
  const double v = 1.5;
  const double stop = field_length(v, field);
  pts[0].x = 1.5 * stop;
  RC_CHECK(evaluate_field(pts, v, 0.0, field) == kWarn);
  RC_CHECK_NEAR(protective_speed(kWarn, v, field.creep), field.creep, 1e-12);
  RC_CHECK_NEAR(protective_speed(kWarn, 0.1, field.creep), 0.1, 1e-12);
  pts[0].x = 0.9 * stop;
  RC_CHECK(evaluate_field(pts, v, 0.0, field) == kStop);
  RC_CHECK_NEAR(protective_speed(kStop, v, field.creep), 0.0, 1e-12);
  pts[0].x = 3.0 * stop;
  RC_CHECK(evaluate_field(pts, v, 0.0, field) == kClear);
}

RC_TEST("a device with a few field sets uses the smallest that was designed for the speed, and refuses a speed it has no set for") {
  FieldConfig field;
  std::vector<double> design;
  design.push_back(0.5);
  design.push_back(1.0);
  design.push_back(1.5);
  design.push_back(2.0);
  RC_CHECK(select_field_set(0.4, design) == 0);
  RC_CHECK(select_field_set(1.1, design) == 2);
  RC_CHECK(select_field_set(2.0, design) == 3);
  RC_CHECK(select_field_set(2.1, design) == -1);
  const double waste = field_length(1.5, field) / field_length(1.1, field);
  rec("waste", waste, 2);
  RC_CHECK(waste > 1.2);
}
