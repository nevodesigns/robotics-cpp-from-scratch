#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <cstddef>
#include <sstream>
#include <string>
#include <vector>

enum Severity { kBlock = 0, kWarning = 1 };

struct VehicleState {
  double soc_wh = 0.0;            // charge left
  double mission_wh = 0.0;        // what the mission needs, there and back (lesson 27-04)
  double reserve_wh = 0.0;        // what must be left at the end
  double estop_tested_h = 0.0;    // hours since the emergency stop was last tried
  double position_sigma_m = 0.0;  // how sure the position is
  double scan_age_s = 0.0;        // age of the newest scan
  double link_rtt_s = 0.0;        // round trip to the station
  bool geofence_loaded = false;
  double log_free_mb = 0.0;
  double clock_error_s = 0.0;
  int release = 0;                // the release on the vehicle
  int release_expected = 0;       // the release the mission was built for
  double motor_temp_c = 0.0;
};

struct CheckLimits {
  double estop_max_h = 24.0;
  double sigma_max_m = 0.5;
  double scan_max_s = 0.2;
  double rtt_max_s = 0.5;
  double log_min_mb = 500.0;
  double clock_max_s = 0.1;
  double temp_max_c = 60.0;
};

struct CheckResult {
  std::string name;
  Severity severity = kBlock;
  bool ok = false;
  std::string detail;
};

inline CheckResult make_check(const char* name, Severity sev, bool ok, const std::string& detail) {
  CheckResult r;
  r.name = name;
  r.severity = sev;
  r.ok = ok;
  r.detail = detail;
  return r;
}

inline std::string num(double v) {
  std::ostringstream o;
  o << v;
  return o.str();
}

// The ten checks, in a fixed order. A number that is not a number fails.
inline std::vector<CheckResult> run_checks(const VehicleState& s, const CheckLimits& lim) {
  std::vector<CheckResult> out;
  out.push_back(make_check("energy", kBlock, s.soc_wh >= s.mission_wh + s.reserve_wh, num(s.soc_wh) + " Wh left, " + num(s.mission_wh + s.reserve_wh) + " Wh needed"));
  out.push_back(make_check("emergency stop", kBlock, s.estop_tested_h <= lim.estop_max_h, "tried " + num(s.estop_tested_h) + " h ago"));
  out.push_back(make_check("position", kBlock, s.position_sigma_m <= lim.sigma_max_m, "sigma " + num(s.position_sigma_m) + " m"));
  out.push_back(make_check("scan", kBlock, s.scan_age_s <= lim.scan_max_s, "age " + num(s.scan_age_s) + " s"));
  out.push_back(make_check("geofence", kBlock, s.geofence_loaded, s.geofence_loaded ? "loaded" : "not loaded"));
  out.push_back(make_check("release", kBlock, s.release == s.release_expected, "on the vehicle " + num(s.release) + ", expected " + num(s.release_expected)));
  out.push_back(make_check("link", kWarning, s.link_rtt_s <= lim.rtt_max_s, "round trip " + num(s.link_rtt_s) + " s"));
  out.push_back(make_check("log space", kWarning, s.log_free_mb >= lim.log_min_mb, num(s.log_free_mb) + " MB free"));
  out.push_back(make_check("clock", kWarning, std::fabs(s.clock_error_s) <= lim.clock_max_s && s.clock_error_s == s.clock_error_s, "error " + num(s.clock_error_s) + " s"));
  out.push_back(make_check("motor temperature", kWarning, s.motor_temp_c <= lim.temp_max_c, num(s.motor_temp_c) + " C"));
  return out;
}

// True when no blocking check failed.
inline bool can_go(const std::vector<CheckResult>& r) {
  for (std::size_t i = 0; i < r.size(); ++i)
    if (!r[i].ok && r[i].severity == kBlock) return false;
  return true;
}

struct Override {
  std::string check;
  std::string reason;
};

struct Decision {
  bool go = false;
  std::vector<std::string> accepted;  // warnings overridden, with the reason
  std::vector<std::string> refused;   // overrides that were not allowed, and why
};

// A person may override a warning, with a reason; a block cannot be overridden.
inline Decision decide(const std::vector<CheckResult>& results, const std::vector<Override>& overrides) {
  Decision d;
  d.go = true;
  for (std::size_t i = 0; i < results.size(); ++i) {
    if (results[i].ok) continue;
    bool overridden = false;
    for (std::size_t k = 0; k < overrides.size(); ++k) {
      if (overrides[k].check != results[i].name) continue;
      if (results[i].severity == kBlock) {
        d.refused.push_back(results[i].name + ": a block cannot be overridden");
      } else if (overrides[k].reason.empty()) {
        d.refused.push_back(results[i].name + ": an override needs a reason");
      } else {
        d.accepted.push_back(results[i].name + ": " + overrides[k].reason);
        overridden = true;
      }
    }
    if (!overridden) d.go = false;
  }
  return d;
}

inline std::string report(const std::vector<CheckResult>& r) {
  std::ostringstream o;
  for (std::size_t i = 0; i < r.size(); ++i)
    o << (r[i].ok ? "pass  " : (r[i].severity == kBlock ? "BLOCK " : "warn  ")) << r[i].name << ": " << r[i].detail << "\n";
  return o.str();
}


#endif
