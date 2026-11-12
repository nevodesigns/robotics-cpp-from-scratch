#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <algorithm>
#include <cmath>

enum class PowerState { ok, derate, stop };

struct PowerConfig {
  double volt_warn = 22.0;       // below this (open circuit estimate) the power is reduced
  double volt_stop = 20.0;       // at or below this it stops
  double volt_resume = 21.0;     // once stopped it may run again only at or above this
  double temp_warn = 60.0;       // above this (deg C) the power is reduced
  double temp_stop = 80.0;       // at or above this it stops
  double temp_resume = 70.0;     // once stopped it may run again only at or below this
  double resistance = 0.1;       // the battery's internal resistance, ohms: what makes the voltage sag under load
  double volt_min = 5.0;         // a reading outside the plausible range is not a reading
  double volt_max = 60.0;
  double temp_min = -40.0;
  double temp_max = 150.0;
};

struct PowerLimit {
  PowerState state = PowerState::stop;
  double factor = 0.0;           // the share of full power allowed: 1 ok, between 0 and 1 derated, 0 stopped
};

// The monitor of a battery and a motor. It turns a voltage at the terminals, the current being drawn and a temperature into a share of full power.
class PowerMonitor {
 public:
  explicit PowerMonitor(const PowerConfig& c) : cfg_(c) {}

  PowerLimit update(double volts, double amps, double temp) {
    // TODO: Implausible (any NaN, volts outside volt_min..volt_max, temp outside temp_min..temp_max): latch stopped_ and return the default PowerLimit (stop, factor 0).
    // rest = volts + max(amps, 0) * resistance. vf = clamp01((rest - volt_stop) / (volt_warn - volt_stop)); tf = clamp01((temp_stop - temp) / (temp_stop - temp_warn)); f = min(vf, tf).
    // If stopped_: leave it only when rest >= volt_resume and temp <= temp_resume and f > 0. If not stopped_ and f <= 0: latch it.
    // Stopped: the default PowerLimit. Otherwise factor f, state derate if f < 1 else ok.
    (void)volts;
    (void)amps;
    (void)temp;
    (void)cfg_;
    (void)stopped_;
    return PowerLimit();
  }

  bool stopped() const { return stopped_; }

 private:
  static double clamp01(double x) { return std::max(0.0, std::min(1.0, x)); }

  PowerConfig cfg_;
  bool stopped_ = false;
};

#endif  // LESSON_SOLUTION_HPP
