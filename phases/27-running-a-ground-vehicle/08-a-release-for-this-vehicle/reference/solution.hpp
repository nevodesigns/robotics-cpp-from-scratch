#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <rc/deploy/release.hpp>
#include <rc/deploy/trial.hpp>
#include <string>
#include <vector>

struct OnBoard {
  std::string id;
  std::string vehicle_class;
  std::string software;
  int config_schema = 1;
  int map_version = 1;
  bool parked = true;
  bool mission_active = false;
  double soc = 1.0;  // fraction of full
};

struct Bundle {
  std::string vehicle_class;
  std::string software;
  int config_schema = 1;
  int map_version = 1;
  std::string calibration_for;  // the vehicle id the calibration was made for, or empty if none is carried
};

enum Refusal { kWrongClass = 0, kWrongCalibration = 1, kOlderSchema = 2, kOlderMap = 3, kNotParked = 4, kMissionActive = 5, kLowCharge = 6, kVersion = 7 };

struct UpdatePolicy {
  double min_soc = 0.5;
  std::string minimum_version = "1.0.0";
  bool rollback_authorised = false;
};

// Every reason the bundle is refused for this vehicle: an empty list means it may go ahead.
inline std::vector<Refusal> check_update(const OnBoard& v, const Bundle& b, const UpdatePolicy& p) {
  std::vector<Refusal> why;
  if (b.vehicle_class != v.vehicle_class) why.push_back(kWrongClass);
  if (!b.calibration_for.empty() && b.calibration_for != v.id) why.push_back(kWrongCalibration);
  if (b.config_schema < v.config_schema) why.push_back(kOlderSchema);
  if (b.map_version < v.map_version) why.push_back(kOlderMap);
  if (!v.parked) why.push_back(kNotParked);
  if (v.mission_active) why.push_back(kMissionActive);
  if (!(v.soc >= p.min_soc)) why.push_back(kLowCharge);
  if (rc::deploy::decide(v.software, b.software, p.minimum_version, p.rollback_authorised) != rc::deploy::VersionDecision::install) why.push_back(kVersion);
  return why;
}

struct SelfTestInput {
  double forward_moved_m = 0.0;   // how far a command to go forward moved the vehicle
  double odom_over_ground = 1.0;  // the wheels' distance over the ground's, over a drive of 1 m
  double estop_distance_m = 0.0;  // the distance an emergency stop took from the test speed
  bool scanner_saw_target = false;
};

struct SelfTestLimits {
  double min_forward_m = 0.05;
  double odom_tolerance = 0.05;  // the ratio may be this far from 1
  double estop_max_m = 1.0;
};

// What the new software got wrong on the vehicle, as names; empty if it passed.
inline std::vector<std::string> post_update_selftest(const SelfTestInput& in, const SelfTestLimits& lim) {
  std::vector<std::string> failed;
  if (!(in.forward_moved_m >= lim.min_forward_m)) failed.push_back("wheel direction");
  if (!(std::fabs(in.odom_over_ground - 1.0) <= lim.odom_tolerance)) failed.push_back("odometry");
  if (!(in.estop_distance_m <= lim.estop_max_m)) failed.push_back("emergency stop");
  if (!in.scanner_saw_target) failed.push_back("scanner");
  return failed;
}

// The boot loader of lesson 20-06 with this release on trial: how many boots until it is settled (confirmed, or rolled back), and which slot it ended on.
struct Settled {
  int boots = 0;
  int slot = 0;
  bool confirmed = false;
  bool rolled_back = false;
};

inline Settled boots_until_settled(int old_slot, int new_slot, int tries, bool selftest_passes) {
  rc::deploy::BootState s;
  s.active = old_slot;
  rc::deploy::start_trial(s, new_slot, tries);
  Settled out;
  for (int guard = 0; guard < 100; ++guard) {
    const int slot = rc::deploy::on_boot(s);
    ++out.boots;
    out.slot = slot;
    if (s.rolled_back) {
      out.rolled_back = true;
      return out;
    }
    if (slot == new_slot && selftest_passes) {
      out.confirmed = rc::deploy::confirm(s);
      return out;
    }
  }
  return out;
}


#endif
