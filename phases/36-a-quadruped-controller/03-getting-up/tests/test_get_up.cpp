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


namespace {

JointMotor leg_motor() {
  JointMotor m;
  m.kt_out = 0.6;        // a hobby grade knee actuator (typical)
  m.resistance = 1.2;
  m.r_th = 2.0;
  m.c_th = 40.0;
  m.rise_limit = 60.0;
  m.tau_peak = 20.0;
  m.tau_cont = 6.0;
  return m;
}

}  // namespace

RC_TEST("the knee's torque is greatest at the bottom of the rise and falls as the legs straighten") {
  LegGeom g;
  double low[3], mid[3], high[3];
  stance_torques(g, 0.12, 12.0, 4, low);
  stance_torques(g, 0.20, 12.0, 4, mid);
  stance_torques(g, 0.28, 12.0, 4, high);
  RC_CHECK(std::fabs(low[2]) > std::fabs(mid[2]) && std::fabs(mid[2]) > std::fabs(high[2]));
  const JointMotor m = leg_motor();
  const RisePlan p = rise_plan(g, 12.0, m, 0.12, 0.28, 2.0, 40);
  RC_CHECK(p.tau_peak > std::fabs(mid[2]) && p.tau_peak <= std::fabs(low[2]) + 1e-9);
}

RC_TEST("a 12 kg robot can rise, a slow rise uses more of the knee's thermal life than a fast one, and the heaviest bodies are limited by torque and by heat") {
  LegGeom g;
  const JointMotor m = leg_motor();
  const double h0 = 0.12, h1 = 0.28;
  const RisePlan fast = rise_plan(g, 12.0, m, h0, h1, 1.0, 40), slow = rise_plan(g, 12.0, m, h0, h1, 6.0, 40);
  rec("mass", 12.0, 0);
  rec("h0", h0, 2);
  rec("h1", h1, 2);
  rec("tau_peak", fast.tau_peak, 1);
  rec("peak_limit", m.tau_peak, 0);
  rec("used_fast", fast.life_used, 3);
  rec("t_fast", 1.0, 0);
  rec("used_slow", slow.life_used, 3);
  rec("t_slow", 6.0, 0);
  RC_CHECK(fast.torque_ok && fast.heat_ok);
  RC_CHECK_NEAR(slow.life_used / fast.life_used, 6.0, 1e-6);
  const double m_peak = max_rising_mass(g, m, h0, h1, 1.0, false), m_heat = max_rising_mass(g, m, h0, h1, 1.0, true);
  rec("m_peak", m_peak, 1);
  rec("m_heat", m_heat, 1);
  const double m_heat_slow = max_rising_mass(g, m, h0, h1, 6.0, true);
  rec("m_heat_slow", m_heat_slow, 1);
  RC_CHECK(m_heat_slow < m_peak);
  RC_CHECK(m_heat <= m_peak + 1e-6);
  RC_CHECK(m_peak > 12.0);
  const double heavy = 1.6 * m_peak;
  rec("mass_heavy", heavy, 0);
  const RisePlan hp = rise_plan(g, heavy, m, h0, h1, 1.0, 40);
  rec("heavy_ok", hp.torque_ok ? 1 : 0, 0);
  RC_CHECK(!hp.torque_ok);
}

RC_TEST("a shallower crouch is easier, and the heaviest body that can rise falls as the crouch deepens") {
  LegGeom g;
  const JointMotor m = leg_motor();
  const double deep = max_rising_mass(g, m, 0.08, 0.28, 1.0, false), shallow = max_rising_mass(g, m, 0.20, 0.28, 1.0, false);
  RC_CHECK(shallow > deep);
  RC_CHECK(deep > 0.0);
}
