#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iostream>

#include <rc/field/sensors.hpp>
#include <rc/hal/interfaces.hpp>

#include "solution.hpp"
#include <iomanip>
#include <iostream>

static void rec(const char* key, double value, int digits = 3) {
  std::cout << "@" << key << " " << std::fixed << std::setprecision(digits) << value << std::endl;
  std::cout.unsetf(std::ios::floatfield);
}


namespace {

// A ten second drive of the fake base, with a GNSS receiver watching, recorded at 50 Hz.
RunLog drive(unsigned int seed) {
  RunLog log;
  log.scenario = "scenario loop\nduration 10\nstart 0 0 0\n";
  rc::hal::FakeDrive base;
  rc::field::Gnss gnss(rc::field::GnssSpec(), seed);
  for (int i = 1; i <= 500; ++i) {
    const double t = 0.02 * i;
    rc::hal::DriveCommand c;
    c.v = 0.5;
    c.w = (i / 125) % 2 == 0 ? 0.3 : -0.3;
    base.command(c);
    base.step(0.02);
    const rc::hal::DriveState s = base.state();
    log.add(t, "pose", {s.x, s.y, s.theta});
    const rc::field::GnssFix f = gnss.measure(s.x, s.y, rc::field::Fix::fixed);
    log.add(t, "gnss", {f.x, f.y});
  }
  return log;
}

}  // namespace

RC_TEST("a recorded run reads back exactly, scenario and all, and with too few digits it does not") {
  const RunLog log = drive(1);
  const std::string text = log.text();
  rec("n_entries", static_cast<double>(log.entries.size()), 0);
  rec("kb", static_cast<double>(text.size()) / 1000.0, 1);
  const LogParse p = parse_log(text);
  RC_CHECK(p.ok);
  RC_CHECK(!p.truncated);
  RC_CHECK(p.log.scenario == log.scenario);
  RC_CHECK(p.log.entries.size() == log.entries.size());
  bool exact = true;
  for (std::size_t i = 0; i < log.entries.size(); ++i)
    for (std::size_t k = 0; k < log.entries[i].values.size(); ++k)
      if (p.log.entries[i].values[k] != log.entries[i].values[k]) exact = false;
  RC_CHECK(exact);

  const LogParse coarse = parse_log(log.text(6));
  RC_CHECK(coarse.ok);
  double worst = 0.0;
  for (std::size_t i = 0; i < log.entries.size(); ++i)
    for (std::size_t k = 0; k < log.entries[i].values.size(); ++k) worst = std::fmax(worst, std::fabs(coarse.log.entries[i].values[k] - log.entries[i].values[k]));
  rec("err6", worst, 7);
  RC_CHECK(worst > 0.0);
  RC_CHECK(worst < 1e-4);
}

RC_TEST("the same seed gives the same run, and another seed differs from the first sample") {
  const RunLog a = drive(1);
  const RunLog b = drive(1);
  const RunLog c = drive(2);
  RC_CHECK(first_difference(a, b, 0.0) < 0.0);
  const double diff = first_difference(a, c, 1e-9);
  rec("diff_t", diff, 2);
  RC_CHECK_NEAR(diff, 0.02, 1e-9);
  RunLog shorter = a;
  shorter.entries.resize(a.entries.size() - 4);
  RC_CHECK_NEAR(first_difference(a, shorter, 0.0), 0.02 * 499, 1e-9);
}

RC_TEST("entries are found by time and by channel") {
  const RunLog log = drive(1);
  const std::vector<LogEntry> one_second = log.between(2.0, 3.0, "pose");
  RC_CHECK(one_second.size() == 50);
  RC_CHECK(log.between(2.0, 3.0, "").size() == 100);
  LogEntry e;
  RC_CHECK(log.at("pose", 3.005, e));
  RC_CHECK_NEAR(e.t, 3.0, 1e-9);
  RC_CHECK(!log.at("pose", 0.001, e));
  RC_CHECK(!log.at("lidar", 5.0, e));
}

RC_TEST("a log cut by a power failure still reads, and a bad line stops the read with its number") {
  const RunLog log = drive(1);
  const std::string text = log.text();
  const std::string cut = text.substr(0, text.size() - 7);
  const LogParse p = parse_log(cut);
  rec("n_cut", static_cast<double>(p.log.entries.size()), 0);
  RC_CHECK(p.ok);
  RC_CHECK(p.truncated);
  RC_CHECK(p.log.entries.size() == log.entries.size() - 1);

  RC_CHECK(!parse_log("hello\n").ok);
  RC_CHECK(parse_log("hello\n").error_line == 1);
  const LogParse bad = parse_log("runlog 1\n@ 1 pose 1 2\n@ 2 pose 1 x\n");
  RC_CHECK(!bad.ok);
  RC_CHECK(bad.error_line == 3);
  const LogParse back = parse_log("runlog 1\n@ 2 pose 1\n@ 1 pose 1\n");
  RC_CHECK(!back.ok);
  RC_CHECK(back.error_line == 3);
}
