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

constexpr double kDt = 0.002;
constexpr double kKp = 20.0;
constexpr double kKd = 8.0;
constexpr double kCruise = 0.5;   // metres per second
constexpr int kSteps = 8000;      // sixteen seconds

const PlantModel kPlant{1.0, 0.6, 0.4};

struct Result {
  double settled = 0.0;      // mean error over the last second
  double worst_late = 0.0;   // worst error after two seconds
};

// Follow a target that accelerates for a second and then holds a steady speed.
//
// `on_error` swaps the derivative from the measurement to the error, which is
// the other way of removing the same term.
Result follow_ramp(double ki, bool feedforward, bool on_error,
                   double damping_guess = kPlant.damping) {
  Pid pid(kKp, ki, on_error ? 0.0 : kKd, -50.0, 50.0);
  Mass plant(kPlant.mass, kPlant.damping, kPlant.load);

  PlantModel model = kPlant;
  model.damping = damping_guess;

  Setpoint target;
  double last_error = 0.0;
  bool have_last = false;
  Result result;
  std::vector<double> errors;

  for (int i = 0; i < kSteps; ++i) {
    const double t = i * kDt;
    target.velocity = t < 1.0 ? kCruise * t : kCruise;
    target.acceleration = t < 1.0 ? kCruise : 0.0;
    target.position += target.velocity * kDt;

    double feedback = pid.update(target.position, plant.position(), kDt);
    if (on_error) {
      const double error = target.position - plant.position();
      if (have_last) feedback += kKd * (error - last_error) / kDt;
      last_error = error;
      have_last = true;
    }

    const double total =
        feedforward ? command(feedback, model, target, -50.0, 50.0) : feedback;
    plant.step(total, kDt);

    const double error = std::fabs(target.position - plant.position());
    errors.push_back(error);
    if (t > 2.0 && error > result.worst_late) result.worst_late = error;
  }

  double tail = 0.0;
  for (std::size_t i = errors.size() - 500; i < errors.size(); ++i) tail += errors[i];
  result.settled = tail / 500.0;
  return result;
}

// The largest command a step change of setpoint provokes.
double step_peak(bool on_error) {
  Pid pid(kKp, 0.0, on_error ? 0.0 : kKd, -1e9, 1e9);
  Mass plant(kPlant.mass, kPlant.damping, kPlant.load);
  double last_error = 0.0;
  bool have_last = false;
  double peak = 0.0;

  for (int i = 0; i < 2000; ++i) {
    const double target = i < 200 ? 0.0 : 1.0;
    double output = pid.update(target, plant.position(), kDt);
    if (on_error) {
      const double error = target - plant.position();
      if (have_last) output += kKd * (error - last_error) / kDt;
      last_error = error;
      have_last = true;
    }
    if (std::fabs(output) > peak) peak = std::fabs(output);
    plant.step(output, kDt);
  }
  return peak;
}

}  // namespace

RC_TEST("what the plant needs, before anything has gone wrong") {
  // Standing still, only the load.
  RC_CHECK_NEAR(kPlant.force_for(0.0, 0.0), 0.4, 1e-12);

  // Moving steadily, the load and the damping.
  RC_CHECK_NEAR(kPlant.force_for(0.5, 0.0), 0.4 + 0.6 * 0.5, 1e-12);

  // Accelerating, the mass as well, and only while the speed is changing.
  RC_CHECK_NEAR(kPlant.force_for(0.5, 2.0), 0.4 + 0.3 + 2.0, 1e-12);

  // Backwards, the damping opposes the motion and the load does not care.
  RC_CHECK_NEAR(kPlant.force_for(-0.5, 0.0), 0.4 - 0.3, 1e-12);

  // A model of nothing asks for nothing.
  const PlantModel weightless{0.0, 0.0, 0.0};
  RC_CHECK_EQ(weightless.force_for(3.0, 4.0), 0.0);
}

RC_TEST("the error a reacting loop is left with, predicted and then measured") {
  const double predicted = ramp_error(kPlant, kKp, kKd, kCruise);
  const Result measured = follow_ramp(0.0, false, false);

  std::cout << "\n    following a ramp at " << kCruise << " m/s, kp = " << kKp
            << ", kd = " << kKd << "\n\n";
  std::cout << "    " << std::left << std::setw(36) << "predicted" << std::right
            << std::fixed << std::setprecision(4) << predicted << " m\n";
  std::cout << "    " << std::left << std::setw(36) << "measured" << std::right
            << measured.settled << " m\n\n";
  std::cout << "    " << std::left << std::setw(36) << "of which the plant's own force"
            << std::right
            << (kPlant.load + kPlant.damping * kCruise) / kKp << " m\n";
  std::cout << "    " << std::left << std::setw(36) << "and the derivative term dragging"
            << std::right << kKd * kCruise / kKp << " m\n";

  std::cout << "\n    it does not decay. That is the error for as long as the\n";
  std::cout << "    target keeps moving, and the larger half of it is the\n";
  std::cout << "    controller opposing the motion rather than the plant\n";

  RC_CHECK_NEAR(measured.settled, predicted, 0.002);

  // It is a constant, not a transient: the worst error after two seconds is
  // the same as the settled one.
  RC_CHECK_NEAR(measured.worst_late, measured.settled, 0.001);

  // And the derivative term is the larger contributor here.
  RC_CHECK(kKd * kCruise / kKp > (kPlant.load + kPlant.damping * kCruise) / kKp);
}

RC_TEST("two causes, and each fix removes exactly its own") {
  std::cout << "\n    " << std::left << std::setw(44) << "" << std::right
            << std::setw(14) << "steady error" << std::setw(16) << "worst after 2s"
            << "\n";

  const auto row = [](const char* name, const Result& result) {
    std::cout << "    " << std::left << std::setw(44) << name << std::right
              << std::fixed << std::setprecision(6) << std::setw(14) << result.settled
              << std::setw(16) << result.worst_late << "\n";
    return result;
  };

  const Result plain = row("feedback only, derivative on measurement",
                           follow_ramp(0.0, false, false));
  const Result fed = row("with feedforward for the plant", follow_ramp(0.0, true, false));
  const Result on_error = row("derivative on error instead", follow_ramp(0.0, false, true));
  const Result both = row("feedforward and derivative on error",
                          follow_ramp(0.0, true, true));
  const Result integral = row("integral instead of feedforward, ki = 20",
                              follow_ramp(20.0, false, false));

  std::cout << "\n    feedforward removes the plant's share, the derivative\n";
  std::cout << "    change removes the controller's, and the two together\n";
  std::cout << "    remove the error\n";

  const double plant_share = (kPlant.load + kPlant.damping * kCruise) / kKp;
  const double derivative_share = kKd * kCruise / kKp;

  // Feedforward takes away the plant's share and leaves the derivative's.
  RC_CHECK_NEAR(plain.settled - fed.settled, plant_share, 0.002);
  RC_CHECK_NEAR(fed.settled, derivative_share, 0.002);

  // Changing where the derivative is taken does the opposite.
  RC_CHECK_NEAR(plain.settled - on_error.settled, derivative_share, 0.002);
  RC_CHECK_NEAR(on_error.settled, plant_share, 0.002);

  // Together, nothing worth measuring is left.
  RC_CHECK(both.settled < 0.002);

  // An integrator gets there too, and it takes a detour: it has to build the
  // force out of accumulated error, so it overshoots on the way.
  RC_CHECK(integral.settled < 0.002);
  RC_CHECK(integral.worst_late > both.worst_late * 20.0);
}

RC_TEST("what the other derivative costs, on a step") {
  const double on_measurement = step_peak(false);
  const double on_error = step_peak(true);

  std::cout << "\n    a step of one metre, and the largest command it provokes\n\n";
  std::cout << "    " << std::left << std::setw(32) << "derivative on measurement"
            << std::right << std::fixed << std::setprecision(1) << std::setw(10)
            << on_measurement << " N\n";
  std::cout << "    " << std::left << std::setw(32) << "derivative on error"
            << std::right << std::setw(10) << on_error << " N\n";

  std::cout << "\n    the setpoint moved a metre in one two millisecond step, so\n";
  std::cout << "    the error's derivative was five hundred metres a second and\n";
  std::cout << "    the term produced four thousand newtons of it. That is why\n";
  std::cout << "    the derivative is taken on the measurement, and why\n";
  std::cout << "    feedforward is the better answer to the ramp\n";

  // Two hundred times the command, from the same gain.
  RC_CHECK(on_error > on_measurement * 100.0);
  RC_CHECK_NEAR(on_error, kKd * 1.0 / kDt, kKd * 1.0 / kDt * 0.02);

  // The measurement cannot jump, so its derivative stays sane.
  RC_CHECK(on_measurement < 30.0);
}

RC_TEST("a rough model beats no model, in both directions") {
  std::cout << "\n    feedforward with a damping estimate that is wrong\n\n";
  std::cout << "    " << std::right << std::setw(18) << "guess / true"
            << std::setw(16) << "steady error" << "\n";

  std::vector<double> errors;
  for (const double factor : {0.0, 0.5, 1.0, 1.5, 2.0}) {
    const Result result = follow_ramp(0.0, true, true, kPlant.damping * factor);
    errors.push_back(result.settled);
    std::cout << "    " << std::right << std::fixed << std::setprecision(2)
              << std::setw(18) << factor << std::setprecision(6) << std::setw(16)
              << result.settled << "\n";
  }

  const double nothing = follow_ramp(0.0, false, false).settled;
  std::cout << "\n    with no feedforward at all: " << std::setprecision(6) << nothing
            << " m\n";
  std::cout << "\n    a damping term left out entirely is still sixteen times\n";
  std::cout << "    better than nothing, and there is no cliff: the error is\n";
  std::cout << "    proportional to how wrong the model is, and symmetric, so\n";
  std::cout << "    guessing high costs the same as guessing low\n";

  // The right model is the best of them.
  RC_CHECK(errors[2] < errors[0]);
  RC_CHECK(errors[2] < errors[4]);

  // Wrong by the same amount either way costs about the same.
  RC_CHECK_NEAR(errors[1] - errors[2], errors[3] - errors[2], 0.003);

  // And even the worst of them is far better than not feeding forward.
  for (const double error : errors) RC_CHECK(error < nothing * 0.1);
}

RC_TEST("the total is limited once, not twice") {
  const Setpoint fast{0.0, 10.0, 0.0};   // needs 0.4 + 6.0 newtons

  // Feedback near the limit, plus a feedforward that would exceed it.
  RC_CHECK_NEAR(command(45.0, kPlant, fast, -50.0, 50.0), 50.0, 1e-12);
  RC_CHECK_NEAR(command(-45.0, kPlant, fast, -50.0, 50.0), -38.6, 1e-12);

  // Below the limit nothing is touched, and the two parts simply add.
  RC_CHECK_NEAR(command(1.0, kPlant, Setpoint{0.0, 0.5, 0.0}, -50.0, 50.0), 1.7, 1e-12);

  // A feedforward alone can reach the limit, which is a real thing to know: it
  // means the profile is asking for more than the actuator has, before any
  // error has happened at all.
  const Setpoint impossible{0.0, 100.0, 0.0};
  RC_CHECK_NEAR(command(0.0, kPlant, impossible, -50.0, 50.0), 50.0, 1e-12);
}
