#include <rc/test/rc_test.hpp>

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>

#include "solution.hpp"

namespace {

const double kStep = 0.001;
const double kTarget = 2.0;      // radians, a large move for a joint that can do 3 rad/s
const double kLoadAt = 8.0;      // seconds: a steady load torque arrives
const double kLoad = 5.0;        // newton metres
const double kTorqueLimit = 20.0;

// A joint: an inertia with a little viscous friction, driven by a torque the motor can
// only supply up to a limit.
struct Joint {
  double inertia = 0.5;
  double viscous = 0.1;
  double position = 0.0;
  double velocity = 0.0;

  void step(double torque, double load, double dt) {
    const double applied = std::max(-kTorqueLimit, std::min(kTorqueLimit, torque));
    velocity += (applied - load - viscous * velocity) / inertia * dt;
    position += velocity * dt;
  }
};

struct Outcome {
  double overshoot = 0.0;        // percent of the move
  double settle = 0.0;           // seconds to stay within two percent
  double peak_speed = 0.0;
  double load_worst = 0.0;       // the worst error after the load arrives
  double load_final = 0.0;
};

enum class Kind { kPD, kPID, kCascade };

// One run: a move of kTarget, held, and then a steady load, for fourteen seconds. The two
// single loop controllers are the usual ones, tuned for critical damping, and the PID has
// an integral term of the same size as the cascade's.
Outcome run(Kind kind) {
  Joint joint;
  CascadeController cascade{CascadeGains{}};
  double integral = 0.0;
  Outcome out;
  double peak = 0.0;

  for (int i = 0; i < 14000; ++i) {
    const double t = static_cast<double>(i) * kStep;
    const double load = t >= kLoadAt ? kLoad : 0.0;
    const double error = kTarget - joint.position;

    double torque = 0.0;
    if (kind == Kind::kCascade) {
      torque = cascade.update(kTarget, joint.position, joint.velocity, kStep);
    } else {
      integral += kind == Kind::kPID ? error * kStep : 0.0;
      torque = 200.0 * error - 28.0 * joint.velocity + (kind == Kind::kPID ? 150.0 * integral : 0.0);
    }

    joint.step(torque, load, kStep);
    out.peak_speed = std::fmax(out.peak_speed, std::fabs(joint.velocity));

    if (t < kLoadAt) {
      peak = std::fmax(peak, joint.position);
      if (std::fabs(kTarget - joint.position) > 0.02 * kTarget) out.settle = t;
    } else {
      out.load_worst = std::fmax(out.load_worst, std::fabs(kTarget - joint.position));
    }
    out.load_final = kTarget - joint.position;
  }
  out.overshoot = 100.0 * (peak - kTarget) / kTarget;
  return out;
}

}  // namespace

RC_TEST("the outer loop asks for a speed in proportion to the error, and never more than the limit") {
  CascadeController controller{CascadeGains{}};

  controller.update(1.0, 0.9, 0.0, kStep);
  RC_CHECK_NEAR(controller.speed_command(), 10.0 * 0.1, 1e-12);

  controller.update(1.0, 0.0, 0.0, kStep);                 // an error of one radian would ask for 10 rad/s
  RC_CHECK_NEAR(controller.speed_command(), 3.0, 1e-12);

  controller.update(-100.0, 0.0, 0.0, kStep);              // and the other way
  RC_CHECK_NEAR(controller.speed_command(), -3.0, 1e-12);

  controller.update(1.0, 1.0, 0.0, kStep);                 // on target, no speed asked for
  RC_CHECK_NEAR(controller.speed_command(), 0.0, 1e-12);
}

RC_TEST("the inner loop is proportional on the speed error and integrates it") {
  CascadeGains g;
  g.torque_limit = 1e9;      // out of the way
  CascadeController c(g);

  // A speed error of 1 rad/s: asked for 3 (a big position error) and going at 2.
  const double first = c.update(10.0, 0.0, 2.0, 0.01);
  RC_CHECK_NEAR(first, 40.0 * 1.0, 1e-9);                  // proportional only, nothing accumulated yet
  RC_CHECK_NEAR(c.integral(), 1.0 * 0.01, 1e-12);

  const double second = c.update(10.0, 0.0, 2.0, 0.01);
  RC_CHECK_NEAR(second, 40.0 * 1.0 + 400.0 * 0.01, 1e-9);  // and now the integral is in it
  RC_CHECK_NEAR(c.integral(), 2.0 * 0.01, 1e-12);

  c.reset();
  RC_CHECK_NEAR(c.integral(), 0.0, 1e-15);
  RC_CHECK_NEAR(c.speed_command(), 0.0, 1e-15);
}

RC_TEST("the torque is held to the limit, and the integral stops growing while it is") {
  CascadeController guarded{CascadeGains{}};
  double torque = 0.0;

  // Pushing hard against something that does not move: asked for 3 rad/s, going at nothing,
  // for five seconds, with a motor that tops out at 20 N m.
  for (int i = 0; i < 5000; ++i) torque = guarded.update(10.0, 0.0, 0.0, kStep);
  RC_CHECK_NEAR(torque, kTorqueLimit, 1e-12);

  // The integral did not run away. At a speed error of 3, a gain of 40 gives 120 N m before
  // the integral adds anything, so it was saturated from the first step and should have
  // accumulated almost nothing; unguarded it would be 3 * 5 = 15 by now.
  std::cout << "  five seconds against a stuck joint, the motor saturated all the time: the integral is "
            << std::fixed << std::setprecision(3) << guarded.integral() << ", where an unguarded one would be 15.\n\n";
  RC_CHECK(guarded.integral() < 0.1);

  // And so the loop can answer at once when the target moves back, instead of unwinding.
  const double reverse = guarded.update(-10.0, 0.0, 0.0, kStep);
  RC_CHECK_NEAR(reverse, -kTorqueLimit, 1e-12);
}

RC_TEST("a move of two radians, and then a load: one loop against two") {
  const Outcome pd = run(Kind::kPD);
  const Outcome pid = run(Kind::kPID);
  const Outcome casc = run(Kind::kCascade);

  const auto row = [](const char* what, const Outcome& o) {
    std::cout << "    " << std::left << std::setw(26) << what << std::right << std::fixed << std::setprecision(1)
              << std::setw(8) << o.overshoot << " %" << std::setprecision(2) << std::setw(8) << o.settle << " s"
              << std::setw(10) << o.peak_speed << std::setprecision(4) << std::setw(11) << o.load_worst
              << std::setw(11) << o.load_final << "\n";
  };

  std::cout << "  a joint of 0.5 kg m^2 asked to move 2 radians with a motor of 20 N m, held, and at eight\n"
            << "  seconds a steady load of 5 N m arrives:\n\n"
            << "    controller                 overshoot   settled  peak speed  worst error after the load\n"
            << "                                                        (rad/s)       (rad)      final (rad)\n";
  row("position loop, P and D", pd);
  row("position loop, PID", pid);
  row("cascade, speed limit 3", casc);
  std::cout << "\n";

  // The cascade does not overshoot and the PID does: the PID's integral winds up through the
  // saturated move and has to be unwound on the far side.
  RC_CHECK(casc.overshoot < 1.0);
  RC_CHECK(pid.overshoot > 5.0);
  RC_CHECK(pid.settle > 2.0 * casc.settle);

  // The speed limit is respected without anyone enforcing it: the outer loop never asks for
  // more than 3, and the inner loop overshoots that a little. The single loops reach
  // whatever the motor will give.
  RC_CHECK(casc.peak_speed < 1.5 * 3.0);
  RC_CHECK(pd.peak_speed > 2.0 * casc.peak_speed);
  RC_CHECK(pid.peak_speed > 2.0 * casc.peak_speed);

  // A load that arrives: the proportional position loop sags by the load over its gain and
  // stays there, the PID takes seconds to find it, and the cascade corrects it in the speed
  // loop and is the fastest by a good margin, finishing on target.
  RC_CHECK(pd.load_final > 0.02);
  RC_CHECK(casc.load_worst < 0.01);
  RC_CHECK(casc.load_worst * 3.0 < pid.load_worst);
  RC_CHECK(std::fabs(casc.load_final) < 1e-3);
}
