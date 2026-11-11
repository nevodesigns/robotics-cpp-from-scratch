#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>

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
  // TODO: with d of zero or less, or a negative hysteresis, give the default result (not valid).
  //
  // Run a SpeedDrive for 60000 steps of dt = 1e-3. The target is a speed of zero, error = -speed.
  // The relay output sign starts at +1. If sign is +1 and error < -hysteresis it flips to -1; if sign is -1 and
  // error > hysteresis it flips to +1, and that flip is the start of a new cycle. The command is d * sign.
  //
  // Between flips track the highest and lowest speed. At each new cycle, once more than 3 cycles have gone
  // (the first ones are still settling), keep the time since the last flip to +1 as a period and half of
  // high minus low as an amplitude, then reset high and low. Keep the last three of each.
  //
  // With fewer than three kept the result is not valid. Otherwise period and amplitude are the means of the
  // last three, ultimate_gain = 4 * d / (pi * amplitude), and valid is true.
  (void)d;
  (void)hysteresis;
  return RelayResult{};
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
  // TODO: kp = 0.6 * ku, ti = tu / 2, td = tu / 8; ki = kp / ti and kd = kp * td.
  // No gains (all zero) for an ultimate gain or period of zero or less.
  (void)ku;
  (void)tu;
  return PidGains{};
}

// A gentler rule, for some overshoot: Kp = 0.33 Ku, Ti = Tu / 2, Td = Tu / 3.
inline PidGains some_overshoot(double ku, double tu) {
  // TODO: kp = 0.33 * ku, ti = tu / 2, td = tu / 3; ki = kp / ti and kd = kp * td. All zero for ku or tu of zero or less.
  (void)ku;
  (void)tu;
  return PidGains{};
}

#endif  // LESSON_SOLUTION_HPP
