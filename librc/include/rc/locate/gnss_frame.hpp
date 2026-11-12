// rc/locate/gnss_frame.hpp
//
// GNSS in the robot's frame, from lesson 25-02.
//
// LocalFrame is an origin in degrees of latitude and longitude; to_local gives the metres east and north of it for a fix, as R cos(lat0) dlon and R dlat on a sphere of 6 371 000 m, and from_local gives the fix back. haversine is the great-circle distance between two fixes, to check against. Lever is where the antenna is in the robot's frame (x ahead, y to the left); antenna_to_reference gives the position of the robot's reference point from the position of the antenna and the heading, p - R(theta) l.
//
// One degree of latitude was 111195 m and one of longitude at 52 degrees north 68458 m (38 percent less); the flat approximation was 0.036 m off the great circle at 1 km, 3.55 m at 10 km and 357 m at 100 km; a robot turning on the spot with its antenna 0.5 m from its centre had an antenna that moved 0.50 m/s and, uncorrected, a position that wandered 1.00 m, which a heading known to 5 degrees brought down to 0.044 m.

#ifndef RC_RC_LOCATE_GNSS_FRAME_HPP
#define RC_RC_LOCATE_GNSS_FRAME_HPP

#include <cmath>

namespace rc {
namespace locate {

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
  rx = ax - (std::cos(heading) * l.x - std::sin(heading) * l.y);
  ry = ay - (std::sin(heading) * l.x + std::cos(heading) * l.y);
}


}  // namespace locate
}  // namespace rc

#endif
