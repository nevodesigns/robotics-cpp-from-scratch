#include <rc/test/rc_test.hpp>
#include <sstream>
#include <cstring>

#include <cmath>
#include <iostream>
#include <limits>

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
    {"m_lidar", 3.0},
    {"m_imu", 2.0},
    {"m_gnss", 1.0},
    {"m_two", 2.0},
    {"flaps_raw", 60.0},
    {"flaps_hys", 1.0},
    {"t_back", 30.100000000000232},
    {"dist_stop", 468.22916666666663},
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


RC_TEST("each fault asks for the mode it should, and two faults ask for more than either") {
  const ModePolicy p;
  VehicleHealth h;
  RC_CHECK(required_mode(h, p) == kFull);
  h = VehicleHealth();
  h.scanner_ok = false;
  const Mode lidar = required_mode(h, p);
  h = VehicleHealth();
  h.gyro_ok = false;
  const Mode imu = required_mode(h, p);
  h = VehicleHealth();
  h.gnss_ok = false;
  const Mode gnss = required_mode(h, p);
  h = VehicleHealth();
  h.wheels_ok = false;
  const Mode wheels = required_mode(h, p);
  h = VehicleHealth();
  h.link_ok = false;
  const Mode link = required_mode(h, p);
  h = VehicleHealth();
  h.gnss_ok = false;
  h.link_ok = false;
  const Mode two = required_mode(h, p);
  rec("m_lidar", static_cast<int>(lidar), 0);
  rec("m_imu", static_cast<int>(imu), 0);
  rec("m_gnss", static_cast<int>(gnss), 0);
  rec("m_two", static_cast<int>(two), 0);
  RC_CHECK(lidar == kSafeStop);
  RC_CHECK(imu == kCreep);
  RC_CHECK(gnss == kReduced && wheels == kReduced && link == kReduced);
  RC_CHECK(two == kCreep);
}

RC_TEST("the sigma of the position moves the mode on, and a sigma that is not a number is a stop") {
  const ModePolicy p;
  VehicleHealth h;
  h.sigma_m = 0.2;
  RC_CHECK(required_mode(h, p) == kFull);
  h.sigma_m = 0.5;
  RC_CHECK(required_mode(h, p) == kReduced);
  h.sigma_m = 1.0;
  RC_CHECK(required_mode(h, p) == kCreep);
  h.sigma_m = 2.0;
  RC_CHECK(required_mode(h, p) == kSafeStop);
  h.sigma_m = std::numeric_limits<double>::quiet_NaN();
  RC_CHECK(required_mode(h, p) == kSafeStop);
}

RC_TEST("a requirement that flaps is not followed up and down") {
  const ModePolicy p;
  ModeManager m(p);
  int raw = 0;
  Mode last_raw = kFull;
  for (double t = 0.0; t < 120.0; t += 0.1) {
    const Mode req = (static_cast<int>(t / 2.0) % 2 == 0) ? kReduced : kFull;
    if (req != last_raw) {
      ++raw;
      last_raw = req;
    }
    m.update(t, req);
  }
  rec("flaps_raw", raw, 0);
  rec("flaps_hys", m.changes(), 0);
  RC_CHECK(raw >= 50);
  RC_CHECK(m.changes() <= 2);
  RC_CHECK(m.mode() == kReduced);
}

RC_TEST("a worse requirement is taken at once, and a better one one step at a time after the dwell") {
  const ModePolicy p;
  ModeManager m(p);
  double t = 0.0;
  const double dt = 0.1;
  for (; t < 5.0; t += dt) m.update(t, kFull);
  RC_CHECK(m.update(t, kSafeStop) == kSafeStop);  // at once
  const double cleared = t;
  double back = -1.0;
  Mode seen[4] = {kSafeStop, kSafeStop, kSafeStop, kSafeStop};
  int steps = 0;
  for (; t < cleared + 100.0; t += dt) {
    const Mode before = m.mode();
    const Mode now = m.update(t, kFull);
    if (now != before && steps < 4) seen[steps++] = now;
    if (now == kFull) {
      back = t - cleared;
      break;
    }
  }
  rec("t_back", back, 0);
  RC_CHECK(steps == 3);
  RC_CHECK(seen[0] == kCreep && seen[1] == kReduced && seen[2] == kFull);
  RC_CHECK_NEAR(back, 3.0 * p.dwell_s, 0.5);
}

RC_TEST("with no GNSS the position goes as the square root of the distance, and the vehicle is stopped when it passes its limit") {
  const double sigma0 = 0.05, k = std::sqrt(0.0048), limit = 1.5;
  const double d = dead_reckoning_distance(sigma0, k, limit);
  rec("dist_stop", d, 0);
  RC_CHECK_NEAR(std::sqrt(sigma0 * sigma0 + k * k * d), limit, 1e-9);
  const ModePolicy p;
  VehicleHealth h;
  h.gnss_ok = false;
  h.sigma_m = std::sqrt(sigma0 * sigma0 + k * k * (d * 0.99));
  RC_CHECK(required_mode(h, p) == kCreep);
  h.sigma_m = std::sqrt(sigma0 * sigma0 + k * k * (d * 1.01));
  RC_CHECK(required_mode(h, p) == kSafeStop);
  RC_CHECK_NEAR(dead_reckoning_distance(1.0, k, 0.5), 0.0, 1e-12);
}

RC_TEST("the speed caps fall with the mode and a safe stop is zero") {
  const ModePolicy p;
  RC_CHECK(mode_speed_cap(kFull, p) > mode_speed_cap(kReduced, p));
  RC_CHECK(mode_speed_cap(kReduced, p) > mode_speed_cap(kCreep, p));
  RC_CHECK_NEAR(mode_speed_cap(kSafeStop, p), 0.0, 1e-12);
}
