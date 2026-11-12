#include <rc/test/rc_test.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>

#include "solution.hpp"

namespace {

struct Noise {
  std::uint64_t s;
  explicit Noise(std::uint64_t seed) : s(seed) {}
  double uniform() {
    s = s * 6364136223846793005ULL + 1442695040888963407ULL;
    return (static_cast<double>(s >> 11) + 0.5) / 9007199254740992.0;
  }
  double gauss(double sigma) { return sigma * std::sqrt(-2.0 * std::log(uniform())) * std::cos(6.283185307179586 * uniform()); }
};

const double kNaN = std::numeric_limits<double>::quiet_NaN();

const char* name(PowerState s) { return s == PowerState::ok ? "ok" : (s == PowerState::derate ? "derate" : "stop"); }

}  // namespace

RC_TEST("power falls smoothly from full at the warning voltage to nothing at the stop voltage, and the temperature does the same") {
  PowerMonitor m{PowerConfig()};
  std::cout << "  at rest, 25 deg C: volts -> share of power\n";
  const double volts[] = {24.0, 22.0, 21.5, 21.0, 20.5, 20.1};
  double previous = 2.0;
  for (double v : volts) {
    PowerMonitor f{PowerConfig()};
    const PowerLimit p = f.update(v, 0.0, 25.0);
    std::cout << "  " << std::fixed << std::setprecision(1) << v << " V   " << std::setprecision(2) << p.factor << "  " << name(p.state) << "\n";
    RC_CHECK(p.factor <= previous);
    previous = p.factor;
  }
  RC_CHECK_NEAR(m.update(22.0, 0.0, 25.0).factor, 1.0, 1e-12);
  RC_CHECK(m.update(22.0, 0.0, 25.0).state == PowerState::ok);
  RC_CHECK_NEAR(PowerMonitor{PowerConfig()}.update(21.0, 0.0, 25.0).factor, 0.5, 1e-12);
  RC_CHECK_NEAR(PowerMonitor{PowerConfig()}.update(24.0, 0.0, 70.0).factor, 0.5, 1e-12);
  RC_CHECK_NEAR(PowerMonitor{PowerConfig()}.update(24.0, 0.0, 60.0).factor, 1.0, 1e-12);
  RC_CHECK(PowerMonitor{PowerConfig()}.update(24.0, 0.0, 80.0).state == PowerState::stop);
  RC_CHECK(PowerMonitor{PowerConfig()}.update(20.0, 0.0, 25.0).state == PowerState::stop);
  std::cout << "  at 70 deg C the share was " << PowerMonitor{PowerConfig()}.update(24.0, 0.0, 70.0).factor << ", at 80 it stopped\n\n";
}

RC_TEST("a stop on the voltage at the terminals stops the robot every time it draws current, and the corrected voltage does not") {
  // A battery that is 22.0 V at rest, driving a load that is on for a second and off for a second. At 15 A it sags by 1.5 V.
  PowerConfig c;
  PowerMonitor m(c);
  int naive_changes = 0;
  int monitor_changes = 0;
  bool naive_stopped = false;
  PowerState last = PowerState::ok;
  double lowest_terminal = 99.0;
  for (int i = 0; i < 600; ++i) {                          // 60 s at 10 Hz
    const double amps = (i / 10) % 2 == 0 ? 15.0 : 0.0;
    const double volts = 22.0 - amps * c.resistance;
    lowest_terminal = std::min(lowest_terminal, volts);
    const bool now = volts < 21.0;                        // the obvious cut-off: stop below 21 V, run above
    if (now != naive_stopped) ++naive_changes;
    naive_stopped = now;
    const PowerState s = m.update(volts, amps, 25.0).state;
    if (s != last) ++monitor_changes;
    last = s;
  }
  std::cout << "  22.0 V at rest, 15 A in one-second pulses, 60 s: the terminals fell to " << std::fixed << std::setprecision(1) << lowest_terminal << " V\n";
  std::cout << "  stop below 21 V at the terminals: " << naive_changes << " changes between stopped and running; the corrected voltage: " << monitor_changes << "\n\n";
  RC_CHECK(naive_changes >= 58);
  RC_CHECK(monitor_changes == 0);
}

RC_TEST("a resume point above the stop point turns a robot that chatters at the threshold into one that stops once") {
  PowerConfig plain;
  plain.volt_resume = plain.volt_stop;                    // no hysteresis: resume as soon as it is above the stop
  PowerConfig better;                                      // resume at 21.0
  PowerMonitor a(plain);
  PowerMonitor b(better);
  Noise n(99);
  int stops_a = 0;
  int stops_b = 0;
  bool was_a = false;
  bool was_b = false;
  for (int i = 0; i < 600; ++i) {
    const double v = 20.0 + n.gauss(0.05);                // a battery sitting right at its stop voltage, with a little noise
    const bool sa = a.update(v, 0.0, 25.0).state == PowerState::stop;
    const bool sb = b.update(v, 0.0, 25.0).state == PowerState::stop;
    if (sa && !was_a) ++stops_a;
    if (sb && !was_b) ++stops_b;
    was_a = sa;
    was_b = sb;
  }
  std::cout << "  600 samples at 20.00 V with noise of 0.05 V: stops with a resume point equal to the stop: " << stops_a << "; with a resume at 21.0 V: " << stops_b << "\n\n";
  RC_CHECK(stops_a > 50);
  RC_CHECK(stops_b == 1);
  RC_CHECK(b.stopped());
  RC_CHECK(b.update(20.5, 0.0, 25.0).state == PowerState::stop);
  RC_CHECK(b.update(21.2, 0.0, 25.0).state != PowerState::stop);
}

RC_TEST("derating with the temperature holds a motor below its limit, and a hard stop at the limit makes it cycle") {
  // A motor that heats at 1.2 deg C per second at full power (in proportion to the power squared) and loses heat at 0.02 per second of its excess over 25.
  PowerConfig derating;
  PowerConfig hard;
  hard.temp_warn = hard.temp_stop - 0.001;                // no derating zone
  int stops[2] = {0, 0};
  double peak[2] = {0.0, 0.0};
  double mean_factor[2] = {0.0, 0.0};
  const PowerConfig* cfgs[2] = {&derating, &hard};
  for (int k = 0; k < 2; ++k) {
    PowerMonitor m(*cfgs[k]);
    double temp = 25.0;
    bool was_stop = false;
    int run_samples = 0;
    for (int i = 0; i < 36000; ++i) {                     // an hour at 10 Hz
      const PowerLimit p = m.update(24.0, 0.0, temp);
      const bool st = p.state == PowerState::stop;
      if (st && !was_stop) ++stops[k];
      was_stop = st;
      temp += (1.2 * p.factor * p.factor - 0.02 * (temp - 25.0)) * 0.1;
      peak[k] = std::max(peak[k], temp);
      if (i >= 18000) {
        mean_factor[k] += p.factor;
        ++run_samples;
      }
    }
    mean_factor[k] /= run_samples;
  }
  std::cout << "  one hour at full demand: with derating the peak was " << std::fixed << std::setprecision(1) << peak[0] << " deg C, stops " << stops[0] << ", mean power in the second half " << std::setprecision(2) << mean_factor[0] << "\n";
  std::cout << "  with a hard stop only:   the peak was " << std::setprecision(1) << peak[1] << " deg C, stops " << stops[1] << ", mean power in the second half " << std::setprecision(2) << mean_factor[1] << "\n\n";
  RC_CHECK(stops[0] == 0);
  RC_CHECK(peak[0] < 70.0);
  RC_CHECK(stops[1] >= 5);
  RC_CHECK(peak[1] > 79.0);
  RC_CHECK(mean_factor[0] > 0.7);
}

RC_TEST("a reading that cannot be real is a stop, and it takes good readings to leave it") {
  PowerMonitor m{PowerConfig()};
  RC_CHECK(m.update(24.0, 5.0, 25.0).state == PowerState::ok);
  RC_CHECK(m.update(kNaN, 5.0, 25.0).state == PowerState::stop);
  RC_CHECK(m.stopped());
  RC_CHECK(m.update(24.0, 5.0, 25.0).state == PowerState::ok);      // a good reading from a healthy battery
  RC_CHECK(m.update(24.0, 5.0, 200.0).state == PowerState::stop);   // a thermistor wire that came off reads off the scale
  RC_CHECK(m.update(24.0, 5.0, -80.0).state == PowerState::stop);
  RC_CHECK(m.update(2.0, 5.0, 25.0).state == PowerState::stop);     // a battery voltage of 2 V is a broken wire
  RC_CHECK(m.update(24.0, kNaN, 25.0).state == PowerState::stop);
  RC_CHECK(m.update(24.0, -3.0, 25.0).state == PowerState::ok);     // regeneration is not a load: no correction is added
  std::cout << "  not a number, 200 deg C, -80 deg C and 2 V all stopped it; a good reading brought it back (the battery and motor were healthy)\n\n";
}
