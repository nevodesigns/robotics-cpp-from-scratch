#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>

const double kEarthRadius = 6371000.0;
const double kDegrees = 3.14159265358979323846 / 180.0;

// A point on the earth taken as the origin of a local frame, in degrees.
struct LocalFrame {
  double lat0 = 0.0;
  double lon0 = 0.0;
};

// Metres east and north of the origin for a fix in degrees: a flat map made at the origin's latitude, good for a few kilometres.
inline void to_local(const LocalFrame& f, double lat, double lon, double& east, double& north) {
  north = kEarthRadius * (lat - f.lat0) * kDegrees;
  east = kEarthRadius * std::cos(f.lat0 * kDegrees) * (lon - f.lon0) * kDegrees;
}

inline void from_local(const LocalFrame& f, double east, double north, double& lat, double& lon) {
  lat = f.lat0 + north / (kEarthRadius * kDegrees);
  lon = f.lon0 + east / (kEarthRadius * std::cos(f.lat0 * kDegrees) * kDegrees);
}

// The distance along the surface of the sphere between two fixes, in metres.
inline double haversine(double lat1, double lon1, double lat2, double lon2) {
  const double a1 = lat1 * kDegrees, a2 = lat2 * kDegrees;
  const double dlat = a2 - a1, dlon = (lon2 - lon1) * kDegrees;
  const double h = std::sin(dlat / 2.0) * std::sin(dlat / 2.0) + std::cos(a1) * std::cos(a2) * std::sin(dlon / 2.0) * std::sin(dlon / 2.0);
  return 2.0 * kEarthRadius * std::asin(std::sqrt(h));
}

// Where the antenna sits on the robot: ahead and to the left of its reference point, in metres.
struct Lever {
  double x = 0.0;
  double y = 0.0;
};

// The position of the robot's reference point from the antenna's position (in the local frame) and the robot's heading: the antenna is the reference point plus the lever turned by the heading, so the reference point is the antenna less that.
inline void antenna_to_reference(double ax, double ay, double heading, const Lever& l, double& rx, double& ry) {
  (void)ax;  // TODO: the antenna is at the reference point plus the lever turned by the heading (x' = cos h lx - sin h ly, y' = sin h lx + cos h ly); take that off
  (void)ay;
  (void)heading;
  (void)l;
  rx = 0.0;
  ry = 0.0;
}


#endif
