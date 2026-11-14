#include <rc/test/rc_test.hpp>
#include <sstream>
#include <cstring>

#include <cmath>

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
    {"inertia", 0.05},
    {"kp", 10.0},
    {"kd", 1.5},
    {"t_nom", 0.531},
    {"os_nom", 0.0},
    {"kp_s", 40.0},
    {"kd_s", 3.0},
    {"os_sim_s", 0.24999999999955058},
    {"t_sim_s", 0.243},
    {"delay_ms", 20.0},
    {"lag_ms", 10.0},
    {"os_real", 23.504761879992397},
    {"max_delay_ms", 19.499969482421875},
    {"max_delay_gentle_ms", 43.499946594238295},
    {"kp_g", 10.0},
    {"kd_g", 1.5},
    {"n", 100.0},
    {"rate_stiff", 46.0},
    {"rate_gentle", 97.0},
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


RC_TEST("in the simulator a critically damped joint settles with almost no overshoot, and a stiffer one is quicker") {
  JointSim j;
  const StepResult r = simulate_step(j, 0.5, 4.0, 0.001);
  rec("inertia", j.inertia, 2);
  rec("kp", j.kp, 0);
  rec("kd", j.kd, 1);
  rec("t_nom", r.t_settle, 2);
  rec("os_nom", r.overshoot * 100.0, 1);
  RC_CHECK(r.settled);
  RC_CHECK(r.overshoot < 0.05);
  JointSim stiff = j;
  stiff.kp = 40.0;
  stiff.kd = 3.0;
  const StepResult s = simulate_step(stiff, 0.5, 4.0, 0.001);
  RC_CHECK(s.settled && s.t_settle < r.t_settle);
}

RC_TEST("a delay and a lag in the actuator make a joint that settled in the simulator ring on the real one, and a joint with too much delay does not settle at all") {
  JointSim sim, real;
  sim.kp = 40.0;
  sim.kd = 3.0;
  real = sim;
  real.delay = 0.02;
  real.lag = 0.01;
  const StepResult a = simulate_step(sim, 0.5, 6.0, 0.001), b = simulate_step(real, 0.5, 6.0, 0.001);
  rec("kp_s", sim.kp, 0);
  rec("kd_s", sim.kd, 1);
  rec("os_sim_s", a.overshoot * 100.0, 1);
  rec("t_sim_s", a.t_settle, 2);
  rec("delay_ms", real.delay * 1000.0, 0);
  rec("lag_ms", real.lag * 1000.0, 0);
  rec("os_real", b.overshoot * 100.0, 1);
  RC_CHECK(a.settled);
  RC_CHECK(!b.settled);                    // the joint that settled in the simulator rings on the real one
  RC_CHECK(b.overshoot > 10.0 * (a.overshoot + 0.01));
  JointSim hopeless = sim;
  hopeless.delay = 0.3;
  RC_CHECK(!simulate_step(hopeless, 0.5, 6.0, 0.001).settled);
  // a limit on the torque and the friction do not stop it settling, they only slow it
  JointSim weak = sim;
  weak.tau_max = 3.0;
  RC_CHECK(simulate_step(weak, 0.5, 6.0, 0.001).settled);
}

RC_TEST("the delay a joint can stand is found, a gentler set of gains stands more, and over a spread of robots the stiff gains fail where the gentle ones settle") {
  JointSim stiff, gentle;
  stiff.kp = 60.0;
  stiff.kd = 3.0;
  gentle.kp = 10.0;
  gentle.kd = 1.5;
  const double ds = max_stable_delay(stiff, 0.5, 6.0, 0.001, 0.2), dg = max_stable_delay(gentle, 0.5, 6.0, 0.001, 0.2);
  rec("max_delay_ms", ds * 1000.0, 0);
  rec("max_delay_gentle_ms", dg * 1000.0, 0);
  rec("kp_g", gentle.kp, 0);
  rec("kd_g", gentle.kd, 1);
  RC_CHECK(dg > ds);
  RC_CHECK(ds > 0.0 && ds < 0.2);
  const int n = 100;
  const double rs = success_rate(stiff, 0.3, 0.03, 0.015, n, 7, 0.5, 6.0, 0.001), rg = success_rate(gentle, 0.3, 0.03, 0.015, n, 7, 0.5, 6.0, 0.001);
  rec("n", n, 0);
  rec("rate_stiff", rs * 100.0, 0);
  rec("rate_gentle", rg * 100.0, 0);
  RC_CHECK(rg > rs);
  RC_CHECK(rg > 0.9);
}
