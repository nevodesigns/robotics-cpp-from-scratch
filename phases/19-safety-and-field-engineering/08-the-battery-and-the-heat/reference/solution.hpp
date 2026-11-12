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
    const bool plausible = !std::isnan(volts) && !std::isnan(amps) && !std::isnan(temp) && volts >= cfg_.volt_min && volts <= cfg_.volt_max && temp >= cfg_.temp_min && temp <= cfg_.temp_max;
    PowerLimit out;
    if (!plausible) {
      stopped_ = true;                         // not knowing is a stop, and it needs a good reading to leave
      return out;
    }
    // The terminals sag by current times resistance while the load is drawn, and the battery has not lost that charge: add it back to get the voltage the battery would have at rest.
    const double rest = volts + std::max(amps, 0.0) * cfg_.resistance;
    const double vf = clamp01((rest - cfg_.volt_stop) / (cfg_.volt_warn - cfg_.volt_stop));
    const double tf = clamp01((cfg_.temp_stop - temp) / (cfg_.temp_stop - cfg_.temp_warn));
    const double f = std::min(vf, tf);
    if (stopped_) {
      if (rest >= cfg_.volt_resume && temp <= cfg_.temp_resume && f > 0.0) stopped_ = false;
    } else if (f <= 0.0) {
      stopped_ = true;
    }
    if (stopped_) return out;
    out.factor = f;
    out.state = f < 1.0 ? PowerState::derate : PowerState::ok;
    return out;
  }

  bool stopped() const { return stopped_; }

 private:
  static double clamp01(double x) { return std::max(0.0, std::min(1.0, x)); }

  PowerConfig cfg_;
  bool stopped_ = false;
};

#endif  // LESSON_SOLUTION_HPP
