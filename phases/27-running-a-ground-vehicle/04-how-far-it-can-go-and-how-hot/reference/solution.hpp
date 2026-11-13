#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <cstddef>
#include <rc/drive/offroad.hpp>
#include <vector>

using namespace rc::drive;

struct DrivePower {
  double mass = 10.0;      // kg
  double efficiency = 0.7; // from the battery to the work done at the wheels (typical)
  double idle_w = 8.0;     // W of the computer and sensors, always on (typical)
  double speed = 1.0;      // m/s the route is driven at
};

// The energy of a path over a terrain, in watt hours: the work against the ground and the slope over the drive's efficiency, plus the idle power over the time.
inline double route_wh(const TerrainGrid& terrain, const std::vector<CarPose>& path, const DrivePower& d) {
  double joules = 0.0;
  double length = 0.0;
  for (std::size_t k = 1; k < path.size(); ++k)
    length += std::hypot(path[k].x - path[k - 1].x, path[k].y - path[k - 1].y);
  joules = path_energy(terrain, path, d.mass) / d.efficiency + d.idle_w * (length / d.speed);
  return joules / 3600.0;
}

// Turn back when what is left would not cover the way home and a reserve.
inline bool must_turn_back(double soc_wh, double home_wh, double reserve_wh) { return soc_wh < home_wh + reserve_wh; }

// The force a slope asks of a vehicle going up it at a steady speed, newtons.
inline double slope_force(double mass, double angle_rad, double crr) { return mass * 9.81 * (std::sin(angle_rad) + crr * std::cos(angle_rad)); }

// The current each of n motors carries to give a force at wheels of a radius through a drive of an efficiency; kt_out is N m per ampere at the output shaft.
inline double motor_current(double force, double wheel_radius, int motors, double efficiency, double kt_out) {
  return force * wheel_radius / (static_cast<double>(motors) * efficiency * kt_out);
}

struct MotorThermal {
  double resistance = 4.0;  // ohm, the winding (typical of a 12 V 37 mm gear motor)
  double kt_out = 0.25;     // N m per A at the output shaft (typical)
  double r_th = 6.0;        // K per W from the winding to the air (typical)
  double c_th = 12.0;       // J per K, the heat capacity of the winding and the can (typical)
  double rise_limit = 75.0; // K above the air that the winding may reach (typical)
};

// The steady rise above the air for a current, K.
inline double steady_rise(double current, const MotorThermal& m) { return current * current * m.resistance * m.r_th; }

// Seconds for a motor carrying a current to reach the rise limit from cold, in closed form; a very large number if it never does.
inline double time_to_limit(double current, const MotorThermal& m) {
  const double steady = steady_rise(current, m);
  if (steady <= m.rise_limit) return 1.0e18;
  return -m.r_th * m.c_th * std::log(1.0 - m.rise_limit / steady);
}


#endif
