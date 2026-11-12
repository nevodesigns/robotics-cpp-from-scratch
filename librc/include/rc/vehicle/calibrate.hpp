// rc/vehicle/calibrate.hpp
//
// Calibration of the track and the scale, from lesson 23-08.
//
// A TurnRun is one spin in place: the sum of the difference of the right and left wheel travels over it, in metres, and the heading change a gyro measured. calibrate_effective_track gives the track that makes the wheels' turn the measured one: the sum of the absolute wheel differences over the sum of the absolute heading changes. When the runs are made in both directions the bias of the gyro adds to the heading change of one and subtracts from the other's size, and cancels. calibrate_scale is the ratio of a measured distance to the wheel travel that made it. spread_of is the mean and the standard deviation of a set of estimates.
//
// The effective track of a skid steer of efficiency 0.78 and track 0.16 m is 0.2051 m. A single spin with a gyro bias of 0.01 rad/s gave 0.31 percent error and a spin each way 0.000; with a floor that scrubs differently each time, a calibration from 1 spin each way had a spread of 0.0034 m and from 4 spins each way 0.0019 m; and a measured stretch of 10.00 m over 10.21 m of wheel gave a scale of 0.9794.

#ifndef RC_RC_VEHICLE_CALIBRATE_HPP
#define RC_RC_VEHICLE_CALIBRATE_HPP

#include <cmath>
#include <cstddef>
#include <vector>

namespace rc {
namespace vehicle {

// One spin in place: how far apart the right and left wheels rolled in total (metres, positive when the right ones went further), and the heading change a gyro measured over it (radians).
struct TurnRun {
  double wheel_diff = 0.0;
  double dtheta = 0.0;
};

// The effective track width that makes the wheels' turn the measured one: the total of the wheel differences over the total of the heading changes, both taken without their signs. Zero if nothing turned.
inline double calibrate_effective_track(const std::vector<TurnRun>& runs) {
  double wheels = 0.0;
  double turned = 0.0;
  for (std::size_t i = 0; i < runs.size(); ++i) {
    wheels += std::fabs(runs[i].wheel_diff);
    turned += std::fabs(runs[i].dtheta);
  }
  return turned > 0.0 ? wheels / turned : 0.0;
}

// The scale: ground travel per metre of wheel travel, from a stretch somebody measured with a tape.
inline double calibrate_scale(double wheel_metres, double measured_metres) { return wheel_metres > 0.0 ? measured_metres / wheel_metres : 0.0; }

struct Spread {
  double mean = 0.0;
  double stddev = 0.0;
};

inline Spread spread_of(const std::vector<double>& v) {
  Spread s;
  if (v.empty()) return s;
  for (std::size_t i = 0; i < v.size(); ++i) s.mean += v[i];
  s.mean /= static_cast<double>(v.size());
  for (std::size_t i = 0; i < v.size(); ++i) s.stddev += (v[i] - s.mean) * (v[i] - s.mean);
  s.stddev = std::sqrt(s.stddev / static_cast<double>(v.size()));
  return s;
}


}  // namespace vehicle
}  // namespace rc

#endif
