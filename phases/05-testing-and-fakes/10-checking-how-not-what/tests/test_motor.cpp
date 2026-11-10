#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "solution.hpp"

namespace {

// A port that only writes down what it was asked to do.
class RecordingPort : public MotorPort {
 public:
  void enable() override { calls.push_back("enable"); }
  void set_speed(double speed) override { calls.push_back("set " + std::to_string(static_cast<int>(std::lround(speed * 100)))); }
  void disable() override { calls.push_back("disable"); }
  std::vector<std::string> calls;
};

// The code under test: bring a motor up to a speed in a number of steps, then leave it there.
struct Style {
  int steps = 4;
  bool enable_twice = false;       // harmless: a second enable changes nothing
  bool repeat_final = false;       // harmless: setting the final speed again changes nothing
  bool disable_first = false;      // harmless: start from a known state
  bool forget_enable = false;      // a bug: the speed is set to a driver that is not enabled
  bool overshoot = false;          // a bug: the last step goes past the target and the limit
  bool stop_short = false;         // a bug: it ends one step short of the target
};

void bring_up(MotorPort& port, double target, const Style& s) {
  if (s.disable_first) port.disable();
  if (!s.forget_enable) port.enable();
  if (s.enable_twice) port.enable();
  const int steps = s.stop_short ? s.steps - 1 : s.steps;
  for (int i = 1; i <= steps; ++i) {
    double v = target * i / s.steps;
    if (s.overshoot && i == s.steps) v = target * 1.5;
    port.set_speed(v);
  }
  if (s.repeat_final) port.set_speed(target);
}

// The first style that worked, which the interaction test was written against.
const char* kExpected[] = {"enable", "set 25", "set 50", "set 75", "set 100"};

bool interaction_test_passes(const Style& s) {
  RecordingPort port;
  bring_up(port, 1.0, s);
  if (port.calls.size() != sizeof(kExpected) / sizeof(kExpected[0])) return false;
  for (std::size_t i = 0; i < port.calls.size(); ++i) {
    if (port.calls[i] != kExpected[i]) return false;
  }
  return true;
}

bool state_test_passes(const Style& s) {
  SimPort port(1.0);
  bring_up(port, 1.0, s);
  return port.enabled() && std::fabs(port.speed() - 1.0) < 1e-12 && port.violations() == 0;
}

struct Named {
  const char* name;
  Style style;
};

std::vector<Named> harmless() {
  std::vector<Named> v;
  Style a; a.steps = 8;           v.push_back({"ramp in 8 steps instead of 4", a});
  Style b; b.enable_twice = true; v.push_back({"enable twice", b});
  Style c; c.repeat_final = true; v.push_back({"set the final speed again", c});
  Style d; d.disable_first = true; v.push_back({"disable before starting", d});
  Style e; e.steps = 2;           v.push_back({"ramp in 2 steps", e});
  return v;
}

std::vector<Named> bugs() {
  std::vector<Named> v;
  Style a; a.forget_enable = true; v.push_back({"never enables the driver", a});
  Style b; b.overshoot = true;     v.push_back({"last step past target and limit", b});
  Style c; c.stop_short = true;    v.push_back({"ends one step short", c});
  return v;
}

}  // namespace

RC_TEST("the fake keeps the state of a motor: it is off until enabled, and stopping it stops it") {
  SimPort port(1.0);
  RC_CHECK(!port.enabled());
  RC_CHECK_NEAR(port.speed(), 0.0, 1e-12);

  port.enable();
  port.set_speed(0.5);
  RC_CHECK(port.enabled());
  RC_CHECK_NEAR(port.speed(), 0.5, 1e-12);
  RC_CHECK_EQ(port.violations(), 0);

  port.disable();
  RC_CHECK(!port.enabled());
  RC_CHECK_NEAR(port.speed(), 0.0, 1e-12);          // a disabled motor is not turning
}

RC_TEST("the fake counts a speed set while disabled and a speed past the limit, and does not obey either") {
  SimPort port(1.0);
  port.set_speed(0.5);                     // disabled
  RC_CHECK_EQ(port.violations(), 1);
  RC_CHECK_NEAR(port.speed(), 0.0, 1e-12);

  port.enable();
  port.set_speed(1.5);                     // past the limit
  RC_CHECK_EQ(port.violations(), 2);
  RC_CHECK_NEAR(port.speed(), 0.0, 1e-12);
  port.set_speed(-1.0);                    // exactly at the limit, going backwards, is fine
  RC_CHECK_EQ(port.violations(), 2);
  RC_CHECK_NEAR(port.speed(), -1.0, 1e-12);
}

RC_TEST("a test of the calls breaks on harmless changes, and a test of the state does not") {
  std::cout << "  change                               calls recorded   state of the motor\n";
  int interaction_broken = 0;
  int state_broken = 0;
  for (const Named& n : harmless()) {
    const bool by_calls = interaction_test_passes(n.style);
    const bool by_state = state_test_passes(n.style);
    std::cout << "  " << std::left << std::setw(36) << n.name << std::right << std::setw(14) << (by_calls ? "passes" : "FAILS")
              << std::setw(21) << (by_state ? "passes" : "FAILS") << "\n";
    if (!by_calls) ++interaction_broken;
    if (!by_state) ++state_broken;
  }
  std::cout << "\n  of 5 changes that do not matter: the calls test failed " << interaction_broken << ", the state test failed "
            << state_broken << "\n";
  RC_CHECK(state_test_passes(Style()));
  RC_CHECK(interaction_test_passes(Style()));
  RC_CHECK_EQ(interaction_broken, 5);
  RC_CHECK_EQ(state_broken, 0);
}

RC_TEST("both kinds of test catch the bugs, but only the state test says what the bug did") {
  std::cout << "\n  bug                                  calls recorded   state of the motor\n";
  int interaction_caught = 0;
  int state_caught = 0;
  for (const Named& n : bugs()) {
    const bool by_calls = !interaction_test_passes(n.style);
    const bool by_state = !state_test_passes(n.style);
    std::cout << "  " << std::left << std::setw(36) << n.name << std::right << std::setw(14) << (by_calls ? "caught" : "missed")
              << std::setw(21) << (by_state ? "caught" : "missed") << "\n";
    if (by_calls) ++interaction_caught;
    if (by_state) ++state_caught;
  }
  std::cout << "\n";
  RC_CHECK_EQ(state_caught, 3);
  RC_CHECK_EQ(interaction_caught, 3);

  // What the state says: the number of rules broken, and where the motor ended.
  SimPort forgot(1.0);
  Style never;
  never.forget_enable = true;
  bring_up(forgot, 1.0, never);
  std::cout << "  never enabled: " << forgot.violations() << " speeds ignored by the driver, final speed " << forgot.speed() << "\n";
  RC_CHECK_EQ(forgot.violations(), 4);
  RC_CHECK_NEAR(forgot.speed(), 0.0, 1e-12);

  SimPort over(1.0);
  Style past;
  past.overshoot = true;
  bring_up(over, 1.0, past);
  std::cout << "  last step too big: " << over.violations() << " speed refused, final speed " << over.speed() << "\n\n";
  RC_CHECK_EQ(over.violations(), 1);
  RC_CHECK_NEAR(over.speed(), 0.75, 1e-12);
}
