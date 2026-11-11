#include <rc/control/pid.hpp>
#include <rc/control/tuning.hpp>
#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

#include "solution.hpp"

namespace {

const double kPi = 3.14159265358979323846;

// What the plant's ultimate gain and period are, worked out here from its constants. The tuning never sees this.
// The ultimate frequency is where the three lags add up to a phase of minus 180 degrees, found by bisection, and the
// ultimate gain is one over the plant's size there.
double true_frequency() {
  double lo = 0.1;
  double hi = 200.0;
  for (int i = 0; i < 80; ++i) {
    const double w = 0.5 * (lo + hi);
    const double phase = std::atan(w * SpeedDrive::kMechanical) + std::atan(w * SpeedDrive::kElectrical) +
                         std::atan(w * SpeedDrive::kSensor);
    if (phase < kPi) lo = w;
    else hi = w;
  }
  return 0.5 * (lo + hi);
}
double true_period() { return 2.0 * kPi / true_frequency(); }
double true_gain() {
  const double w = true_frequency();
  const double size = SpeedDrive::kGain / (std::sqrt(1.0 + w * w * SpeedDrive::kMechanical * SpeedDrive::kMechanical) *
                                           std::sqrt(1.0 + w * w * SpeedDrive::kElectrical * SpeedDrive::kElectrical) *
                                           std::sqrt(1.0 + w * w * SpeedDrive::kSensor * SpeedDrive::kSensor));
  return 1.0 / size;
}

// A step to 1 under a PID with these gains, for 12 s, and the four numbers that describe it.
rc::control::StepResponse step_with(const PidGains& g) {
  SpeedDrive drive;
  rc::control::Pid pid(g.kp, g.ki, g.kd, -1000.0, 1000.0);
  std::vector<double> output;
  const double dt = 1e-3;
  for (int i = 0; i < 12000; ++i) {
    const double u = pid.update(1.0, drive.speed(), dt);
    output.push_back(drive.step(u, dt));
  }
  return rc::control::analyse(rc::span<const double>(output.data(), output.size()), 1.0, dt, 0.02);
}

}  // namespace

RC_TEST("the relay makes the speed swing by itself, and reports the swing it saw") {
  const RelayResult r = relay_test(1.0, 0.01);
  std::cout << "  relay of 1 with a margin of 0.01: amplitude " << std::fixed << std::setprecision(4) << r.amplitude
            << " rad/s, period " << std::setprecision(3) << r.period << " s, ultimate gain " << std::setprecision(2)
            << r.ultimate_gain << "\n\n";
  RC_CHECK(r.valid);
  RC_CHECK(r.amplitude > 0.05 && r.amplitude < 0.5);
  RC_CHECK(r.period > 0.3 && r.period < 1.0);
}

RC_TEST("what the relay finds is the plant's ultimate gain and period, worked out here from its constants") {
  const RelayResult r = relay_test(1.0, 0.01);
  const double ku = true_gain();
  const double tu = true_period();
  std::cout << "  ultimate gain: relay " << std::fixed << std::setprecision(2) << r.ultimate_gain << ", from the plant "
            << ku << "\n  ultimate period: relay " << std::setprecision(3) << r.period << " s, from the plant " << tu
            << " s\n\n";
  RC_CHECK(r.valid);
  RC_CHECK_NEAR(r.ultimate_gain, ku, 0.15 * ku);
  RC_CHECK_NEAR(r.period, tu, 0.10 * tu);
}

RC_TEST("the answer does not depend on how hard the relay pushes, for a plant that is linear") {
  std::cout << "  relay size   amplitude   ultimate gain   period\n";
  double lowest = 1e9;
  double highest = 0.0;
  for (double d : {0.25, 0.5, 1.0, 2.0, 4.0}) {
    const RelayResult r = relay_test(d, 0.01 * d);
    std::cout << "  " << std::setw(10) << std::fixed << std::setprecision(2) << d << "   " << std::setw(9)
              << std::setprecision(4) << r.amplitude << "   " << std::setw(13) << std::setprecision(2)
              << r.ultimate_gain << "   " << std::setw(6) << std::setprecision(3) << r.period << "\n";
    RC_CHECK(r.valid);
    if (r.ultimate_gain < lowest) lowest = r.ultimate_gain;
    if (r.ultimate_gain > highest) highest = r.ultimate_gain;
  }
  std::cout << "\n";
  RC_CHECK(lowest > 0.0);
  RC_CHECK(highest < 1.05 * lowest);
}

RC_TEST("a wide margin on the relay bends the answer, and the gain it reports falls") {
  std::cout << "  margin   amplitude   ultimate gain   period\n";
  double previous = 1e9;
  bool falls = true;
  double tight = 0.0;
  double wide = 0.0;
  for (double h : {0.01, 0.05, 0.1, 0.2}) {
    const RelayResult r = relay_test(1.0, h);
    std::cout << "  " << std::setw(6) << std::fixed << std::setprecision(2) << h << "   " << std::setw(9)
              << std::setprecision(4) << r.amplitude << "   " << std::setw(13) << std::setprecision(2)
              << r.ultimate_gain << "   " << std::setw(6) << std::setprecision(3) << r.period << "\n";
    RC_CHECK(r.valid);
    if (!(r.ultimate_gain < previous)) falls = false;
    previous = r.ultimate_gain;
    if (h == 0.01) tight = r.ultimate_gain;
    if (h == 0.2) wide = r.ultimate_gain;
  }
  std::cout << "\n";
  RC_CHECK(falls);
  RC_CHECK(wide < 0.9 * tight);
}

RC_TEST("a relay that cannot be run gives nothing, and no result gives no gains") {
  RC_CHECK(relay_test(1.0, 0.01).valid);
  RC_CHECK(!relay_test(0.0, 0.01).valid);
  RC_CHECK(!relay_test(-1.0, 0.01).valid);
  RC_CHECK(!relay_test(1.0, -0.1).valid);
  const PidGains none = ziegler_nichols(0.0, 2.0);
  RC_CHECK_NEAR(none.kp, 0.0, 1e-12);
  RC_CHECK_NEAR(none.ki, 0.0, 1e-12);
  const PidGains also_none = some_overshoot(10.0, 0.0);
  RC_CHECK_NEAR(also_none.kp, 0.0, 1e-12);
}

RC_TEST("the rules turn the two numbers into gains: Ziegler and Nichols and the gentler set") {
  const PidGains z = ziegler_nichols(10.0, 2.0);
  RC_CHECK_NEAR(z.kp, 6.0, 1e-9);
  RC_CHECK_NEAR(z.ki, 6.0, 1e-9);     // kp / (Tu / 2)
  RC_CHECK_NEAR(z.kd, 1.5, 1e-9);     // kp * Tu / 8
  const PidGains g = some_overshoot(10.0, 3.0);
  RC_CHECK_NEAR(g.kp, 3.3, 1e-9);
  RC_CHECK_NEAR(g.ki, 3.3 / 1.5, 1e-9);
  RC_CHECK_NEAR(g.kd, 3.3, 1e-9);     // kp * Tu / 3
}

RC_TEST("gains from one relay test tune the loop: four numbers for each rule set, measured on a step") {
  const RelayResult r = relay_test(1.0, 0.01);
  RC_CHECK(r.valid);
  const PidGains classic = ziegler_nichols(r.ultimate_gain, r.period);
  const PidGains gentle = some_overshoot(r.ultimate_gain, r.period);
  const rc::control::StepResponse a = step_with(classic);
  const rc::control::StepResponse b = step_with(gentle);

  std::cout << "  from Ku " << std::fixed << std::setprecision(2) << r.ultimate_gain << " and Tu " << std::setprecision(3)
            << r.period << "\n  rule set          kp      ki      kd     rise (s)  overshoot  settling (s)  final error\n"
            << "  Ziegler-Nichols   " << std::setprecision(2) << std::setw(5) << classic.kp << "  " << std::setw(6) << classic.ki
            << "  " << std::setw(6) << classic.kd << "   " << std::setw(7) << std::setprecision(3) << a.rise_time << "  "
            << std::setw(9) << a.overshoot << "  " << std::setw(12) << a.settling_time << "  " << std::setw(11)
            << std::setprecision(4) << a.final_error << "\n"
            << "  some overshoot    " << std::setprecision(2) << std::setw(5) << gentle.kp << "  " << std::setw(6) << gentle.ki
            << "  " << std::setw(6) << gentle.kd << "   " << std::setw(7) << std::setprecision(3) << b.rise_time << "  "
            << std::setw(9) << b.overshoot << "  " << std::setw(12) << b.settling_time << "  " << std::setw(11)
            << std::setprecision(4) << b.final_error << "\n\n";

  RC_CHECK(a.rise_time > 0.0);
  RC_CHECK(b.rise_time > 0.0);
  RC_CHECK(a.settling_time > 0.0);
  RC_CHECK(b.settling_time > 0.0);
  RC_CHECK(std::fabs(a.final_error) < 0.01);
  RC_CHECK(std::fabs(b.final_error) < 0.01);
  RC_CHECK(b.overshoot < a.overshoot);
  RC_CHECK(a.rise_time < b.rise_time);
}
