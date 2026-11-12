#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <algorithm>
#include <cmath>

// What a data sheet gives, at the output shaft of the gearbox, plus the numbers it does not give. Every value is typical of a 12 V, 37 mm class brushed gear motor, not a measurement of one.
struct GearSpec {
  double volts = 12.0;             // supply voltage (typical)
  double no_load_rpm = 350.0;      // output shaft speed with no load at that voltage (typical)
  double stall_torque = 0.7;       // output shaft torque when held, at that voltage (N m, typical)
  double gear_ratio = 30.0;        // motor turns per output turn (typical)
  double max_current = 3.0;        // the driver's current limit (A, typical)
  double rotor_inertia = 3.0e-6;   // kg m2 (typical)
  double load_inertia = 2.0e-3;    // kg m2 at the output shaft: a wheel and a share of a robot (typical)
  double viscous = 1.0e-6;         // N m s/rad at the motor shaft (typical)
  double backlash_deg = 2.0;       // the total gap in the gearbox, in degrees at the output (typical)
};

class GearMotor {
 public:
  explicit GearMotor(const GearSpec& spec = GearSpec()) : s_(spec) {
    const double pi = 3.14159265358979323846;
    const double w_nl_motor = spec.no_load_rpm * 2.0 * pi / 60.0 * spec.gear_ratio;
    const double stall_motor = spec.stall_torque / spec.gear_ratio;
    // The data sheet's no load speed already includes the friction, so the back voltage constant is a little smaller than volts over speed.
    ke_ = spec.volts / w_nl_motor - spec.viscous * spec.volts / stall_motor;
    r_ = spec.volts * ke_ / stall_motor;
    j_ = spec.rotor_inertia + spec.load_inertia / (spec.gear_ratio * spec.gear_ratio);
    half_gap_ = spec.backlash_deg * pi / 180.0 / 2.0;
  }

  // The electromechanical time constant, from the constants: how long the speed takes to reach 63 percent of its final value after a step in voltage, with no load.
  double time_constant() const { return j_ * r_ / (ke_ * ke_ + s_.viscous * r_); }

  // The most torque the output shaft can give at a voltage, which is the stall torque or what the current limit allows.
  double max_torque_out(double volts) const {
    const double current = std::min(s_.max_current, std::fabs(volts) / r_);
    return current * ke_ * s_.gear_ratio;
  }

  // Advances by dt: the voltage drives a current (limited), the current a torque, and the torque, less friction and the load reflected to the motor, accelerates the rotor.
  void step(double volts, double load_torque_out, double dt) {
    const double v = std::max(-s_.volts, std::min(s_.volts, volts));
    double current = (v - ke_ * w_) / r_;
    current = std::max(-s_.max_current, std::min(s_.max_current, current));
    const double torque = ke_ * current - s_.viscous * w_ - load_torque_out / s_.gear_ratio;
    w_ += torque / j_ * dt;
    theta_motor_ += w_ * dt;
    const double gear_side = theta_motor_ / s_.gear_ratio;
    if (gear_side - theta_out_ > half_gap_) theta_out_ = gear_side - half_gap_;
    if (gear_side - theta_out_ < -half_gap_) theta_out_ = gear_side + half_gap_;
  }

  double motor_speed() const { return w_; }                       // rad/s at the motor shaft
  double output_speed() const { return w_ / s_.gear_ratio; }      // rad/s at the output shaft
  double gear_angle() const { return theta_motor_ / s_.gear_ratio; }  // where the gear side of the backlash is, in radians at the output
  double output_angle() const { return theta_out_; }              // where the output shaft is

 private:
  GearSpec s_;
  double ke_ = 0.0;
  double r_ = 0.0;
  double j_ = 0.0;
  double half_gap_ = 0.0;
  double w_ = 0.0;
  double theta_motor_ = 0.0;
  double theta_out_ = 0.0;
};


#endif
