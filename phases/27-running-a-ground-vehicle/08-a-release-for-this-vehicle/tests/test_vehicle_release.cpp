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
    {"reasons_three", 3.0},
    {"turn_ratio", 1.6},
    {"fail_wheel", 1.0},
    {"fail_odom", 1.0},
    {"boots_good", 1.0},
    {"boots_bad", 4.0},
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

OnBoard vehicle() {
  OnBoard v;
  v.id = "ugv-07";
  v.vehicle_class = "skid-10kg";
  v.software = "1.4.2";
  v.config_schema = 2;
  v.map_version = 5;
  v.parked = true;
  v.mission_active = false;
  v.soc = 0.9;
  return v;
}

Bundle bundle() {
  Bundle b;
  b.vehicle_class = "skid-10kg";
  b.software = "1.5.0";
  b.config_schema = 2;
  b.map_version = 5;
  b.calibration_for = "ugv-07";
  return b;
}

}  // namespace

RC_TEST("a good bundle for a parked, charged vehicle is accepted, and one that carries no calibration is too") {
  RC_CHECK(check_update(vehicle(), bundle(), UpdatePolicy()).empty());
  Bundle b = bundle();
  b.calibration_for = "";
  RC_CHECK(check_update(vehicle(), b, UpdatePolicy()).empty());
}

RC_TEST("each reason refuses on its own, with the right name") {
  const UpdatePolicy p;
  {
    Bundle b = bundle();
    b.vehicle_class = "car-20kg";
    const std::vector<Refusal> r = check_update(vehicle(), b, p);
    RC_CHECK(r.size() == 1 && r[0] == kWrongClass);
  }
  {
    Bundle b = bundle();
    b.calibration_for = "ugv-03";
    const std::vector<Refusal> r = check_update(vehicle(), b, p);
    RC_CHECK(r.size() == 1 && r[0] == kWrongCalibration);
  }
  {
    Bundle b = bundle();
    b.config_schema = 1;
    const std::vector<Refusal> r = check_update(vehicle(), b, p);
    RC_CHECK(r.size() == 1 && r[0] == kOlderSchema);
  }
  {
    Bundle b = bundle();
    b.map_version = 4;
    const std::vector<Refusal> r = check_update(vehicle(), b, p);
    RC_CHECK(r.size() == 1 && r[0] == kOlderMap);
  }
  {
    OnBoard v = vehicle();
    v.parked = false;
    const std::vector<Refusal> r = check_update(v, bundle(), p);
    RC_CHECK(r.size() == 1 && r[0] == kNotParked);
  }
  {
    OnBoard v = vehicle();
    v.mission_active = true;
    const std::vector<Refusal> r = check_update(v, bundle(), p);
    RC_CHECK(r.size() == 1 && r[0] == kMissionActive);
  }
  {
    OnBoard v = vehicle();
    v.soc = std::numeric_limits<double>::quiet_NaN();
    const std::vector<Refusal> r = check_update(v, bundle(), p);
    RC_CHECK(r.size() == 1 && r[0] == kLowCharge);
  }
  {
    Bundle b = bundle();
    b.software = "1.4.0";
    const std::vector<Refusal> r = check_update(vehicle(), b, p);
    RC_CHECK(r.size() == 1 && r[0] == kVersion);
    UpdatePolicy back = p;
    back.rollback_authorised = true;
    RC_CHECK(check_update(vehicle(), b, back).empty());
  }
}

RC_TEST("every reason is listed, not just the first") {
  OnBoard v = vehicle();
  v.parked = false;
  v.mission_active = true;
  v.soc = 0.2;
  const std::vector<Refusal> r = check_update(v, bundle(), UpdatePolicy());
  rec("reasons_three", static_cast<double>(r.size()), 0);
  RC_CHECK(r.size() == 3);
}

RC_TEST("another vehicle's calibration is refused, and what it would have done is a turn that is off by the ratio of the tracks") {
  Bundle b = bundle();
  b.calibration_for = "ugv-03";
  RC_CHECK(!check_update(vehicle(), b, UpdatePolicy()).empty());
  // A skid steer turns at r (wr - wl) / (chi B): with B taken as 0.25 when it is 0.40 it turns this much less than asked.
  const double ratio = 0.40 / 0.25;
  rec("turn_ratio", ratio, 2);
  RC_CHECK_NEAR(ratio, 1.6, 1e-12);
}

RC_TEST("the self test names what is wrong") {
  SelfTestInput good;
  good.forward_moved_m = 1.0;
  good.odom_over_ground = 1.01;
  good.estop_distance_m = 0.6;
  good.scanner_saw_target = true;
  RC_CHECK(post_update_selftest(good, SelfTestLimits()).empty());
  SelfTestInput wheel = good;
  wheel.forward_moved_m = -0.3;
  const std::vector<std::string> a = post_update_selftest(wheel, SelfTestLimits());
  rec("fail_wheel", static_cast<double>(a.size()), 0);
  RC_CHECK(a.size() == 1 && a[0] == "wheel direction");
  SelfTestInput odom = good;
  odom.odom_over_ground = 0.8;
  const std::vector<std::string> b = post_update_selftest(odom, SelfTestLimits());
  rec("fail_odom", static_cast<double>(b.size()), 0);
  RC_CHECK(b.size() == 1 && b[0] == "odometry");
  SelfTestInput nan = good;
  nan.estop_distance_m = std::numeric_limits<double>::quiet_NaN();
  nan.forward_moved_m = std::numeric_limits<double>::quiet_NaN();
  RC_CHECK(post_update_selftest(nan, SelfTestLimits()).size() == 2);
}

RC_TEST("a release that passes is confirmed at its first boot, and one that fails is rolled back to the old slot") {
  const Settled good = boots_until_settled(0, 1, 3, true);
  const Settled bad = boots_until_settled(0, 1, 3, false);
  rec("boots_good", good.boots, 0);
  rec("boots_bad", bad.boots, 0);
  RC_CHECK(good.confirmed && good.slot == 1 && good.boots == 1);
  RC_CHECK(bad.rolled_back && !bad.confirmed && bad.slot == 0);
  RC_CHECK(bad.boots == 4);
}
