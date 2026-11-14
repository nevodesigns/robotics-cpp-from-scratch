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
    {"v_noload", 1.8325957145940457},
    {"v_flat", 1.6695902868497559},
    {"v_slope", 0.9949465621175693},
    {"climb_motor", 25.57208171689462},
    {"climb_grip", 22.293629159694085},
    {"climb_tarmac", 38.131913675601716},
    {"a_good", 4.54269728880722},
    {"a_sag", 3.317287219652093},
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

const double kPi = 3.14159265358979323846;

rc::field::Cell on(const rc::field::Surface& s, double grade = 0.0) {
  rc::field::Cell c;
  c.surface = s;
  c.grade_x = grade;
  return c;
}

const char* name(Limit l) { return l == Limit::motors ? "motors" : "grip"; }

}  // namespace

RC_TEST("the force falls in a straight line with speed from the stall force to nothing at the no load speed") {
  DriveTrain d;
  d.battery_resistance = 0.0;
  const double stall = drive_force(d, 0.0);
  RC_CHECK_NEAR(stall, 4.0 * 0.7 * 0.85 / 0.05, 0.05 * stall);
  const double w_nl = 350.0 * 2.0 * kPi / 60.0 * 0.05;
  rec("v_noload", w_nl, 2);
  RC_CHECK(drive_force(d, 0.5 * w_nl) < stall && drive_force(d, 0.5 * w_nl) > 0.0);
  RC_CHECK_NEAR(drive_force(d, 0.5 * w_nl), 0.5 * stall, 0.03 * stall);
  RC_CHECK(drive_force(d, w_nl) < 0.02 * stall);
}

RC_TEST("the top speed on the level is a little under the no load speed, and a slope takes much of it") {
  DriveTrain d;
  const rc::field::Cell gravel = on(rc::field::gravel());
  const double flat = top_speed(d, gravel, 0.0);
  const double slope = top_speed(d, on(rc::field::gravel(), std::tan(10.0 * kPi / 180.0)), 0.0);
  rec("v_flat", flat, 2);
  rec("v_slope", slope, 2);
  RC_CHECK(flat < 350.0 * 2.0 * kPi / 60.0 * 0.05);
  RC_CHECK(flat > 0.8 * 350.0 * 2.0 * kPi / 60.0 * 0.05);
  RC_CHECK(slope < 0.8 * flat);
  RC_CHECK(slope > 0.0);
  // At the top speed the force and the resistance are equal.
  RC_CHECK_NEAR(drive_force(d, flat), resistance(d, gravel, 0.0, flat), 1e-6);
  // A slope too steep for the motors gives a top speed of zero.
  RC_CHECK(top_speed(d, on(rc::field::tarmac(), std::tan(50.0 * kPi / 180.0)), 0.0) == 0.0);
}

RC_TEST("the motors climb a steeper slope than gravel will hold, and a lesser one than tarmac will") {
  DriveTrain d;
  const double motors = max_climb(d, 0.04) * 180.0 / kPi;
  const double gravel = rc::field::max_climb_angle(rc::field::gravel()) * 180.0 / kPi;
  const double tarmac = rc::field::max_climb_angle(rc::field::tarmac()) * 180.0 / kPi;
  rec("climb_motor", motors, 1);
  rec("climb_grip", gravel, 1);
  rec("climb_tarmac", tarmac, 1);
  RC_CHECK(motors > gravel);
  RC_CHECK(motors < tarmac);
  RC_CHECK(max_climb(d, 0.04) > 0.4);
}

RC_TEST("a battery with a large internal resistance sags under the load of a standing start and takes the acceleration with it") {
  DriveTrain good;
  good.battery_resistance = 0.02;
  DriveTrain sagging = good;
  sagging.battery_resistance = 0.5;
  const rc::field::Cell tarmac = on(rc::field::tarmac());
  const double a_good = max_accel_motors(good, tarmac, 0.0, 0.0);
  const double a_sag = max_accel_motors(sagging, tarmac, 0.0, 0.0);
  rec("a_good", a_good, 2);
  rec("a_sag", a_sag, 2);
  RC_CHECK(bus_voltage(sagging, 0.0) < 0.75 * bus_voltage(good, 0.0));
  RC_CHECK(a_sag < 0.8 * a_good);
  // The voltage rises again as the vehicle speeds up and the motors draw less.
  RC_CHECK(bus_voltage(sagging, 1.0) > bus_voltage(sagging, 0.0));
}

RC_TEST("on tarmac the motors are the limit, and on gravel and ice the grip is") {
  DriveTrain d;
  const Achievable t = achievable_accel(d, on(rc::field::tarmac()), 0.0, 0.0);
  const Achievable g = achievable_accel(d, on(rc::field::gravel()), 0.0, 0.0);
  const Achievable i = achievable_accel(d, on(rc::field::ice()), 0.0, 0.0);
  std::cout << "  tarmac " << t.accel << " m/s2 by the " << name(t.binding) << ", gravel " << g.accel << " by the " << name(g.binding) << ", ice " << i.accel << " by the " << name(i.binding) << "\n";
  recs("lim_tarmac", name(t.binding));
  recs("lim_gravel", name(g.binding));
  recs("lim_ice", name(i.binding));
  RC_CHECK(t.binding == Limit::motors);
  RC_CHECK(g.binding == Limit::grip);
  RC_CHECK(i.binding == Limit::grip);
  RC_CHECK(i.accel < g.accel && g.accel < t.accel);
}
