#include <rc/test/rc_test.hpp>

#include <rc/control/pid.hpp>
#include <rc/control/tuning.hpp>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

#include "solution.hpp"

namespace {

using rc::control::Mass;
using rc::control::Pid;
using rc::control::PlantModel;

constexpr double kTopSpeed = 0.5;
constexpr double kAcceleration = 1.0;
constexpr double kDt = 0.002;

const PlantModel kPlant{1.0, 0.6, 0.4};

// The largest jump between consecutive commanded velocities, sampled finely.
//
// A profile the machine can follow changes velocity by no more than its own
// acceleration times the interval between samples. Anything larger is an
// acceleration the profile never promised, and in the limit an infinite one.
double worst_velocity_jump(const Trapezoid& profile, int samples = 20000) {
  double worst = 0.0;
  double previous = 0.0;
  for (int i = 1; i <= samples; ++i) {
    const double t = i * profile.duration() / samples;
    const double velocity = profile.at(t).velocity;
    const double jump = std::fabs(velocity - previous);
    if (jump > worst) worst = jump;
    previous = velocity;
  }
  return worst;
}

// The three phase assumption, written without checking that a cruise exists.
struct ThreePhases {
  double distance = 0.0, top = 0.0, accel = 0.0, ramp = 0.0, cruise = 0.0;

  ThreePhases(double d, double v, double a)
      : distance(d), top(v), accel(a), ramp(v / a), cruise((d - v * v / a) / v) {}

  double duration() const { return 2.0 * ramp + cruise; }

  Setpoint at(double t) const {
    Setpoint point;
    if (t <= 0.0) return point;
    if (t < ramp) return {0.5 * accel * t * t, accel * t, accel};
    const double first = 0.5 * accel * ramp * ramp;
    if (t < ramp + cruise) return {first + top * (t - ramp), top, 0.0};
    const double s = t - ramp - cruise;
    return {first + top * cruise + top * s - 0.5 * accel * s * s, top - accel * s,
            -accel};
  }
};

struct Drive {
  double peak_command = 0.0;
  double worst_error = 0.0;
};

// Drive a one metre move, as a step or through a profile with feedforward.
Drive move_one_metre(bool profiled) {
  Pid pid(20.0, 0.0, 8.0, -1e9, 1e9);
  Mass plant(kPlant.mass, kPlant.damping, kPlant.load);
  const Trapezoid profile(1.0, kTopSpeed, kAcceleration);

  Drive drive;
  for (int i = 0; i < 4000; ++i) {
    const double t = i * kDt;
    Setpoint target;
    if (profiled) {
      target = profile.at(t);
    } else {
      target.position = t < 0.1 ? 0.0 : 1.0;
    }

    double command = pid.update(target.position, plant.position(), kDt);
    if (profiled) command += kPlant.force_for(target.velocity, target.acceleration);

    if (std::fabs(command) > drive.peak_command) drive.peak_command = std::fabs(command);
    plant.step(command, kDt);

    if (t > 0.2) {
      const double error = std::fabs(target.position - plant.position());
      if (error > drive.worst_error) drive.worst_error = error;
    }
  }
  return drive;
}

}  // namespace

RC_TEST("a long move has three phases and a short one has two") {
  std::cout << "\n    top speed " << kTopSpeed << " m/s, acceleration "
            << kAcceleration << " m/s^2\n";
  std::cout << "    speeding up and slowing down again costs "
            << kTopSpeed * kTopSpeed / kAcceleration << " m\n\n";
  std::cout << "    " << std::right << std::setw(10) << "distance" << std::setw(10)
            << "ramp s" << std::setw(11) << "cruise s" << std::setw(10) << "total s"
            << std::setw(12) << "peak m/s" << std::setw(16) << "arrives at" << "\n";

  for (const double distance : {0.05, 0.1, 0.2, 0.25, 0.3, 1.0, 5.0}) {
    const Trapezoid profile(distance, kTopSpeed, kAcceleration);
    std::cout << "    " << std::right << std::fixed << std::setprecision(3)
              << std::setw(10) << distance << std::setprecision(4) << std::setw(10)
              << profile.ramp_time() << std::setw(11) << profile.cruise_time()
              << std::setw(10) << profile.duration() << std::setw(12)
              << profile.peak_speed() << std::setprecision(9) << std::setw(16)
              << profile.at(profile.duration()).position << "\n";

    // However the phases fall out, it arrives exactly and stops.
    RC_CHECK_NEAR(profile.at(profile.duration()).position, distance, 1e-12);
    RC_CHECK_EQ(profile.at(profile.duration()).velocity, 0.0);
  }

  // The distance that decides it is exactly top speed squared over the
  // acceleration, and it is a boundary rather than a region.
  const double boundary = kTopSpeed * kTopSpeed / kAcceleration;
  RC_CHECK(!Trapezoid(boundary * 0.999, kTopSpeed, kAcceleration).cruises());
  RC_CHECK(Trapezoid(boundary * 1.001, kTopSpeed, kAcceleration).cruises());

  // A short move never reaches the top speed, and the peak it does reach is
  // the square root of the acceleration times the distance.
  const Trapezoid brief(0.05, kTopSpeed, kAcceleration);
  RC_CHECK(brief.peak_speed() < kTopSpeed);
  RC_CHECK_NEAR(brief.peak_speed(), std::sqrt(kAcceleration * 0.05), 1e-12);
}

RC_TEST("the commanded velocity never jumps") {
  const Trapezoid profile(1.0, kTopSpeed, kAcceleration);

  std::cout << "\n    a one metre move, sampled\n\n";
  std::cout << "    " << std::right << std::setw(10) << "t" << std::setw(14)
            << "position" << std::setw(14) << "velocity" << std::setw(14)
            << "acceleration" << "\n";
  for (const double t : {0.0, 0.25, 0.5, 1.0, 2.0, 2.5, 3.0}) {
    const Setpoint point = profile.at(t);
    std::cout << "    " << std::right << std::fixed << std::setprecision(4)
              << std::setw(10) << t << std::setprecision(6) << std::setw(14)
              << point.position << std::setw(14) << point.velocity
              << std::setprecision(2) << std::setw(14) << point.acceleration << "\n";
  }

  const int samples = 20000;
  const double interval = profile.duration() / samples;
  const double allowed = kAcceleration * interval;
  const double jump = worst_velocity_jump(profile, samples);

  std::cout << "\n    sampled " << samples << " times, the largest change in the\n";
  std::cout << "    commanded velocity between two samples is " << std::scientific
            << std::setprecision(2) << jump << ",\n";
  std::cout << "    against the " << allowed << " the profile's own acceleration\n";
  std::cout << "    over that interval allows\n" << std::defaultfloat;

  // Continuous to the sampling interval, which is what "a machine can follow
  // this" means: no acceleration anywhere that the profile did not promise.
  RC_CHECK(jump <= allowed * 1.001);

  // Past the end it holds the destination rather than extrapolating, which is
  // what a caller running its clock on relies upon.
  RC_CHECK_NEAR(profile.at(10.0).position, 1.0, 1e-12);
  RC_CHECK_EQ(profile.at(10.0).velocity, 0.0);
  RC_CHECK_EQ(profile.at(-1.0).position, 0.0);
}

RC_TEST("the three phase assumption on a move too short for three phases") {
  const ThreePhases naive(0.10, kTopSpeed, kAcceleration);
  const Trapezoid honest(0.10, kTopSpeed, kAcceleration);

  std::cout << "\n    a 0.10 m move, planned as three phases regardless\n\n";
  std::cout << "    " << std::left << std::setw(20) << "ramp" << std::right
            << std::fixed << std::setprecision(4) << naive.ramp << " s\n";
  std::cout << "    " << std::left << std::setw(20) << "cruise" << std::right
            << naive.cruise << " s\n";
  std::cout << "    " << std::left << std::setw(20) << "duration" << std::right
            << naive.duration() << " s\n\n";

  std::cout << "    " << std::right << std::setw(10) << "t" << std::setw(16)
            << "naive velocity" << std::setw(18) << "correct velocity" << "\n";
  for (const double t : {0.30, 0.40, 0.4999, 0.5001, 0.60}) {
    std::cout << "    " << std::right << std::fixed << std::setprecision(4)
              << std::setw(10) << t << std::setprecision(6) << std::setw(16)
              << naive.at(t).velocity << std::setw(18) << honest.at(t).velocity << "\n";
  }

  // A negative cruise time, which is the arithmetic saying the move does not
  // fit and nothing looking.
  RC_CHECK(naive.cruise < 0.0);

  // It still arrives, exactly, which is what makes this survive a test.
  RC_CHECK_NEAR(naive.at(naive.duration()).position, 0.10, 1e-12);
  RC_CHECK_NEAR(naive.at(naive.duration()).velocity, 0.0, 1e-12);

  // And partway through it asks for an instant change of velocity, which is an
  // infinite acceleration and the one thing a profile exists to prevent.
  double worst = 0.0, previous = 0.0;
  for (int i = 1; i <= 20000; ++i) {
    const double velocity = naive.at(i * naive.duration() / 20000).velocity;
    worst = std::max(worst, std::fabs(velocity - previous));
    previous = velocity;
  }
  std::cout << "\n    largest jump in the naive commanded velocity: " << std::fixed
            << std::setprecision(4) << worst << " m/s, in one sample\n";
  std::cout << "\n    it arrives in the right place, having asked for an\n";
  std::cout << "    infinite acceleration on the way. An endpoint test passes\n";
  std::cout << "    and the machine bangs\n";

  RC_CHECK(worst > 0.25);
  RC_CHECK(worst_velocity_jump(honest) <=
           kAcceleration * honest.duration() / 20000.0 * 1.001);
}

RC_TEST("stretching the move without changing its shape") {
  const Trapezoid quick(1.0, kTopSpeed, kAcceleration);
  const Trapezoid slow = quick.scaled_to(quick.duration() * 2.0);

  std::cout << "\n    the same one metre move, stretched to twice the time\n\n";
  std::cout << "    " << std::left << std::setw(22) << "original duration"
            << std::right << std::fixed << std::setprecision(4) << quick.duration()
            << " s\n";
  std::cout << "    " << std::left << std::setw(22) << "stretched duration"
            << std::right << slow.duration() << " s\n";
  std::cout << "    " << std::left << std::setw(22) << "original peak speed"
            << std::right << quick.peak_speed() << " m/s\n";
  std::cout << "    " << std::left << std::setw(22) << "stretched peak speed"
            << std::right << slow.peak_speed() << " m/s\n";

  RC_CHECK_NEAR(slow.duration(), quick.duration() * 2.0, 1e-9);
  RC_CHECK_NEAR(slow.peak_speed(), quick.peak_speed() / 2.0, 1e-9);
  RC_CHECK_NEAR(slow.at(slow.duration()).position, 1.0, 1e-12);

  // The shape is the same: at the same fraction of the way through, the target
  // is at the same fraction of the way along.
  for (const double fraction : {0.1, 0.25, 0.5, 0.75, 0.9}) {
    RC_CHECK_NEAR(slow.at(slow.duration() * fraction).position,
                  quick.at(quick.duration() * fraction).position, 1e-9);
  }

  std::cout << "\n    at every fraction of the way through, the stretched\n";
  std::cout << "    profile is at the same place. Slowing the whole path is\n";
  std::cout << "    what keeps the shape, which is what lesson 13-04 needed\n";

  // A duration shorter than the machine can manage is refused rather than
  // returning something it cannot follow.
  const Trapezoid impossible = quick.scaled_to(quick.duration() * 0.5);
  RC_CHECK_NEAR(impossible.duration(), quick.duration(), 1e-12);
}

RC_TEST("a step and a profile through the same loop") {
  const Drive stepped = move_one_metre(false);
  const Drive profiled = move_one_metre(true);

  std::cout << "\n    a one metre move through the loop from 14-05\n\n";
  std::cout << "    " << std::left << std::setw(34) << "" << std::right
            << std::setw(16) << "peak command" << std::setw(16) << "worst error" << "\n";
  std::cout << "    " << std::left << std::setw(34) << "a step" << std::right
            << std::fixed << std::setprecision(1) << std::setw(16)
            << stepped.peak_command << std::setprecision(4) << std::setw(16)
            << stepped.worst_error << "\n";
  std::cout << "    " << std::left << std::setw(34) << "a profile with feedforward"
            << std::right << std::setprecision(1) << std::setw(16)
            << profiled.peak_command << std::setprecision(4) << std::setw(16)
            << profiled.worst_error << "\n";

  std::cout << "\n    the step asks for a metre immediately and gets whatever\n";
  std::cout << "    the actuator has until the error comes down. What is left\n";
  std::cout << "    of the profile's error is the standing lag from 14-05,\n";
  std::cout << "    which that lesson says how to remove\n";

  // An order of magnitude less command, and far closer tracking.
  RC_CHECK(profiled.peak_command < stepped.peak_command * 0.2);
  RC_CHECK(profiled.worst_error < stepped.worst_error * 0.3);

  // And the residual is the derivative term's standing lag, kd * v / kp.
  RC_CHECK_NEAR(profiled.worst_error, 8.0 * kTopSpeed / 20.0, 0.01);
}

RC_TEST("a profile of nothing, and a move backwards") {
  // No distance to cover.
  const Trapezoid still(0.0, kTopSpeed, kAcceleration);
  RC_CHECK_EQ(still.duration(), 0.0);
  RC_CHECK_EQ(still.at(0.0).position, 0.0);
  RC_CHECK_EQ(still.at(5.0).position, 0.0);

  // Nonsense limits produce a profile that does nothing rather than a division
  // by zero.
  const Trapezoid broken(1.0, 0.0, kAcceleration);
  RC_CHECK_EQ(broken.duration(), 0.0);
  const Trapezoid also_broken(1.0, kTopSpeed, 0.0);
  RC_CHECK_EQ(also_broken.duration(), 0.0);

  // Backwards is the same move with every sign turned round.
  const Trapezoid back(-1.0, kTopSpeed, kAcceleration);
  const Trapezoid forth(1.0, kTopSpeed, kAcceleration);
  RC_CHECK_NEAR(back.duration(), forth.duration(), 1e-12);
  RC_CHECK_NEAR(back.at(back.duration()).position, -1.0, 1e-12);
  RC_CHECK_NEAR(back.peak_speed(), -forth.peak_speed(), 1e-12);

  for (const double t : {0.3, 1.0, 2.0}) {
    RC_CHECK_NEAR(back.at(t).position, -forth.at(t).position, 1e-12);
    RC_CHECK_NEAR(back.at(t).velocity, -forth.at(t).velocity, 1e-12);
    RC_CHECK_NEAR(back.at(t).acceleration, -forth.at(t).acceleration, 1e-12);
  }
}
