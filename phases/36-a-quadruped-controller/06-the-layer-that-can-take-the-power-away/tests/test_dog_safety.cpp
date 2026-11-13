#include <rc/test/rc_test.hpp>

#include <cmath>

#include "solution.hpp"
#include <iomanip>
#include <iostream>

static void rec(const char* key, double value, int digits = 3) {
  std::cout << "@" << key << " " << std::fixed << std::setprecision(digits) << value << std::endl;
  std::cout.unsetf(std::ios::floatfield);
}
[[maybe_unused]] static void recs(const char* key, const std::string& value) { std::cout << "@" << key << " " << value << std::endl; }


RC_TEST("the torque is cut to the peak the motor may give, whichever way it pushes") {
  RC_CHECK_NEAR(limit_torque(40.0, 20.0), 20.0, 1e-12);
  RC_CHECK_NEAR(limit_torque(-40.0, 20.0), -20.0, 1e-12);
  RC_CHECK_NEAR(limit_torque(5.0, 20.0), 5.0, 1e-12);
  rec("cmd_in", 40.0, 0);
  rec("cmd_out", limit_torque(40.0, 20.0), 0);
}

RC_TEST("hot knees are derated down to nothing, then switched off") {
  SafetyLimits l;
  SafetyIn in;
  in.heat = 0.7;
  SafetyOut o = safety_decide(in, l);
  RC_CHECK(o.mode == kSafeNormal && o.scale == 1.0);
  in.heat = 0.9;
  o = safety_decide(in, l);
  rec("heat_a", 0.9, 1);
  rec("scale_a", o.scale, 2);
  RC_CHECK(o.mode == kSafeDerated);
  RC_CHECK_NEAR(o.scale, 0.5, 1e-12);
  in.heat = 0.95;
  o = safety_decide(in, l);
  rec("heat_b", 0.95, 2);
  rec("scale_b", o.scale, 2);
  RC_CHECK_NEAR(o.scale, 0.25, 1e-12);
  in.heat = 1.0;
  in.standing = false;
  o = safety_decide(in, l);
  rec("heat_c", 1.0, 1);
  RC_CHECK(o.mode == kSafeOff && o.scale == 0.0);
  RC_CHECK_NEAR(apply_scale(10.0, safety_decide(SafetyIn(), l)), 10.0, 1e-12);
}

RC_TEST("a silent link asks for soft and then off, a stop and a fall ask for off, and off while standing becomes soft") {
  SafetyLimits l;
  SafetyIn in;
  in.link_age = 0.3;
  const int m_short = safety_decide(in, l).mode;
  in.link_age = 2.0;
  const int m_long = safety_decide(in, l).mode;
  rec("age_short", 0.3, 1);
  rec("m_short", m_short, 0);
  rec("age_long", 2.0, 1);
  rec("m_long", m_long, 0);
  RC_CHECK(m_short == kSafeSoft && m_long == kSafeOff);
  SafetyIn stop;
  stop.estop = true;
  stop.standing = true;
  const int up = safety_decide(stop, l).mode;
  stop.standing = false;
  const int down = safety_decide(stop, l).mode;
  rec("m_estop_up", up, 0);
  rec("m_estop_down", down, 0);
  RC_CHECK(up == kSafeSoft && down == kSafeOff);
  SafetyIn fall;
  fall.fall = kFallSoft;
  fall.standing = true;
  const int m_fall = safety_decide(fall, l).mode;
  rec("m_fall", m_fall, 0);
  RC_CHECK(m_fall == kSafeSoft);
  fall.fall = kFallen;
  fall.standing = false;
  RC_CHECK(safety_decide(fall, l).mode == kSafeOff);
  SafetyIn both;
  both.heat = 0.9;
  both.link_age = 0.3;
  both.standing = true;
  const SafetyOut bo = safety_decide(both, l);
  rec("m_both", bo.mode, 0);
  RC_CHECK(bo.mode == kSafeSoft);              // the higher of soft and derated
  RC_CHECK_NEAR(bo.scale, 0.5, 1e-12);          // and the derating still applies
}
