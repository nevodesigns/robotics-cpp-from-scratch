// rc/drive/range.hpp
//
// How far it can go and how hot it gets, from lesson 27-04.
//
// route_wh gives the energy of a route over a terrain grid (lesson 26-09) in watt hours: the work against rolling resistance and the slope along each step, divided by the efficiency of the drive, plus the constant power of the computer and sensors over the time the route takes at a speed. must_turn_back says the vehicle should go home now: the charge left is less than the energy of the way home plus a reserve. slope_force is the force a slope asks of a vehicle; motor_current the current each motor must carry for it; time_to_limit, in closed form, how long a motor takes to rise by a temperature given the heat it makes, its thermal resistance and its heat capacity (never, if the steady rise is under the limit). The motor values are typical of a 37 mm class gear motor and are marked as such.
//
// A metre of tarmac cost 10.1 J for a 10 kg vehicle with an 8 W computer at 1 m/s, of which 79 percent was the computer, and a metre of sand 22.0 J. A vehicle that estimated the way home as flat turned back 16.8 m out and was 25 J short of getting home over the sand band between; one that planned the way home over the ground turned back 12.0 m out and got home with 71 J to spare. A slope of 14 degrees heated a motor to a steady rise of 71 K, under the limit of 75 K, and one of 20 degrees reached the limit in 62 s, which at 0.5 m/s is 31 m of climb.

#ifndef RC_RC_DRIVE_RANGE_HPP
#define RC_RC_DRIVE_RANGE_HPP

#include <cmath>
#include <cstddef>
#include <rc/drive/offroad.hpp>
#include <vector>

namespace rc {
namespace drive {

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


}  // namespace drive
}  // namespace rc

#endif
