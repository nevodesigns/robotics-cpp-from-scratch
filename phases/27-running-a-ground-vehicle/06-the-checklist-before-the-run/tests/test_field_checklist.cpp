#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iostream>
#include <limits>

#include "solution.hpp"
#include <iomanip>
#include <iostream>

static void rec(const char* key, double value, int digits = 3) {
  std::cout << "@" << key << " " << std::fixed << std::setprecision(digits) << value << std::endl;
  std::cout.unsetf(std::ios::floatfield);
}
[[maybe_unused]] static void recs(const char* key, const std::string& value) { std::cout << "@" << key << " " << value << std::endl; }


namespace {

VehicleState good() {
  VehicleState s;
  s.soc_wh = 5.0;
  s.mission_wh = 4.0;
  s.reserve_wh = 0.5;
  s.estop_tested_h = 3.0;
  s.position_sigma_m = 0.1;
  s.scan_age_s = 0.05;
  s.link_rtt_s = 0.1;
  s.geofence_loaded = true;
  s.log_free_mb = 4000.0;
  s.clock_error_s = 0.01;
  s.release = 7;
  s.release_expected = 7;
  s.motor_temp_c = 35.0;
  return s;
}

int failed(const std::vector<CheckResult>& r) {
  int n = 0;
  for (std::size_t i = 0; i < r.size(); ++i)
    if (!r[i].ok) ++n;
  return n;
}

}  // namespace

RC_TEST("a good vehicle passes all ten checks and may go") {
  const std::vector<CheckResult> r = run_checks(good(), CheckLimits());
  rec("n_checks", static_cast<double>(r.size()), 0);
  RC_CHECK(r.size() == 10);
  RC_CHECK(failed(r) == 0);
  RC_CHECK(can_go(r));
}

RC_TEST("each check can fail alone, and the block or the warning is the one it is meant to be") {
  const CheckLimits lim;
  std::vector<VehicleState> bad(10, good());
  bad[0].soc_wh = 4.0;
  bad[1].estop_tested_h = 30.0;
  bad[2].position_sigma_m = 2.0;
  bad[3].scan_age_s = 1.0;
  bad[4].geofence_loaded = false;
  bad[5].release = 8;
  bad[6].link_rtt_s = 2.0;
  bad[7].log_free_mb = 10.0;
  bad[8].clock_error_s = 5.0;
  bad[9].motor_temp_c = 90.0;
  int blocks = 0, warns = 0;
  for (int k = 0; k < 10; ++k) {
    const std::vector<CheckResult> r = run_checks(bad[static_cast<std::size_t>(k)], lim);
    RC_CHECK(failed(r) == 1);
    RC_CHECK(!r[static_cast<std::size_t>(k)].ok);
    if (can_go(r)) ++warns; else ++blocks;
  }
  rec("n_block", blocks, 0);
  rec("n_warn", warns, 0);
  RC_CHECK(blocks == 6);
  RC_CHECK(warns == 4);
}

RC_TEST("a number that is not a number fails its check") {
  VehicleState s = good();
  const double nan = std::numeric_limits<double>::quiet_NaN();
  s.soc_wh = s.estop_tested_h = s.position_sigma_m = s.scan_age_s = s.link_rtt_s = nan;
  s.log_free_mb = s.clock_error_s = s.motor_temp_c = nan;
  s.mission_wh = nan;
  const std::vector<CheckResult> r = run_checks(s, CheckLimits());
  rec("nan_fails", failed(r), 0);
  RC_CHECK(failed(r) == 8);
  RC_CHECK(!can_go(r));
}

RC_TEST("the energy check is exact at the edge") {
  VehicleState s = good();
  s.soc_wh = 4.5;
  s.mission_wh = 4.0;
  s.reserve_wh = 0.5;
  RC_CHECK(run_checks(s, CheckLimits())[0].ok);
  s.soc_wh = 4.49;
  RC_CHECK(!run_checks(s, CheckLimits())[0].ok);
}

RC_TEST("a warning can be overridden with a reason, a block cannot, and a reason is needed") {
  const CheckLimits lim;
  VehicleState s = good();
  s.link_rtt_s = 2.0;
  std::vector<Override> o(1);
  o[0].check = "link";
  o[0].reason = "line of sight, no teleoperation today";
  const Decision a = decide(run_checks(s, lim), o);
  RC_CHECK(a.go);
  RC_CHECK(a.accepted.size() == 1);
  o[0].reason = "";
  const Decision b = decide(run_checks(s, lim), o);
  RC_CHECK(!b.go);
  RC_CHECK(b.refused.size() == 1);
  s.geofence_loaded = false;
  o[0].check = "geofence";
  o[0].reason = "it is fine";
  const Decision c = decide(run_checks(s, lim), o);
  RC_CHECK(!c.go);
  RC_CHECK(c.refused.size() == 1);
}

RC_TEST("the report names every check and shows the numbers") {
  VehicleState s = good();
  s.soc_wh = 3.0;
  const std::string text = report(run_checks(s, CheckLimits()));
  RC_CHECK(text.find("BLOCK energy: 3 Wh left, 4.5 Wh needed") != std::string::npos);
  RC_CHECK(text.find("pass  geofence: loaded") != std::string::npos);
}
