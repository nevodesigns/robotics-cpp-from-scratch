#include <rc/test/rc_test.hpp>
#include <sstream>
#include <cstring>

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
    {"n_checks", 10.0},
    {"n_pass", 10.0},
    {"scale_ok", 1200.0000058175067},
    {"n_faults", 10.0},
    {"scale_read", 1377.7777844571372},
    {"deadband_read", 0.25},
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


namespace {

std::vector<BringUpResult> run(const BoardFaults& f) {
  FakeBoard board(f);
  return run_bringup(board, BringUpExpect(), board);
}

bool has(const std::vector<std::string>& v, const std::string& id) {
  for (std::size_t i = 0; i < v.size(); ++i)
    if (v[i] == id) return true;
  return false;
}

double measured(const std::vector<BringUpResult>& r, const std::string& id) {
  for (std::size_t i = 0; i < r.size(); ++i)
    if (r[i].id == id) return r[i].measured;
  return -1.0;
}

std::string advice(const std::vector<BringUpResult>& r, const std::string& id) {
  for (std::size_t i = 0; i < r.size(); ++i)
    if (r[i].id == id) return r[i].advice;
  return "";
}

}  // namespace

RC_TEST("a healthy board passes the ten checks, in the order that costs least first") {
  const std::vector<BringUpResult> r = run(BoardFaults());
  for (std::size_t i = 0; i < r.size(); ++i) std::cout << "  " << r[i].id << ": " << (r[i].ok ? "ok" : "FAILED") << " (measured " << r[i].measured << ")\n";
  rec("n_checks", static_cast<double>(r.size()), 0);
  int pass = 0;
  for (std::size_t i = 0; i < r.size(); ++i) pass += r[i].ok ? 1 : 0;
  rec("n_pass", pass, 0);
  rec("scale_ok", measured(r, "scale"), 0);
  RC_CHECK(r.size() == 10);
  RC_CHECK(pass == 10);
  const char* order[] = {"link", "estop", "battery", "imu", "gyro", "direction", "sides", "scale", "deadband", "watchdog"};
  for (std::size_t i = 0; i < r.size() && i < 10; ++i) RC_CHECK(r[i].id == order[i]);
  RC_CHECK_NEAR(measured(r, "scale"), 1200.0, 36.0);
  RC_CHECK_NEAR(measured(r, "deadband"), 0.08, 0.011);
}

RC_TEST("each fault fails the check of its own, and no check that is not its consequence") {
  struct Case {
    const char* name;
    BoardFaults f;
    std::vector<std::string> expect;
  };
  std::vector<Case> cases;
  BoardFaults f;
  f = BoardFaults(); f.swap_wheels = true;      cases.push_back({"swapped wheels", f, {"sides"}});
  f = BoardFaults(); f.invert_left = true;      cases.push_back({"reversed left motor", f, {"direction", "sides", "scale"}});
  f = BoardFaults(); f.tick_scale = 1.15;       cases.push_back({"ticks 15 percent off", f, {"scale"}});
  f = BoardFaults(); f.watchdog = 0.0;          cases.push_back({"no watchdog", f, {"watchdog"}});
  f = BoardFaults(); f.stuck_right = true;      cases.push_back({"dead right encoder", f, {"direction", "scale"}});
  f = BoardFaults(); f.battery = 9.0;           cases.push_back({"flat battery", f, {"battery"}});
  f = BoardFaults(); f.az_sign = -1.0;          cases.push_back({"upside down IMU", f, {"imu"}});
  f = BoardFaults(); f.gyro_bias = 0.1;         cases.push_back({"gyro bias", f, {"gyro"}});
  f = BoardFaults(); f.deadband = 0.25;         cases.push_back({"large dead band", f, {"deadband"}});
  f = BoardFaults(); f.estop = true;            cases.push_back({"stop pressed", f, {"estop"}});
  rec("n_faults", static_cast<double>(cases.size()), 0);
  for (std::size_t i = 0; i < cases.size(); ++i) {
    const std::vector<std::string> failed = failed_ids(run(cases[i].f));
    std::cout << "  " << cases[i].name << " failed:";
    for (std::size_t k = 0; k < failed.size(); ++k) std::cout << " " << failed[k];
    std::cout << "\n";
    RC_CHECK(failed.size() == cases[i].expect.size());
    for (std::size_t k = 0; k < cases[i].expect.size(); ++k) RC_CHECK(has(failed, cases[i].expect[k]));
  }
}

RC_TEST("the numbers measured are the ones the fault set, and the advice says what to do about it") {
  BoardFaults off;
  off.tick_scale = 1.15;
  const std::vector<BringUpResult> a = run(off);
  rec("scale_read", measured(a, "scale"), 0);
  RC_CHECK_NEAR(measured(a, "scale"), 1380.0, 40.0);
  RC_CHECK(advice(a, "scale").find("wheel diameter") != std::string::npos);

  BoardFaults dead;
  dead.deadband = 0.25;
  const std::vector<BringUpResult> b = run(dead);
  rec("deadband_read", measured(b, "deadband"), 2);
  RC_CHECK_NEAR(measured(b, "deadband"), 0.25, 0.011);

  BoardFaults rev;
  rev.invert_left = true;
  RC_CHECK(advice(run(rev), "direction").find("left encoder counts down") != std::string::npos);
  BoardFaults stuck;
  stuck.stuck_right = true;
  RC_CHECK(advice(run(stuck), "direction").find("right encoder does not count") != std::string::npos);
  RC_CHECK(advice(run(BoardFaults()), "direction").empty());
}
