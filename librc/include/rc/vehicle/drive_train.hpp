// rc/vehicle/drive_train.hpp
//
// The drive train of a vehicle, from lesson 23-06.
//
// A DriveTrain is a number of identical gear motors (the GearSpec of lesson 22-04), a gearbox efficiency, a wheel radius, a mass and a battery with an internal resistance. motor_force gives the force at the wheels at a speed and a bus voltage from the torque-speed line of the motor, with the driver's current limit; bus_voltage finds the voltage the battery holds up while it feeds that current; resistance is the rolling resistance, the pull of the slope and the drag of the air; top_speed is where the force and the resistance are equal, max_accel_motors is their difference over the mass, and max_climb is the slope at which the standstill force is all spent against gravity. achievable_accel is the lower of that and the grip of lesson 23-05, and says which it was.
//
// The reference vehicle (four gear motors of 12 V, 350 rpm and 0.7 N m at a 0.05 m wheel, 10 kg, a battery of 0.05 ohm) had a no load speed of 1.83 m/s, a top speed on level gravel of 1.67 m/s and on a slope of 10 degrees of 0.99; it could climb 25.6 degrees by its motors, against 22.3 degrees by the grip of gravel and 38.1 on tarmac; a battery of 0.5 ohm cut its standstill acceleration from 4.54 to 3.32 m/s2; and the limit was the motors on tarmac, the grip on gravel and the grip on ice.

#ifndef RC_RC_VEHICLE_DRIVE_TRAIN_HPP
#define RC_RC_VEHICLE_DRIVE_TRAIN_HPP

#include <algorithm>
#include <cmath>
#include <rc/field/gear_motor.hpp>
#include <rc/field/terrain.hpp>
#include <rc/vehicle/traction.hpp>

namespace rc {
namespace vehicle {

// A drive train: identical motors driving the wheels through a gearbox, from a battery that sags under load.
struct DriveTrain {
  rc::field::GearSpec motor;
  int motors = 4;
  double wheel_radius = 0.05;
  double mass = 10.0;
  double gearbox_efficiency = 0.85;    // typical of a small spur gearbox
  double battery_volts = 12.0;
  double battery_resistance = 0.05;    // ohms, internal (typical)
  double drag_area = 0.05;             // Cd times the frontal area, m2 (typical)
};

// The constants of the motor, worked out as lesson 22-04 does from the data sheet numbers.
struct MotorConstants {
  double ke = 0.0;       // back voltage constant, V s/rad
  double resistance = 0.0;
};

inline MotorConstants motor_constants(const rc::field::GearSpec& s) {
  const double pi = 3.14159265358979323846;
  const double w_nl = s.no_load_rpm * 2.0 * pi / 60.0 * s.gear_ratio;
  const double stall = s.stall_torque / s.gear_ratio;
  MotorConstants m;
  m.ke = s.volts / w_nl - s.viscous * s.volts / stall;
  m.resistance = s.volts * m.ke / stall;
  return m;
}

// The current one motor draws at a speed of the vehicle and a bus voltage, held to the driver's limit.
inline double motor_current(const DriveTrain& d, double speed, double bus_volts) {
  const MotorConstants k = motor_constants(d.motor);
  const double w_motor = speed / d.wheel_radius * d.motor.gear_ratio;
  const double i = (bus_volts - k.ke * w_motor) / k.resistance;
  return std::max(-d.motor.max_current, std::min(d.motor.max_current, i));
}

// The force at the wheels, all motors together, at a speed and a bus voltage.
inline double motor_force(const DriveTrain& d, double speed, double bus_volts) {
  const MotorConstants k = motor_constants(d.motor);
  const double w_motor = speed / d.wheel_radius * d.motor.gear_ratio;
  const double torque = k.ke * motor_current(d, speed, bus_volts) - d.motor.viscous * w_motor;
  return d.motors * torque * d.motor.gear_ratio * d.gearbox_efficiency / d.wheel_radius;
}

// The voltage the battery holds up while it feeds the motors at that speed: it sags by the internal resistance times the current, and the current depends on the voltage, so it is found by repeating until it settles.
inline double bus_voltage(const DriveTrain& d, double speed) {
  double v = d.battery_volts;
  for (int i = 0; i < 40; ++i) {
    const double total = d.motors * std::fabs(motor_current(d, speed, v));
    v = d.battery_volts - d.battery_resistance * total;
  }
  return v;
}

inline double drive_force(const DriveTrain& d, double speed) { return motor_force(d, speed, bus_voltage(d, speed)); }

// What pushes back: rolling resistance and the pull of the slope along the heading (from the terrain cell), and the drag of the air.
inline double resistance(const DriveTrain& d, const rc::field::Cell& cell, double heading, double speed) {
  const rc::field::Traction t = rc::field::traction(cell, heading);
  const double w = d.mass * 9.81;
  return w * (cell.surface.crr * std::cos(t.angle) + std::sin(t.angle)) + 0.5 * 1.2 * d.drag_area * speed * speed;
}

// The speed at which the motors' force and the resistance are equal, by bisection. Zero if they cannot move the vehicle at all.
inline double top_speed(const DriveTrain& d, const rc::field::Cell& cell, double heading) {
  const MotorConstants k = motor_constants(d.motor);
  double lo = 0.0;
  double hi = d.battery_volts / k.ke / d.motor.gear_ratio * d.wheel_radius;
  if (drive_force(d, 0.0) <= resistance(d, cell, heading, 0.0)) return 0.0;
  for (int i = 0; i < 60; ++i) {
    const double mid = 0.5 * (lo + hi);
    if (drive_force(d, mid) > resistance(d, cell, heading, mid)) lo = mid;
    else hi = mid;
  }
  return 0.5 * (lo + hi);
}

inline double max_accel_motors(const DriveTrain& d, const rc::field::Cell& cell, double heading, double speed) {
  return (drive_force(d, speed) - resistance(d, cell, heading, speed)) / d.mass;
}

// The steepest slope (radians) the motors can start on, with the rolling resistance: the standstill force is all spent against gravity.
inline double max_climb(const DriveTrain& d, double crr) {
  const double ratio = drive_force(d, 0.0) / (d.mass * 9.81);
  // sin(a) + crr cos(a) = ratio, solved by bisection.
  double lo = 0.0;
  double hi = 1.5707963;
  for (int i = 0; i < 60; ++i) {
    const double mid = 0.5 * (lo + hi);
    if (std::sin(mid) + crr * std::cos(mid) < ratio) lo = mid;
    else hi = mid;
  }
  return 0.5 * (lo + hi);
}

enum class Limit { motors, grip };

struct Achievable {
  double accel = 0.0;
  Limit binding = Limit::motors;
};

// The most the vehicle can speed up in a straight line: the lower of what the motors give and what the grip holds.
inline Achievable achievable_accel(const DriveTrain& d, const rc::field::Cell& cell, double heading, double speed) {
  Achievable a;
  const double motors = max_accel_motors(d, cell, heading, speed);
  const double grip = traction_budget(cell, heading, speed, 0.0).drive;
  a.accel = std::min(motors, grip);
  a.binding = motors <= grip ? Limit::motors : Limit::grip;
  return a;
}


}  // namespace vehicle
}  // namespace rc

#endif
