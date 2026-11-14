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
    {"drive_straight", 4.0221},
    {"brake_straight", 4.806900000000001},
    {"share", 68.69056548821132},
    {"drive_turn", 2.815822867763497},
    {"brake_turn", 3.6006228677634966},
    {"stop_straight", 0.23403856955626284},
    {"stop_turn", 0.3124459409709816},
    {"v_slide", 1.8098505463159107},
    {"rest", 19.850438282315036},
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

rc::field::Cell on(const rc::field::Surface& s, double grade = 0.0) {
  rc::field::Cell c;
  c.surface = s;
  c.grade_x = grade;
  return c;
}

}  // namespace

RC_TEST("on a straight line the budget is the whole grip, less the rolling resistance") {
  const rc::field::Cell gravel = on(rc::field::gravel());
  const TractionBudget b = traction_budget(gravel, 0.0, 1.5, 0.0);
  rec("drive_straight", b.drive, 2);
  rec("brake_straight", b.brake, 2);
  RC_CHECK_NEAR(b.share, 0.0, 1e-12);
  RC_CHECK_NEAR(b.drive, 9.81 * (0.45 - 0.04), 1e-9);
  RC_CHECK_NEAR(b.brake, 9.81 * (0.45 + 0.04), 1e-9);
  RC_CHECK(!b.slides);
}

RC_TEST("a turn takes a share of the grip and the rest is what is left to speed up and to brake with") {
  const rc::field::Cell gravel = on(rc::field::gravel());
  const double kappa = 1.0 / 0.742;
  const TractionBudget turn = traction_budget(gravel, 0.0, 1.5, kappa);
  const TractionBudget straight = traction_budget(gravel, 0.0, 1.5, 0.0);
  rec("share", turn.share * 100.0, 0);
  rec("drive_turn", turn.drive, 2);
  rec("brake_turn", turn.brake, 2);
  const double grip = 0.45 * 9.81;
  RC_CHECK_NEAR(turn.lateral, 2.25 * kappa, 1e-12);
  RC_CHECK_NEAR(turn.share, 2.25 * kappa / grip, 1e-12);
  RC_CHECK_NEAR(turn.brake, grip * std::sqrt(1.0 - turn.share * turn.share) + 9.81 * 0.04, 1e-9);
  RC_CHECK(turn.drive < straight.drive);
  RC_CHECK(turn.brake < straight.brake);
  // The sum of the squares of the two accelerations used is the square of the grip, at the edge of what is allowed.
  const double longitudinal = turn.brake - 9.81 * 0.04;
  RC_CHECK_NEAR(longitudinal * longitudinal + turn.lateral * turn.lateral, grip * grip, 1e-9);
  const double stop_straight = 1.5 * 1.5 / (2.0 * straight.brake);
  const double stop_turn = 1.5 * 1.5 / (2.0 * turn.brake);
  rec("stop_straight", stop_straight, 2);
  rec("stop_turn", stop_turn, 2);
  RC_CHECK(stop_turn > stop_straight);
}

RC_TEST("past the edge the tyres let go, and the speed at which that happens is the one the limit gives") {
  const rc::field::Cell gravel = on(rc::field::gravel());
  const double kappa = 1.0 / 0.742;
  const double vmax = max_speed_for_curvature(gravel, 0.0, kappa);
  rec("v_slide", vmax, 2);
  RC_CHECK_NEAR(vmax, std::sqrt(0.45 * 9.81 / kappa), 1e-12);
  RC_CHECK(!traction_budget(gravel, 0.0, vmax * 0.99, kappa).slides);
  RC_CHECK(traction_budget(gravel, 0.0, vmax * 1.01, kappa).slides);
  const TractionBudget over = traction_budget(gravel, 0.0, vmax * 1.2, kappa);
  RC_CHECK(over.slides);
  RC_CHECK(over.drive < 0.0);
  RC_CHECK_NEAR(max_curvature_at_speed(gravel, 0.0, vmax), kappa, 1e-12);
  // Ice lets go at a lower speed on the same circle.
  RC_CHECK(max_speed_for_curvature(on(rc::field::ice()), 0.0, kappa) < vmax);
}

RC_TEST("near the edge there is almost nothing left to brake with, and a slope takes its share first") {
  const rc::field::Cell gravel = on(rc::field::gravel());
  const double kappa = 1.0 / 0.742;
  const double v99 = 0.99 * max_speed_for_curvature(gravel, 0.0, kappa);
  // At 99 percent of the speed the share is 98 percent of the grip.
  const TractionBudget edge = traction_budget(gravel, 0.0, v99, kappa);
  const double straight = traction_budget(gravel, 0.0, v99, 0.0).brake;
  const double rest = (edge.brake - 9.81 * 0.04) / (straight - 9.81 * 0.04) * 100.0;
  rec("rest", rest, 0);
  RC_CHECK(rest < 25.0);

  // Up a slope (heading along +x, grade 0.2 rising) the same turn leaves less to speed up with and more to brake with.
  const rc::field::Cell hill = on(rc::field::gravel(), 0.2);
  const TractionBudget flat_turn = traction_budget(gravel, 0.0, 1.0, kappa);
  const TractionBudget hill_turn = traction_budget(hill, 0.0, 1.0, kappa);
  RC_CHECK(hill_turn.drive < flat_turn.drive);
  RC_CHECK(hill_turn.brake > flat_turn.brake - 0.5);
  RC_CHECK_NEAR(hill_turn.brake - hill_turn.drive, 2.0 * (9.81 * std::sin(std::atan(0.2)) + 9.81 * 0.04 * std::cos(std::atan(0.2))), 1e-9);
}
