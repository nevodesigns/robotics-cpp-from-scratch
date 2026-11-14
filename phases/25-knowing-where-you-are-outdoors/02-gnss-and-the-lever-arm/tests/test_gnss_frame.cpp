#include <rc/test/rc_test.hpp>
#include <sstream>
#include <cstring>

#include <algorithm>
#include <cmath>
#include <iostream>

#include "solution.hpp"
#include <iomanip>
#include <iostream>

// The numbers this lesson publishes: its page, its header and its quiz quote them, so each is held here as it was
// measured. A change that moves one by more than half a unit of its last printed digit fails the suite.
struct Pin {
  const char* key;
  double value;
};
static const Pin kPins[] = {
    {"deg_lat", 111194.92664455874},
    {"deg_lon", 68458.43258671739},
    {"no_cos_pct", 38.43385246743417},
    {"err_1km", 0.03551673032779945},
    {"err_10km", 3.5535433780205494},
    {"err_100km", 357.2291773498582},
    {"wander", 0.9999974219290664},
    {"antenna_speed", 0.4999936538282143},
    {"residual", 0.043619387365340524},
};

static void rec(const char* key, double value, int digits = 3) {
  std::cout << "@" << key << " " << std::fixed << std::setprecision(digits) << value << std::endl;
  std::cout.unsetf(std::ios::floatfield);
  for (const Pin& pin : kPins) {
    if (std::strcmp(pin.key, key) != 0) continue;
    double allowed = 0.5;
    for (int i = 0; i < digits; ++i) allowed /= 10.0;
    if (!(std::fabs(value - pin.value) <= allowed)) {
      std::ostringstream message;
      message << "the published number " << key << " was " << pin.value << " and is now " << value;
      rc::test::record(__FILE__, __LINE__, message.str());
    }
  }
}
[[maybe_unused]] static void recs(const char* key, const std::string& value) { std::cout << "@" << key << " " << value << std::endl; }


namespace {

const double kPi = 3.14159265358979323846;

}  // namespace

RC_TEST("a degree of latitude is 111 km, and a degree of longitude is that times the cosine of the latitude") {
  LocalFrame f;
  f.lat0 = 52.0;
  f.lon0 = 5.0;
  double e = 0.0, n = 0.0;
  to_local(f, 53.0, 5.0, e, n);
  rec("deg_lat", n, 0);
  RC_CHECK_NEAR(n, 111194.9, 1.0);
  RC_CHECK_NEAR(e, 0.0, 1e-9);
  to_local(f, 52.0, 6.0, e, n);
  rec("deg_lon", e, 0);
  RC_CHECK_NEAR(e, 111194.9 * std::cos(52.0 * kPi / 180.0), 1.0);
  rec("no_cos_pct", 100.0 * (1.0 - std::cos(52.0 * kPi / 180.0)), 0);
  RC_CHECK(e < 0.7 * 111194.9);
}

RC_TEST("the local frame and the fix convert back and forth") {
  LocalFrame f;
  f.lat0 = 52.0;
  f.lon0 = 5.0;
  double lat = 0.0, lon = 0.0, e = 0.0, n = 0.0;
  from_local(f, 123.4, -567.8, lat, lon);
  to_local(f, lat, lon, e, n);
  RC_CHECK_NEAR(e, 123.4, 1e-6);
  RC_CHECK_NEAR(n, -567.8, 1e-6);
}

RC_TEST("the flat approximation is good to centimetres at a kilometre and falls off with distance") {
  LocalFrame f;
  f.lat0 = 52.0;
  f.lon0 = 5.0;
  const double distances[3] = {1000.0, 10000.0, 100000.0};
  double errs[3];
  for (int i = 0; i < 3; ++i) {
    // A point a distance north-east of the origin.
    double lat = 0.0, lon = 0.0;
    from_local(f, distances[i] / std::sqrt(2.0), distances[i] / std::sqrt(2.0), lat, lon);
    errs[i] = std::fabs(haversine(f.lat0, f.lon0, lat, lon) - distances[i]);
  }
  rec("err_1km", errs[0], 3);
  rec("err_10km", errs[1], 2);
  rec("err_100km", errs[2], 0);
  RC_CHECK(errs[0] < 0.05);
  RC_CHECK(errs[1] < 5.0);
  RC_CHECK(errs[2] > errs[1] && errs[1] > errs[0]);
}

RC_TEST("the antenna is not the robot: it circles when the robot turns on the spot, and the lever arm brings the point back") {
  Lever l;
  l.x = 0.4;
  l.y = 0.3;
  const double lever = std::hypot(l.x, l.y);
  double lo_x = 1e9, hi_x = -1e9, lo_y = 1e9, hi_y = -1e9;
  double c_lo_x = 1e9, c_hi_x = -1e9, c_lo_y = 1e9, c_hi_y = -1e9;
  double r5_x = 0.0, r5_y = 0.0;
  double speed = 0.0, last_x = 0.0, last_y = 0.0;
  for (int k = 0; k <= 360; ++k) {
    const double heading = k * kPi / 180.0;
    // The robot is at (10, 20) and does not move; the antenna is at the reference point plus the lever turned.
    const double ax = 10.0 + std::cos(heading) * l.x - std::sin(heading) * l.y;
    const double ay = 20.0 + std::sin(heading) * l.x + std::cos(heading) * l.y;
    lo_x = std::min(lo_x, ax); hi_x = std::max(hi_x, ax); lo_y = std::min(lo_y, ay); hi_y = std::max(hi_y, ay);
    double rx = 0.0, ry = 0.0;
    antenna_to_reference(ax, ay, heading, l, rx, ry);
    c_lo_x = std::min(c_lo_x, rx); c_hi_x = std::max(c_hi_x, rx); c_lo_y = std::min(c_lo_y, ry); c_hi_y = std::max(c_hi_y, ry);
    double bx = 0.0, by = 0.0;
    antenna_to_reference(ax, ay, heading + 5.0 * kPi / 180.0, l, bx, by);  // the heading wrong by 5 degrees
    r5_x = std::max(r5_x, std::hypot(bx - 10.0, by - 20.0));
    if (k > 0) speed = std::max(speed, std::hypot(ax - last_x, ay - last_y) / (kPi / 180.0));  // per radian: at 1 rad/s this is m/s
    last_x = ax;
    last_y = ay;
  }
  const double wander = std::max(hi_x - lo_x, hi_y - lo_y);
  rec("wander", wander, 2);
  rec("antenna_speed", speed, 2);
  RC_CHECK_NEAR(wander, 2.0 * lever, 0.01);
  RC_CHECK(c_hi_x - c_lo_x < 1e-9 && c_hi_y - c_lo_y < 1e-9);
  const double residual = r5_x;
  (void)r5_y;
  rec("residual", residual, 3);
  RC_CHECK_NEAR(residual, lever * std::tan(5.0 * kPi / 180.0) * 1.0, 0.004);
  RC_CHECK_NEAR(speed, lever, 0.01);
}
