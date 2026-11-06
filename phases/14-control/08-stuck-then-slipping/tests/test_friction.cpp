#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

#include "solution.hpp"

namespace {

const double kStep = 0.001;

struct Outcome {
  double final_error = 0.0;
  HuntingReport report;
};

// A PID loop holding the plant at one metre. The integral is optionally frozen while the
// error is smaller than `freeze`, and the last third of the run is judged: a loop that
// has settled is quiet there.
Outcome hold(double kp, double ki, double kd, double freeze, double seconds = 60.0) {
  FrictionPlant plant;
  const double target = 1.0;
  double integral = 0.0;
  std::vector<double> error;

  const int steps = static_cast<int>(seconds / kStep);
  for (int i = 0; i < steps; ++i) {
    const double e = target - plant.position;
    if (std::fabs(e) >= freeze) integral += e * kStep;
    const double u = kp * e + ki * integral - kd * plant.velocity;
    plant.step(u, kStep);
    error.push_back(e);
  }

  Outcome out;
  out.final_error = error.back();
  const std::size_t from = error.size() * 2 / 3;
  out.report = analyze_hunting(rc::span<const double>(error.data() + from, error.size() - from), 0.02, 2);
  return out;
}

}  // namespace

RC_TEST("a mass that is stuck stays stuck until the force passes the breakaway, and not before") {
  FrictionPlant below;
  for (int i = 0; i < 2000; ++i) below.step(1.4, kStep);   // 1.4 N against a breakaway of 1.5
  RC_CHECK_NEAR(below.position, 0.0, 1e-12);
  RC_CHECK_NEAR(below.velocity, 0.0, 1e-12);

  FrictionPlant above;
  for (int i = 0; i < 2000; ++i) above.step(1.6, kStep);
  RC_CHECK(above.position > 0.1);
  RC_CHECK(above.velocity > 0.0);

  FrictionPlant backwards;
  for (int i = 0; i < 2000; ++i) backwards.step(-1.6, kStep);
  RC_CHECK(backwards.position < -0.1);   // the same in either direction
}

RC_TEST("once moving it needs less force to keep going than it took to start") {
  // Stiction is bigger than Coulomb friction, so there is a range of forces that will
  // not start the mass and will keep it going. 1.2 N is in it.
  FrictionPlant from_rest;
  for (int i = 0; i < 5000; ++i) from_rest.step(1.2, kStep);
  RC_CHECK_NEAR(from_rest.position, 0.0, 1e-12);

  FrictionPlant already_moving;
  for (int i = 0; i < 1000; ++i) already_moving.step(1.6, kStep);   // started
  const double speed_before = already_moving.velocity;
  for (int i = 0; i < 20000; ++i) already_moving.step(1.2, kStep);  // and then eased off

  // It keeps going, and settles at the speed where the force less Coulomb is balanced by
  // the viscous drag: (1.2 - 1.0) / 0.5 = 0.4 metres a second.
  RC_CHECK(speed_before > 0.0);
  RC_CHECK_NEAR(already_moving.velocity, 0.4, 1e-3);
}

RC_TEST("a moving mass with no force on it stops, and then stays exactly stopped") {
  FrictionPlant plant;
  plant.velocity = 2.0;

  int stopped_at = -1;
  for (int i = 0; i < 10000; ++i) {
    plant.step(0.0, kStep);
    if (stopped_at < 0 && plant.velocity == 0.0) stopped_at = i;
  }
  RC_REQUIRE(stopped_at > 0);

  // Exactly zero afterwards. A friction written as sign times a force chatters from one
  // side of zero to the other and never stops, which is the bug this state avoids.
  const double rest = plant.position;
  for (int i = 0; i < 5000; ++i) {
    plant.step(0.0, kStep);
    RC_CHECK_EQ(plant.velocity, 0.0);
  }
  RC_CHECK_NEAR(plant.position, rest, 1e-15);

  // And it stopped when the equations say it should. Slowing from 2 m/s under a
  // Coulomb force of 1 N and a viscous drag of 0.5 N per metre a second takes
  // (m / b) * ln(1 + b * v / Coulomb) = 2 * ln 2 = 1.386 seconds.
  RC_CHECK_NEAR(static_cast<double>(stopped_at) * kStep, 2.0 * std::log(2.0), 0.01);
}

RC_TEST("hunting is a swing that keeps crossing its mean, and a steady offset is not one") {
  std::vector<double> swing;
  std::vector<double> offset(500, 0.8);
  std::vector<double> small;
  for (int i = 0; i < 1000; ++i) {
    swing.push_back(0.3 + 0.05 * std::sin(static_cast<double>(i) * 0.1));   // about an offset
    small.push_back(0.001 * std::sin(static_cast<double>(i) * 0.1));
  }

  const HuntingReport s = analyze_hunting(rc::span<const double>(swing.data(), swing.size()), 0.02, 2);
  const HuntingReport o = analyze_hunting(rc::span<const double>(offset.data(), offset.size()), 0.02, 2);
  const HuntingReport t = analyze_hunting(rc::span<const double>(small.data(), small.size()), 0.02, 2);

  RC_CHECK(s.hunting);
  RC_CHECK_NEAR(s.peak_to_peak, 0.1, 2e-3);
  RC_CHECK(s.crossings >= 10);
  RC_CHECK(!o.hunting);                     // large and steady
  RC_CHECK_NEAR(o.peak_to_peak, 0.0, 1e-12);
  RC_CHECK_EQ(o.crossings, 0);
  RC_CHECK(!t.hunting);                     // crossing, and too small to matter
  RC_CHECK(t.crossings >= 10);

  const HuntingReport none = analyze_hunting(rc::span<const double>(), 0.02, 2);
  RC_CHECK(!none.hunting);
}

RC_TEST("without an integral the loop sticks short, and with one it can hunt") {
  const Outcome pd = hold(20.0, 0.0, 2.0, 0.0);
  const Outcome gentle = hold(20.0, 5.0, 2.0, 0.0);
  const Outcome medium = hold(20.0, 20.0, 2.0, 0.0);
  const Outcome hard = hold(20.0, 50.0, 2.0, 0.0);

  const auto row = [](const char* what, const Outcome& o) {
    std::cout << "    " << std::left << std::setw(26) << what << std::right << std::fixed << std::setprecision(4)
              << std::setw(11) << o.final_error << std::setw(12) << o.report.peak_to_peak << std::setw(10)
              << o.report.crossings << "   " << (o.report.hunting ? "hunting" : "settled") << "\n";
  };

  std::cout << "  a mass of 1 kg with a breakaway of 1.5 N and a Coulomb friction of 1.0 N, held at one metre\n"
            << "  with a proportional gain of 20 and damping of 2, sixty seconds, the last twenty judged:\n\n"
            << "    controller                 final error  peak to peak  crossings\n";
  row("no integral", pd);
  row("integral gain 5", gentle);
  row("integral gain 20", medium);
  row("integral gain 50", hard);
  std::cout << "\n  With no integral the force stops growing once it is under the breakaway, so the mass\n"
            << "  sticks where it is: the error can be as large as 1.5 / 20 = 0.075 and never closes.\n\n";

  // No integral: it stops short, inside the band the breakaway force allows, and stays.
  RC_CHECK(std::fabs(pd.final_error) > 0.03);
  RC_CHECK(std::fabs(pd.final_error) <= 1.5 / 20.0 + 1e-9);
  RC_CHECK(!pd.report.hunting);

  // A small integral gets there and settles. A larger one winds up past the breakaway,
  // the mass lets go, overshoots, sticks, and the integral winds the other way.
  RC_CHECK(!gentle.report.hunting);
  RC_CHECK(std::fabs(gentle.final_error) < std::fabs(pd.final_error));
  RC_CHECK(medium.report.hunting);
  RC_CHECK(hard.report.hunting);
  RC_CHECK(hard.report.peak_to_peak > medium.report.peak_to_peak);
  RC_CHECK(hard.report.crossings > medium.report.crossings);
}

RC_TEST("freezing the integral inside a small band stops the hunting, and a wide band costs accuracy") {
  const double freezes[] = {0.0, 0.01, 0.03, 0.08};
  Outcome results[4];
  for (int i = 0; i < 4; ++i) results[i] = hold(20.0, 20.0, 2.0, freezes[i]);

  std::cout << "  the same loop with an integral gain of 20, the integral frozen while the error is smaller\n"
            << "  than a band:\n\n"
            << "    band    final error  peak to peak   " << "\n";
  for (int i = 0; i < 4; ++i) {
    std::cout << "    " << std::fixed << std::setprecision(2) << std::setw(4) << freezes[i] << std::setprecision(4)
              << std::setw(13) << results[i].final_error << std::setw(14) << results[i].report.peak_to_peak << "   "
              << (results[i].report.hunting ? "hunting" : "settled") << "\n";
  }
  std::cout << "\n  The band trades the hunting for a steady error about as large as the band.\n\n";

  RC_CHECK(results[0].report.hunting);
  RC_CHECK(!results[1].report.hunting);
  RC_CHECK(!results[2].report.hunting);
  RC_CHECK(std::fabs(results[1].final_error) < 0.01);
  RC_CHECK(std::fabs(results[2].final_error) < 0.03);

  // Too wide a band is just a bigger error: the loop is quiet and further from the target
  // than the hunting ever took it.
  RC_CHECK(std::fabs(results[3].final_error) > std::fabs(results[1].final_error));
  RC_CHECK(std::fabs(results[3].final_error) > 0.03);
}
