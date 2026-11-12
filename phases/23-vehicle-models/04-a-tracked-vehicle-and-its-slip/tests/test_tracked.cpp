#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iostream>

#include "solution.hpp"
#include <iomanip>
#include <iostream>

static void rec(const char* key, double value, int digits = 3) {
  std::cout << "@" << key << " " << std::fixed << std::setprecision(digits) << value << std::endl;
  std::cout.unsetf(std::ios::floatfield);
}
[[maybe_unused]] static void recs(const char* key, const std::string& value) { std::cout << "@" << key << " " << value << std::endl; }


namespace {

const double kPi = 3.14159265358979323846;

}  // namespace

RC_TEST("the thrust is zero with no slip, rises with it, and tends to the largest the soil can give") {
  Soil sand;
  Track track;
  RC_CHECK(track_thrust(sand, track, 0.0) == 0.0);
  double last = 0.0;
  for (int k = 1; k <= 100; ++k) {
    const double f = track_thrust(sand, track, 0.01 * k);
    RC_CHECK(f > last);
    last = f;
  }
  const double fmax = max_thrust(sand, track);
  RC_CHECK_NEAR(fmax, 1000.0 * 0.4 * 0.08 + 98.1 * std::tan(0.4887), 1e-9);
  RC_CHECK(track_thrust(sand, track, 1.0) < fmax);
  RC_CHECK(track_thrust(sand, track, 1.0) > 0.9 * fmax);
  // For a small slip the curve is a straight line of slope F_max * L / (2 K).
  const double small = 1e-4;
  RC_CHECK_NEAR(track_thrust(sand, track, small), fmax * small * 0.4 / (2.0 * 0.025), 2e-3 * fmax * small * 0.4 / (2.0 * 0.025));
}

RC_TEST("half the largest thrust comes at a slip of ten percent on a loose sand, and the slip for a thrust is the inverse") {
  Soil sand;
  Track track;
  const double fmax = max_thrust(sand, track);
  const double half = slip_for_thrust(sand, track, 0.5 * fmax);
  rec("half_slip", half * 100.0, 1);
  RC_CHECK_NEAR(track_thrust(sand, track, half), 0.5 * fmax, 1e-6);
  RC_CHECK_NEAR(half, 0.10, 0.005);
  RC_CHECK(slip_for_thrust(sand, track, 2.0 * fmax) == 1.0);
  RC_CHECK_NEAR(track_thrust(sand, track, slip_for_thrust(sand, track, 20.0)), 20.0, 1e-6);
  // Firmer ground gives more thrust for the same slip.
  Soil firm;
  firm.cohesion = 4000.0;
  firm.friction_angle = 0.5236;
  firm.shear_modulus = 0.015;
  RC_CHECK(track_thrust(firm, track, 0.05) > track_thrust(sand, track, 0.05));
}

RC_TEST("a pivot costs a track well under half of what a four wheeled chassis of the same size needs, and a slip of a few percent") {
  const double mass = 20.0, mu = 0.5, length = 0.40, gauge = 0.30;
  const double tracked = pivot_thrust_tracked(mass, mu, length, gauge);
  const double wheeled = mu * mass * 9.81 * std::sqrt(length * length + gauge * gauge) / (2.0 * gauge);
  rec("pivot_tracked", tracked, 1);
  rec("pivot_wheeled", wheeled, 1);
  RC_CHECK_NEAR(tracked, 0.5 * 20.0 * 9.81 * 0.4 / (4.0 * 0.3), 1e-9);
  RC_CHECK(tracked < wheeled);
  Soil sand;
  Track track;
  const double slip = slip_for_thrust(sand, track, tracked);
  rec("pivot_slip", slip * 100.0, 1);
  RC_CHECK(slip > 0.0 && slip < 0.2);
  RC_CHECK(tracked < max_thrust(sand, track));
}

RC_TEST("one track slipping more than the other turns a vehicle that was told to go straight") {
  TrackedDrive d(0.05, 0.30, 1.3);
  const double w = 6.0;  // rad/s: 0.3 m/s at the sprocket
  for (int i = 0; i < 10000; ++i) d.step(w, w, 0.10, 0.20, 0.001);
  const double deg = d.pose().theta * 180.0 / kPi;
  rec("veer", std::fabs(deg), 1);
  std::cout << std::fixed << std::setprecision(1) << "  both tracks at 0.3 m/s, slips of 10 and 20 percent: heading " << deg << " degrees after 10 s, " << d.pose().x << " m along\n";
  RC_CHECK(deg < -20.0);
  RC_CHECK_NEAR(deg, (0.3 * 0.8 - 0.3 * 0.9) / (1.3 * 0.30) * 10.0 * 180.0 / kPi, 0.5);
  TrackedDrive even(0.05, 0.30, 1.3);
  for (int i = 0; i < 10000; ++i) even.step(w, w, 0.15, 0.15, 0.001);
  RC_CHECK_NEAR(even.pose().theta, 0.0, 1e-12);
}
