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
    {"vx_max", 1.0},
    {"vy_max", 0.4},
    {"ask_vx", 1.0},
    {"ask_vy", 0.4},
    {"clip_vx", 0.7071067811865475},
    {"clip_vy", 0.282842712474619},
    {"jump_to", 0.8},
    {"ax", 1.0},
    {"t_ramp", 0.8},
    {"v_legs", 1.6},
    {"step_max", 0.2},
    {"t_stance", 0.125},
    {"v_ask", 0.5},
    {"step_need", 0.0625},
    {"ok_slow", 1.0},
    {"v_fast", 2.0},
    {"ok_fast", 0.0},
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


RC_TEST("a command outside the ellipse is scaled back onto it, keeping its direction, and the turn rate is capped") {
  CmdLimits l;
  Cmd now, ask;
  ask.vx = 1.0;
  ask.vy = 0.4;
  ask.wz = 5.0;
  // let the command in use catch up with a very large time step so only the clipping shows
  const Cmd c = limit_command(ask, now, l, 100.0);
  rec("vx_max", l.vx_max, 1);
  rec("vy_max", l.vy_max, 1);
  rec("ask_vx", ask.vx, 1);
  rec("ask_vy", ask.vy, 1);
  rec("clip_vx", c.vx, 2);
  rec("clip_vy", c.vy, 2);
  RC_CHECK_NEAR(std::sqrt((c.vx / l.vx_max) * (c.vx / l.vx_max) + (c.vy / l.vy_max) * (c.vy / l.vy_max)), 1.0, 1e-9);
  RC_CHECK_NEAR(c.vy / c.vx, ask.vy / ask.vx, 1e-9);
  RC_CHECK_NEAR(c.wz, l.wz_max, 1e-12);
  Cmd inside;
  inside.vx = 0.5;
  inside.vy = 0.1;
  const Cmd same = limit_command(inside, now, l, 100.0);
  RC_CHECK_NEAR(same.vx, 0.5, 1e-12);
  RC_CHECK_NEAR(same.vy, 0.1, 1e-12);
}

RC_TEST("a jump in the command becomes a ramp at the bounded acceleration, in both directions") {
  CmdLimits l;
  Cmd now, ask;
  ask.vx = 0.8;
  const double dt = 0.01;
  int n = 0;
  while (now.vx < 0.8 - 1e-9 && n < 10000) {
    const Cmd next = limit_command(ask, now, l, dt);
    RC_CHECK(next.vx - now.vx <= l.ax * dt + 1e-12);
    now = next;
    ++n;
  }
  rec("jump_to", 0.8, 1);
  rec("ax", l.ax, 1);
  rec("t_ramp", n * dt, 2);
  RC_CHECK_NEAR(n * dt, 0.8 / l.ax, 2.0 * dt);
  ask.vx = -0.3;
  const Cmd back = limit_command(ask, now, l, dt);
  RC_CHECK(back.vx < now.vx && now.vx - back.vx <= l.ax * dt + 1e-12);
  Cmd zero;
  now.vx = 0.5;
  for (int i = 0; i < 100; ++i) now = limit_command(zero, now, l, dt);
  RC_CHECK_NEAR(now.vx, 0.0, 1e-12);
}

RC_TEST("the legs allow a top speed, and a speed asks for a step that must be within the leg's reach") {
  const double step_max = 0.20, duty = 0.5, period_min = 0.25;
  const double v_legs = max_speed(step_max, duty, period_min);
  const double stance = duty * period_min;
  rec("v_legs", v_legs, 2);
  rec("step_max", step_max, 2);
  rec("t_stance", stance, 3);
  const Stride slow = gait_for_speed(0.5, stance, step_max), fast = gait_for_speed(2.0, stance, step_max);
  rec("v_ask", 0.5, 1);
  rec("step_need", slow.step, 3);
  rec("ok_slow", slow.ok ? 1 : 0, 0);
  rec("v_fast", 2.0, 1);
  rec("ok_fast", fast.ok ? 1 : 0, 0);
  RC_CHECK_NEAR(gait_for_speed(v_legs, stance, step_max).step, step_max, 1e-12);
  RC_CHECK(slow.ok && !fast.ok);
  RC_CHECK_NEAR(slow.step, 0.5 * stance, 1e-12);
}
