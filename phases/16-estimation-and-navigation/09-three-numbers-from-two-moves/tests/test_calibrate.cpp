#include <rc/test/rc_test.hpp>

#include <rc/nav/odometry.hpp>

#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>

#include "solution.hpp"

namespace {

const double kBase = 0.4;            // the wheel base on the drawing, metres
const double kPi = 3.141592653589793;

struct Rng {
  std::uint32_t state;
  std::uint32_t next() {
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
  }
  double uniform() { return static_cast<double>(next() >> 8) / 16777216.0; }
  double gaussian() {
    double sum = 0.0;
    for (int i = 0; i < 12; ++i) sum += uniform();
    return sum - 6.0;
  }
};

// The robot as it really is: wheels that travel a little more or less than the encoders say,
// a wheel base that is not the drawing's. Moved in a thousand small pieces so that the way it
// is integrated is not what is being measured, and heading is kept unwrapped so a spin of four
// turns is four turns.
struct TrueRobot {
  double left_scale;
  double right_scale;
  double base;
  double x = 0.0;
  double y = 0.0;
  double theta = 0.0;

  void move(double left, double right) {
    const int pieces = 1000;
    for (int i = 0; i < pieces; ++i) {
      const double dl = left_scale * left / pieces;
      const double dr = right_scale * right / pieces;
      const double forward = 0.5 * (dl + dr);
      const double turn = (dr - dl) / base;
      x += forward * std::cos(theta + 0.5 * turn);
      y += forward * std::sin(theta + 0.5 * turn);
      theta += turn;
    }
  }
};

// The two moves, done on a robot with the given errors and measured from outside with the given
// noise: a tape to a few millimetres and a protractor to a fifth of a degree.
struct Measurements {
  StraightTest straight;
  SpinTest spin;
};

Measurements run_tests(double left_scale, double right_scale, double base_scale, double distance_noise,
                       double angle_noise, Rng& rng) {
  const double straight_distance = 5.0;
  const double spin_travel = 4.0 * kPi * kBase / 2.0 * 2.0;   // four full turns: wheels travel 2 pi base each per turn

  Measurements m;

  TrueRobot a{left_scale, right_scale, base_scale * kBase};
  a.move(straight_distance, straight_distance);
  m.straight.nominal_distance = straight_distance;
  m.straight.measured_distance = std::hypot(a.x, a.y) + distance_noise * rng.gaussian();
  m.straight.heading_change = a.theta + angle_noise * rng.gaussian();

  TrueRobot b{left_scale, right_scale, base_scale * kBase};
  b.move(-spin_travel / 4.0, spin_travel / 4.0);
  m.spin.nominal_travel = spin_travel / 4.0;
  m.spin.measured_rotation = b.theta + angle_noise * rng.gaussian();
  return m;
}

struct Drift {
  double position_cm = 0.0;
  double heading_deg = 0.0;
};

// A square four metres on a side, driven counter clockwise: straight, spin ninety degrees,
// four times. The odometry is fed what the encoders report, and the true robot the same.
Drift drive_square(double left_scale, double right_scale, double base_scale, const WheelErrors* calibration) {
  TrueRobot truth{left_scale, right_scale, base_scale * kBase};

  const double wheel_base = calibration != nullptr ? calibration->wheelbase_scale * kBase : kBase;
  rc::nav::Odometry odometry(wheel_base, rc::sim::Pose{0.0, 0.0, 0.0});

  const double turn_travel = (kPi / 2.0) * kBase / 2.0;   // each wheel, opposite ways, for ninety degrees
  const auto drive = [&](double left, double right) {
    truth.move(left, right);
    const int pieces = 200;
    for (int i = 0; i < pieces; ++i) {
      const double l = left / pieces;
      const double r = right / pieces;
      if (calibration != nullptr) {
        const WheelTravel t = corrected(*calibration, l, r);
        odometry.update(t.left, t.right);
      } else {
        odometry.update(l, r);
      }
    }
  };

  for (int side = 0; side < 4; ++side) {
    drive(4.0, 4.0);
    drive(-turn_travel, turn_travel);
  }

  Drift d;
  d.position_cm = 100.0 * std::hypot(truth.x - odometry.pose().x, truth.y - odometry.pose().y);
  double heading_error = truth.theta - odometry.pose().theta;
  heading_error = std::atan2(std::sin(heading_error), std::cos(heading_error));
  d.heading_deg = std::fabs(heading_error) * 180.0 / kPi;
  return d;
}

}  // namespace

RC_TEST("with exact measurements the three numbers come back exactly") {
  Rng rng{1};
  const double sets[][3] = {{1.012, 0.994, 1.020}, {0.97, 1.03, 0.98}, {1.0, 1.0, 1.0}, {1.05, 1.05, 1.0}, {0.99, 1.01, 1.10}};
  for (const auto& s : sets) {
    const Measurements m = run_tests(s[0], s[1], s[2], 0.0, 0.0, rng);
    const WheelErrors e = calibrate(kBase, m.straight, m.spin);
    RC_REQUIRE(e.valid);
    // To the accuracy of the simulated robot, which is moved in a thousand pieces and so is
    // exact to about a part in a million, not to the last digit.
    RC_CHECK_NEAR(e.left_scale, s[0], 1e-5);
    RC_CHECK_NEAR(e.right_scale, s[1], 1e-5);
    RC_CHECK_NEAR(e.wheelbase_scale, s[2], 1e-5);
  }
}

RC_TEST("a move of nothing says nothing, and nothing is divided by it") {
  StraightTest none_straight;
  SpinTest none_spin;
  RC_CHECK(!calibrate(kBase, none_straight, none_spin).valid);

  Rng rng{2};
  const Measurements m = run_tests(1.01, 0.99, 1.0, 0.0, 0.0, rng);
  RC_CHECK(calibrate(kBase, m.straight, m.spin).valid);
  RC_CHECK(!calibrate(0.0, m.straight, m.spin).valid);
  RC_CHECK(!calibrate(-1.0, m.straight, m.spin).valid);

  SpinTest no_rotation = m.spin;
  no_rotation.measured_rotation = 0.0;
  const WheelErrors e = calibrate(kBase, m.straight, no_rotation);
  RC_CHECK(!e.valid);
  RC_CHECK_NEAR(e.left_scale, 1.0, 1e-15);   // the defaults, not infinity or a number from nowhere
}

RC_TEST("the corrected travel is each wheel's own scale") {
  WheelErrors e;
  e.left_scale = 1.02;
  e.right_scale = 0.98;
  const WheelTravel t = corrected(e, 1.0, 2.0);
  RC_CHECK_NEAR(t.left, 1.02, 1e-12);
  RC_CHECK_NEAR(t.right, 1.96, 1e-12);
}

RC_TEST("which error costs what: each one alone, on a square four metres on a side") {
  struct Case {
    const char* name;
    double left;
    double right;
    double base;
  };
  const Case cases[] = {{"left wheel 1.2 % big", 1.012, 1.0, 1.0},
                        {"right wheel 0.6 % small", 1.0, 0.994, 1.0},
                        {"wheel base 2 % wider", 1.0, 1.0, 1.02},
                        {"both wheels 1 % big", 1.01, 1.01, 1.0},
                        {"all of them together", 1.012, 0.994, 1.02}};

  std::cout << "  a robot told to drive a square four metres on a side and finish where it started, with the\n"
            << "  odometry believing the drawing. How far from where it thinks it is it ends up:\n\n"
            << "    the real robot                       position      heading\n";
  Drift together;
  Drift base_only;
  Drift both_big;
  for (const Case& c : cases) {
    const Drift d = drive_square(c.left, c.right, c.base, nullptr);
    std::cout << "    " << std::left << std::setw(34) << c.name << std::right << std::fixed << std::setprecision(1)
              << std::setw(8) << d.position_cm << " cm" << std::setw(9) << d.heading_deg << " deg\n";
    if (std::string(c.name) == "all of them together") together = d;
    if (std::string(c.name) == "wheel base 2 % wider") base_only = d;
    if (std::string(c.name) == "both wheels 1 % big") both_big = d;
  }
  std::cout << "\n";

  // Four turns that are each two percent short are seven degrees off by the end.
  RC_CHECK(base_only.heading_deg > 6.0);
  RC_CHECK(base_only.position_cm > 10.0);

  // Two wheels that are equally big carry the robot one percent further on every straight and
  // turn it one percent further on every spin: four ninety degree turns that are each 0.9 degrees
  // long are 3.6 degrees round by the end.
  RC_CHECK(both_big.heading_deg > 3.0 && both_big.heading_deg < 4.0);

  // Together it is a robot that is two and a half metres from where it thinks it is after sixteen.
  RC_CHECK(together.position_cm > 20.0);

  // And with the three numbers found, from exact measurements of the two moves, it closes to
  // within a centimetre, which is as good as the simulation is.
  Rng rng{5};
  const Measurements exact = run_tests(1.012, 0.994, 1.02, 0.0, 0.0, rng);
  const WheelErrors found = calibrate(kBase, exact.straight, exact.spin);
  RC_REQUIRE(found.valid);
  const Drift fixed = drive_square(1.012, 0.994, 1.02, &found);
  std::cout << "  with the three found from exact measurements of a drive and a spin: " << std::fixed
            << std::setprecision(2) << fixed.position_cm << " cm, " << fixed.heading_deg << " deg.\n\n";
  RC_CHECK(fixed.position_cm < 1.0);
}

RC_TEST("calibrated from two moves, the square closes to within centimetres") {
  const double left = 1.012;
  const double right = 0.994;
  const double base = 1.02;

  // Twenty calibrations with the measurement noise of a tape and a protractor, to see what the
  // noise does to the three numbers.
  double sum_left = 0.0, sum_right = 0.0, sum_base = 0.0;
  double worst_left = 0.0, worst_right = 0.0, worst_base = 0.0;
  WheelErrors representative;
  for (int trial = 0; trial < 20; ++trial) {
    Rng rng{static_cast<std::uint32_t>(100 + trial)};
    const Measurements m = run_tests(left, right, base, 0.003, 0.2 * kPi / 180.0, rng);
    const WheelErrors e = calibrate(kBase, m.straight, m.spin);
    RC_REQUIRE(e.valid);
    sum_left += (e.left_scale - left) * (e.left_scale - left);
    sum_right += (e.right_scale - right) * (e.right_scale - right);
    sum_base += (e.wheelbase_scale - base) * (e.wheelbase_scale - base);
    worst_left = std::fmax(worst_left, std::fabs(e.left_scale - left));
    worst_right = std::fmax(worst_right, std::fabs(e.right_scale - right));
    worst_base = std::fmax(worst_base, std::fabs(e.wheelbase_scale - base));
    if (trial == 0) representative = e;
  }

  const Drift before = drive_square(left, right, base, nullptr);
  const Drift after = drive_square(left, right, base, &representative);

  std::cout << "  twenty calibrations with a tape good to 3 mm and a protractor good to 0.2 degrees, against a\n"
            << "  robot whose left wheel is 1.2 % big, right 0.6 % small and wheel base 2 % wide:\n\n"
            << "    scale             rms error     worst\n"
            << std::fixed << std::setprecision(4) << "    left wheel     " << std::setw(10) << std::sqrt(sum_left / 20.0)
            << std::setw(11) << worst_left << "\n"
            << "    right wheel    " << std::setw(10) << std::sqrt(sum_right / 20.0) << std::setw(11) << worst_right << "\n"
            << "    wheel base     " << std::setw(10) << std::sqrt(sum_base / 20.0) << std::setw(11) << worst_base << "\n\n"
            << "  and the square, driven with the odometry believing the drawing and then believing one of those:\n\n"
            << std::setprecision(1) << "    believing the drawing   " << std::setw(7) << before.position_cm << " cm, "
            << std::setw(5) << before.heading_deg << " deg\n"
            << "    calibrated              " << std::setw(7) << after.position_cm << " cm, " << std::setw(5)
            << after.heading_deg << " deg\n\n";

  RC_CHECK(worst_left < 0.004);
  RC_CHECK(worst_right < 0.004);
  RC_CHECK(worst_base < 0.004);

  RC_CHECK(before.position_cm > 20.0);
  RC_CHECK(after.position_cm < 5.0);
  RC_CHECK(after.position_cm * 6.0 < before.position_cm);
  RC_CHECK(after.heading_deg < 1.0);
}
