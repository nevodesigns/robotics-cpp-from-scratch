#include <rc/test/rc_test.hpp>
#include <sstream>
#include <cstring>

#include <cmath>
#include <iostream>

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
    {"kmh5", 1.3888888888888888},
    {"t_ramp", 1.9999999999998905},
    {"t_stop", 0.6670000000000005},
    {"d_stop", 0.33283350000000883},
    {"v_lat", 0.75},
    {"d_right", 0.6423097222222202},
    {"d_wrong", 8.330833499999706},
    {"ratio", 12.97011895005613},
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


namespace {

// Holds a wanted command and runs the shaper until the speed is reached (or a time passes), returning the time, and the distance travelled.
struct Run {
  double time = 0.0;
  double distance = 0.0;
  double v = 0.0;
};

Run drive_to(double want_v, double start_v, const VehicleLimits& lim, double stop_at_v) {
  Run r;
  r.v = start_v;
  double w = 0.0;
  const double dt = 0.001;
  for (int i = 0; i < 60000; ++i) {
    const Shaped s = shape(want_v, 0.0, r.v, w, dt, lim);
    r.v = s.v;
    r.distance += r.v * dt;
    r.time += dt;
    if (want_v > start_v ? r.v >= stop_at_v - 1e-9 : r.v <= stop_at_v + 1e-9) break;
  }
  return r;
}

}  // namespace

RC_TEST("a unit is part of the type, and the conversions are right") {
  rec("kmh5", Speed::from_kmh(5.0).mps);
  RC_CHECK_NEAR(Speed::from_kmh(5.0).mps, 1.3889, 1e-4);
  RC_CHECK_NEAR(Speed::from_mps(2.0).kmh(), 7.2, 1e-9);
  RC_CHECK_NEAR(Rate::from_dps(90.0).rps, 1.5707963, 1e-6);
}

RC_TEST("speed up slowly, brake harder, and the numbers are the ones the limits say") {
  VehicleLimits lim;
  const Run up = drive_to(1.0, 0.0, lim, 1.0);
  rec("t_ramp", up.time);
  RC_CHECK_NEAR(up.time, 2.0, 0.003);

  const Run down = drive_to(0.0, 1.0, lim, 0.0);
  rec("t_stop", down.time);
  rec("d_stop", down.distance);
  RC_CHECK_NEAR(down.time, 1.0 / 1.5, 0.003);
  RC_CHECK_NEAR(down.distance, 1.0 / (2.0 * 1.5), 0.002);
  RC_CHECK(down.time < up.time);
}

RC_TEST("a fast turn at speed is cut by speed times turn rate, and a straight line is not") {
  VehicleLimits lim;
  lim.max_alpha = 100.0;
  lim.max_accel = 100.0;
  const Shaped turn = shape(1.0, 2.0, 1.0, 2.0, 0.01, lim);
  rec("v_lat", turn.v);
  RC_CHECK(turn.limited_lateral);
  RC_CHECK_NEAR(turn.v, 0.75, 1e-9);
  RC_CHECK_NEAR(turn.v * turn.w, lim.max_lateral, 1e-9);

  const Shaped straight = shape(1.0, 0.0, 1.0, 0.0, 0.01, lim);
  RC_CHECK(!straight.limited_lateral);
  RC_CHECK_NEAR(straight.v, 1.0, 1e-9);

  const Shaped backwards = shape(-1.0, 2.0, -1.0, 2.0, 0.01, lim);
  RC_CHECK_NEAR(backwards.v, -0.75, 1e-9);
}

RC_TEST("a limit read in the wrong unit makes the stopping distance thirteen times as long") {
  VehicleLimits right;
  right.max_v = Speed::from_kmh(5.0);
  VehicleLimits wrong;
  wrong.max_v = Speed::from_mps(5.0);
  const Run a = drive_to(0.0, right.max_v.mps, right, 0.0);
  const Run b = drive_to(0.0, wrong.max_v.mps, wrong, 0.0);
  rec("d_right", a.distance);
  rec("d_wrong", b.distance);
  rec("ratio", b.distance / a.distance, 1);
  RC_CHECK_NEAR(a.distance, 1.3889 * 1.3889 / 3.0, 0.002);
  RC_CHECK_NEAR(b.distance, 25.0 / 3.0, 0.01);
  RC_CHECK(b.distance > 12.0 * a.distance);
}

RC_TEST("a command beyond the limits is flagged as clamped") {
  VehicleLimits lim;
  const Shaped s = shape(9.0, 9.0, 0.0, 0.0, 0.01, lim);
  RC_CHECK(s.limited_speed);
  RC_CHECK(s.v <= lim.max_v.mps);
  RC_CHECK(s.w <= lim.max_w.rps);
}
