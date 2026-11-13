#include <rc/test/rc_test.hpp>

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

bool has(const DayResult& r, FindingKind k) {
  for (std::size_t i = 0; i < r.findings.size(); ++i)
    if (r.findings[i].kind == k) return true;
  return false;
}

}  // namespace

RC_TEST("with the layers the vehicle does the whole day, touches no one, and gets home with charge to spare") {
  const DayResult r = run_day(DayConfig(), true);
  rec("legs_on", r.legs_done, 0);
  rec("t_on", r.time, 0);
  rec("contacts_on", r.contacts, 0);
  rec("field_s", r.field_stop_s, 1);
  rec("att", r.attempts, 0);
  rec("reduced_s", r.mode_s[1] + r.mode_s[2], 0);
  rec("stop_s", r.mode_s[3], 1);
  rec("used_wh", r.used_wh, 2);
  rec("start_wh", r.start_wh, 2);
  RC_CHECK(r.completed);
  RC_CHECK(r.legs_done == 4);
  RC_CHECK(r.contacts == 0);
  RC_CHECK(r.field_stop_s > 3.0);
  RC_CHECK(r.attempts >= 1);
  RC_CHECK(r.mode_s[1] + r.mode_s[2] > 50.0);
  RC_CHECK(r.mode_s[3] > 2.0);
  RC_CHECK(r.end_wh > 0.1 * r.start_wh);
  RC_CHECK(r.final_s < 1.5);
  RC_CHECK(!r.stranded);
}

RC_TEST("a release offered in the middle of a job is refused, and the same release at the dock is taken") {
  const DayResult r = run_day(DayConfig(), true);
  RC_CHECK(r.release_refused_midday);
  RC_CHECK(r.release_accepted_at_dock);
}

RC_TEST("the log of the day shows the snag, and what the vehicle did about it") {
  const DayResult r = run_day(DayConfig(), true);
  rec("n_find", static_cast<double>(r.findings.size()), 0);
  RC_CHECK(has(r, kFindStuck));
  RC_CHECK(!has(r, kFindGap));
  RC_CHECK(!has(r, kFindOffPath));
}

RC_TEST("without the layers the same day has a contact, skips two of its four legs, and runs out of charge on the road") {
  const DayResult r = run_day(DayConfig(), false);
  rec("legs_off", r.legs_done, 0);
  rec("contacts_off", r.contacts, 0);
  rec("t_off", r.time, 0);
  rec("stranded_at", r.stranded_s, 0);
  RC_CHECK(r.legs_done < 4);
  RC_CHECK(r.contacts == 1);
  RC_CHECK(r.stranded);
  RC_CHECK(r.time > 1.5 * run_day(DayConfig(), true).time);
}

RC_TEST("a vehicle with too little charge is kept at the dock by the checklist") {
  DayConfig cfg;
  cfg.soc_wh = 0.5;
  const DayResult r = run_day(cfg, true);
  RC_CHECK(r.refused_at_dock);
  RC_CHECK(r.distance == 0.0);
  RC_CHECK(r.blocked_by.size() == 1 && r.blocked_by[0] == "energy");
}

RC_TEST("the day is the same every time, and the report says what happened") {
  const DayResult a = run_day(DayConfig(), true);
  const DayResult b = run_day(DayConfig(), true);
  RC_CHECK_NEAR(a.time, b.time, 1e-12);
  RC_CHECK_NEAR(a.used_wh, b.used_wh, 1e-12);
  const std::string text = day_report(a);
  RC_CHECK(text.find("legs done 4 of 4") != std::string::npos);
  RC_CHECK(text.find("release: refused in the middle of the job, taken at the dock") != std::string::npos);
}

RC_TEST("the speed limits are joined by taking the least, and the field has the last word") {
  RC_CHECK_NEAR(combine_speed(1.5, 0.8, kClear, 0.3), 0.8, 1e-12);
  RC_CHECK_NEAR(combine_speed(0.5, 0.8, kClear, 0.3), 0.5, 1e-12);
  RC_CHECK_NEAR(combine_speed(1.5, 2.0, kWarn, 0.3), 0.3, 1e-12);
  RC_CHECK_NEAR(combine_speed(1.5, 2.0, kStop, 0.3), 0.0, 1e-12);
}
