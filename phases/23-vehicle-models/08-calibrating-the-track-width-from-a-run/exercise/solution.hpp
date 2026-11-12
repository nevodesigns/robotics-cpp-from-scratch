#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <cstddef>
#include <vector>

using namespace rc::vehicle;

// One spin in place: how far apart the right and left wheels rolled in total (metres, positive when the right ones went further), and the heading change a gyro measured over it (radians).
struct TurnRun {
  double wheel_diff = 0.0;
  double dtheta = 0.0;
};

// The effective track width that makes the wheels' turn the measured one: the total of the wheel differences over the total of the heading changes, both taken without their signs. Zero if nothing turned.
inline double calibrate_effective_track(const std::vector<TurnRun>& runs) {
  (void)runs;  // TODO: add up the absolute wheel differences and the absolute heading changes of all the runs, and divide the first by the second
  return 0.0;
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


#endif
