// rc/control/autotune.hpp
//
// Tuning by experiment, from lesson 14-14, graduated.
//
// A relay that pushes +d or -d according to which side of the target the speed is on makes a plant swing by itself, and
// the swing gives the ultimate gain as Ku = 4 d / (pi a) and the ultimate period as the time of a cycle. On a motor speed
// plant of three lags the relay reported Ku 5.71 and Tu 0.544 s against 6.42 and 0.513 s worked out from the plant's own
// constants, and the answer did not move as the relay went from 0.25 to 4.
//
// A wide margin on the relay bends it: Ku read 5.71, 4.39, 3.51 and 2.64 for margins of 0.01, 0.05, 0.10 and 0.20.
//
// The two rule sets, on a step to 1: Ziegler and Nichols gave rise 0.135 s, overshoot 0.597, settling 1.574 s and no final error;
// the gentler set gave 0.226 s, 0.376, 1.943 s and none.

#ifndef RC_CONTROL_AUTOTUNE_HPP
#define RC_CONTROL_AUTOTUNE_HPP

#include <cmath>

namespace rc {
namespace control {

// A motor speed loop's plant: the command becomes a speed through three lags in series, the mechanics (0.5 s),
// the electrics (0.2 s) and the speed sensor's filter (50 ms), with a steady state gain of 3. It is the plant the tuning
// is done on, and nothing in the tuning knows these numbers.
class SpeedDrive {
 public:
  static constexpr double kGain = 3.0;
  static constexpr double kMechanical = 0.5;
  static constexpr double kElectrical = 0.2;
  static constexpr double kSensor = 0.05;

  double step(double command, double dt) {
    a_ += (kGain * command - a_) / kMechanical * dt;
    b_ += (a_ - b_) / kElectrical * dt;
    c_ += (b_ - c_) / kSensor * dt;
    return c_;
  }
  double speed() const { return c_; }
  void reset() { a_ = b_ = c_ = 0.0; }

 private:
  double a_ = 0.0;
  double b_ = 0.0;
  double c_ = 0.0;
};

// What a relay test found.
struct RelayResult {
  bool valid = false;           // enough clean cycles were seen
  double amplitude = 0.0;       // half the peak to peak swing of the speed
  double period = 0.0;          // seconds per cycle
  double ultimate_gain = 0.0;   // the proportional gain at which the loop would just sustain the swing
};

// Drive the plant with a relay: the command is +d while the speed is below the target of zero (by a margin of
// `hysteresis`) and -d while it is above, so the speed swings about zero by itself. Its swing gives the ultimate gain
// as Ku = 4 d / (pi a), and the time of a cycle is the ultimate period.
inline RelayResult relay_test(double d, double hysteresis) {
  RelayResult result;
  if (d <= 0.0 || hysteresis < 0.0) return result;

  const double dt = 1e-3;
  const int steps = 60000;
  SpeedDrive drive;
  double sign = 1.0;
  double high = -1e9;
  double low = 1e9;
  double last_rise = -1.0;
  int rises = 0;
  double periods[3] = {0.0, 0.0, 0.0};
  double amplitudes[3] = {0.0, 0.0, 0.0};
  int kept = 0;

  for (int i = 0; i < steps; ++i) {
    const double error = -drive.speed();
    bool rose = false;
    if (sign > 0.0 && error < -hysteresis) sign = -1.0;
    else if (sign < 0.0 && error > hysteresis) {
      sign = 1.0;
      rose = true;
    }
    const double p = drive.step(d * sign, dt);
    if (p > high) high = p;
    if (p < low) low = p;
    if (rose) {
      const double t = i * dt;
      ++rises;
      if (rises > 3 && last_rise >= 0.0) {
        periods[kept % 3] = t - last_rise;
        amplitudes[kept % 3] = 0.5 * (high - low);
        ++kept;
      }
      last_rise = t;
      high = -1e9;
      low = 1e9;
    }
  }

  if (kept < 3) return result;
  result.period = (periods[0] + periods[1] + periods[2]) / 3.0;
  result.amplitude = (amplitudes[0] + amplitudes[1] + amplitudes[2]) / 3.0;
  if (result.amplitude <= 0.0 || result.period <= 0.0) return result;
  result.ultimate_gain = 4.0 * d / (3.14159265358979323846 * result.amplitude);
  result.valid = true;
  return result;
}

// A set of PID gains.
struct PidGains {
  double kp = 0.0;
  double ki = 0.0;
  double kd = 0.0;
};

// Ziegler and Nichols' rule for a quarter amplitude decay: Kp = 0.6 Ku, Ti = Tu / 2, Td = Tu / 8.
// ki = kp / Ti and kd = kp * Td. No gains for an ultimate gain or period of zero or less.
inline PidGains ziegler_nichols(double ku, double tu) {
  PidGains g;
  if (ku <= 0.0 || tu <= 0.0) return g;
  g.kp = 0.6 * ku;
  g.ki = g.kp / (0.5 * tu);
  g.kd = g.kp * (tu / 8.0);
  return g;
}

// A gentler rule, for some overshoot: Kp = 0.33 Ku, Ti = Tu / 2, Td = Tu / 3.
inline PidGains some_overshoot(double ku, double tu) {
  PidGains g;
  if (ku <= 0.0 || tu <= 0.0) return g;
  g.kp = 0.33 * ku;
  g.ki = g.kp / (0.5 * tu);
  g.kd = g.kp * (tu / 3.0);
  return g;
}

}  // namespace control
}  // namespace rc

#endif  // RC_CONTROL_AUTOTUNE_HPP
