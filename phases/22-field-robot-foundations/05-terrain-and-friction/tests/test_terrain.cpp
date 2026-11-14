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
    {"d_tarmac", 0.2501516544405045},
    {"d_gravel", 0.4160685681000229},
    {"d_ice", 1.8533963488091925},
    {"ratio", 7.409090909090908},
    {"drive_up", 2.0200942499908647},
    {"drive_cross", 4.022100000000001},
    {"stop_down", 0.7169347975103216},
    {"stop_flat", 0.4160685681000229},
    {"climb_gravel", 22.293629159694085},
    {"climb_ice", 5.1427645578842425},
    {"hold_grass", 19.290046219188735},
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

const double kPi = 3.14159265358979323846;

Cell on(const Surface& s, double grade_x = 0.0, double grade_y = 0.0) {
  Cell c;
  c.surface = s;
  c.grade_x = grade_x;
  c.grade_y = grade_y;
  return c;
}

}  // namespace

RC_TEST("the stopping distance on flat ground is v squared over twice g times (mu + crr), and the surface changes it several times over") {
  const double v = 2.0;
  const Surface all[] = {tarmac(), gravel(), grass(), sand(), ice()};
  double d[5];
  for (int i = 0; i < 5; ++i) {
    const Traction t = traction(on(all[i]), 0.0);
    d[i] = stopping_distance(v, t.brake);
    RC_CHECK_NEAR(d[i], v * v / (2.0 * 9.81 * (all[i].mu + all[i].crr)), 1e-9);
  }
  rec("d_tarmac", d[0]);
  rec("d_gravel", d[1]);
  rec("d_ice", d[4]);
  rec("ratio", d[4] / d[0], 1);
  RC_CHECK(d[4] > 7.0 * d[0]);
  RC_CHECK(d[0] < d[1] && d[1] < d[2] && d[2] < d[3] && d[3] < d[4]);
}

RC_TEST("a slope helps one way and hurts the other, and costs nothing across it") {
  const double grade = 0.2;
  const Cell hill = on(gravel(), grade, 0.0);
  const Traction up = traction(hill, 0.0);
  const Traction across = traction(hill, kPi / 2.0);
  const Traction down = traction(hill, kPi);
  const Traction flat = traction(on(gravel()), 0.0);
  rec("drive_up", up.drive);
  rec("drive_cross", across.drive);
  RC_CHECK(up.drive < across.drive);
  RC_CHECK_NEAR(across.drive, flat.drive, 1e-9);
  RC_CHECK(up.can_climb);
  const double alpha = std::atan(grade);
  RC_CHECK_NEAR(up.drive, 9.81 * (0.45 * std::cos(alpha) - std::sin(alpha) - 0.04 * std::cos(alpha)), 1e-9);

  const double stop_down = stopping_distance(2.0, down.brake);
  const double stop_flat = stopping_distance(2.0, flat.brake);
  rec("stop_down", stop_down);
  rec("stop_flat", stop_flat);
  RC_CHECK(stop_down > stop_flat);
  RC_CHECK(traction(hill, kPi).angle < 0.0);
}

RC_TEST("there is a steepest slope a surface can be climbed or held on") {
  const double gravel_deg = max_climb_angle(gravel()) * 180.0 / kPi;
  const double ice_deg = max_climb_angle(ice()) * 180.0 / kPi;
  rec("climb_gravel", gravel_deg, 1);
  rec("climb_ice", ice_deg, 1);
  RC_CHECK(gravel_deg > 20.0 && gravel_deg < 25.0);
  RC_CHECK(ice_deg < 6.0);

  const double just_under = std::tan(max_climb_angle(gravel()) - 0.01);
  const double just_over = std::tan(max_climb_angle(gravel()) + 0.01);
  RC_CHECK(traction(on(gravel(), just_under), 0.0).can_climb);
  RC_CHECK(!traction(on(gravel(), just_over), 0.0).can_climb);

  RC_CHECK(traction(on(grass(), 0.30), 0.0).holds);
  RC_CHECK(!traction(on(grass(), 0.40), 0.0).holds);
  rec("hold_grass", std::atan(0.35) * 180.0 / kPi, 1);
}

RC_TEST("patches override the base in the order they were added, and an edge belongs to one patch only") {
  Terrain world(on(tarmac()));
  Patch mud;
  mud.x0 = 5.0;
  mud.y0 = 0.0;
  mud.x1 = 10.0;
  mud.y1 = 10.0;
  mud.cell = on(sand());
  world.add(mud);
  Patch strip;
  strip.x0 = 7.0;
  strip.y0 = 0.0;
  strip.x1 = 8.0;
  strip.y1 = 10.0;
  strip.cell = on(ice());
  world.add(strip);
  RC_CHECK(std::string(world.at(1.0, 1.0).surface.name) == "tarmac");
  RC_CHECK(std::string(world.at(5.0, 1.0).surface.name) == "sand");
  RC_CHECK(std::string(world.at(7.5, 1.0).surface.name) == "ice");
  RC_CHECK(std::string(world.at(8.0, 1.0).surface.name) == "sand");
  RC_CHECK(std::string(world.at(10.0, 1.0).surface.name) == "tarmac");
}
