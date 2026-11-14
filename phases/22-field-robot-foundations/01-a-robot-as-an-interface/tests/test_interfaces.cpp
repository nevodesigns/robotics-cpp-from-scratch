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
    {"vfinal", 0.9999999999999889},
    {"t63", 0.20000000000000015},
    {"wd_stop", 1.2649999999999715},
    {"coast", 0.1995002946112172},
    {"t_arm", 2.0000000000000013},
    {"t_leg", 0.5000000000000002},
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

// Steps a drive for a time, resending the same command every 0.1 s when asked to, so that the watchdog stays quiet.
void run(Drive& d, double seconds, const DriveCommand* resend, double dt = 0.001) {
  const int steps = static_cast<int>(seconds / dt + 0.5);
  for (int i = 0; i < steps; ++i) {
    if (resend != nullptr && i % 100 == 0) d.command(*resend);
    d.step(dt);
  }
}

}  // namespace

RC_TEST("a command is limited, answered with a lag of the time constant, and held") {
  FakeDrive d;
  DriveCommand fast;
  fast.v = 5.0;
  run(d, 10.0, &fast);
  rec("vfinal", d.state().v);
  RC_CHECK_NEAR(d.state().v, 1.0, 0.001);

  // A command that is not a number is a stop, not full speed.
  FakeDrive confused;
  DriveCommand nan_command;
  nan_command.v = std::nan("");
  confused.command(fast);
  confused.command(nan_command);
  run(confused, 3.0, nullptr);
  RC_CHECK_NEAR(confused.state().v, 0.0, 1e-3);

  FakeDrive step;
  DriveCommand one;
  one.v = 1.0;
  step.command(one);
  double t = 0.0;
  while (step.state().v < 0.632 && t < 5.0) {
    step.command(one);
    step.step(0.001);
    t += 0.001;
  }
  rec("t63", t);
  RC_CHECK_NEAR(t, 0.2, 0.01);
}

RC_TEST("a base that stops hearing commands stops itself, and a stop coasts for the time constant") {
  FakeDrive d;
  DriveCommand slow;
  slow.v = 0.5;
  d.command(slow);
  double t = 0.0;
  bool moving = false;
  while (t < 5.0) {
    d.step(0.001);
    t += 0.001;
    if (d.state().v > 0.3) moving = true;
    if (moving && d.state().v < 0.01) break;
  }
  rec("wd_stop", t);
  RC_CHECK(moving);
  RC_CHECK(t > 0.5);
  RC_CHECK(t < 2.0);

  FakeDrive e;
  DriveCommand one;
  one.v = 1.0;
  run(e, 3.0, &one);
  const double before = e.state().x;
  e.stop();
  run(e, 3.0, nullptr);
  const double coast = e.state().x - before;
  rec("coast", coast);
  RC_CHECK_NEAR(coast, 0.2, 0.005);
}

RC_TEST("the same program drives any Drive, and a turn command turns") {
  FakeDrive d;
  DriveCommand c;
  c.v = 0.5;
  c.w = 1.0;
  run(d, 6.0, &c);
  const DriveState s = d.state();
  RC_CHECK_NEAR(s.v, 0.5, 0.001);
  RC_CHECK_NEAR(s.w, 1.0, 0.001);
  RC_CHECK(s.theta > 5.0);
}

RC_TEST("joint buses move at their rate, refuse a wrong sized command, and keep arm and leg apart") {
  ArmBus arm(3, 0.5);
  RC_CHECK(!arm.set_targets({1.0, 2.0}));
  RC_CHECK(!arm.set_targets({1.0, std::nan(""), 0.0}));
  RC_CHECK(arm.set_targets({1.0, 0.0, -1.0}));
  double t = 0.0;
  while (!arm.at_target(1e-6) && t < 10.0) {
    arm.step(0.01);
    t += 0.01;
  }
  rec("t_arm", t);
  RC_CHECK_NEAR(t, 2.0, 0.02);

  LegBus legs(4, 3, 2.0);
  RC_CHECK(legs.leg_count() == 4);
  RC_CHECK(legs.joint_count() == 12);
  std::vector<double> all(12, 1.0);
  RC_CHECK(legs.set_targets(all));
  t = 0.0;
  while (!legs.at_target(1e-6) && t < 10.0) {
    legs.step(0.01);
    t += 0.01;
  }
  rec("t_leg", t);
  RC_CHECK_NEAR(t, 0.5, 0.02);
}
