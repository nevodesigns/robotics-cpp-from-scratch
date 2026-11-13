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

const double kPi = 3.14159265358979323846;

double leader_q(double t) { return 0.5 * std::sin(kPi * t); }

// rms error between the follower and the leader over 4 s after the first, with the leader read at rate_hz and the messages delayed.
double track_rms(double rate_hz, double delay, double tau, double vmax) {
  const double dt = 0.001;
  double q = 0.0, sum = 0.0;
  int n = 0;
  for (int i = 0; i < 5000; ++i) {
    const double t = i * dt;
    // the newest message that has arrived: sent at k / rate, arrives delay later
    const double k = std::floor((t - delay) * rate_hz);
    const double target = k >= 0.0 ? leader_q(k / rate_hz) : 0.0;
    q = follower_step(q, target, tau, vmax, dt);
    if (t >= 1.0) {
      sum += (q - leader_q(t)) * (q - leader_q(t));
      ++n;
    }
  }
  return std::sqrt(sum / n);
}

}  // namespace

RC_TEST("the map takes the leader's zero away, scales, adds the follower's zero and clamps") {
  JointMap m;
  m.leader_zero = 0.5;
  m.follower_zero = -0.2;
  m.scale = 2.0;
  m.lo = -1.0;
  m.hi = 1.0;
  RC_CHECK_NEAR(map_joint(m, 0.5), -0.2, 1e-12);
  RC_CHECK_NEAR(map_joint(m, 0.7), 0.2, 1e-12);
  RC_CHECK_NEAR(map_joint(m, 5.0), 1.0, 1e-12);
  RC_CHECK_NEAR(map_joint(m, -5.0), -1.0, 1e-12);
  RC_CHECK(link_alive(1.0, 0.9, 0.2));
  RC_CHECK(!link_alive(1.0, 0.7, 0.2));
}

RC_TEST("a follower step goes toward the target as a lag and never faster than its limit") {
  double q = 0.0;
  const double tau = 0.05, vmax = 2.0, dt = 0.001;
  q = follower_step(q, 10.0, tau, vmax, dt);
  RC_CHECK_NEAR(q, vmax * dt, 1e-12);
  double r = 0.0;
  for (int i = 0; i < 250; ++i) r = follower_step(r, 0.1, tau, vmax, dt);
  RC_CHECK_NEAR(r, 0.1, 0.1 * 0.01);
  RC_CHECK(follower_step(0.3, 0.1, tau, vmax, dt) < 0.3);
}

RC_TEST("slower messages and delay both add to the tracking error") {
  const double tau = 0.05, vmax = 4.0;
  const double e100 = track_rms(100.0, 0.0, tau, vmax);
  const double e20 = track_rms(20.0, 0.0, tau, vmax);
  const double ed = track_rms(100.0, 0.1, tau, vmax);
  rec("tau_ms", tau * 1000.0, 0);
  rec("r_hi", 100, 0);
  rec("r_lo", 20, 0);
  rec("delay_ms", 100, 0);
  rec("e_100", e100, 3);
  rec("e_20", e20, 3);
  rec("e_delay", ed, 3);
  RC_CHECK(e20 > e100);
  RC_CHECK(ed > e100);
  RC_CHECK(e100 < 0.1);
}

RC_TEST("a jump in the leader is followed at the speed limit, and no faster") {
  const double dt = 0.001, tau = 0.05, vmax = 2.0;
  double q = 0.0, worst = 0.0, t_reach = -1.0;
  for (int i = 0; i < 3000; ++i) {
    const double next = follower_step(q, 1.0, tau, vmax, dt);
    const double v = (next - q) / dt;
    if (v > worst) worst = v;
    q = next;
    if (t_reach < 0.0 && q >= 0.95) t_reach = i * dt;
  }
  rec("jump", 1.0, 1);
  rec("vmax", vmax, 1);
  rec("t_jump", t_reach, 2);
  RC_CHECK(worst <= vmax + 1e-9);
  RC_CHECK(t_reach > 0.45 && t_reach < 0.65);
}

RC_TEST("when the link is lost a follower with a watchdog holds, and one without goes on to the last message") {
  const double dt = 0.001, tau = 0.05, vmax = 2.0, timeout = 0.2;
  const double t_loss = 0.8;  // the last message is sent here; the leader jumped by 1 rad at 0.7 s
  double q_wd = 0.0, q_no = 0.0, q_at_timeout = -1.0;
  double last_target = 0.0, last_rx = 0.0;
  for (int i = 0; i < 3000; ++i) {
    const double t = i * dt;
    if (t <= t_loss && i % 10 == 0) {
      last_target = t >= 0.7 ? 1.0 : 0.0;
      last_rx = t;
    }
    q_no = follower_step(q_no, last_target, tau, vmax, dt);
    if (link_alive(t, last_rx, timeout)) {
      q_wd = follower_step(q_wd, last_target, tau, vmax, dt);
    } else if (q_at_timeout < 0.0) {
      q_at_timeout = q_wd;
    }
  }
  rec("stop_wd", std::fabs(q_wd - q_at_timeout), 3);
  rec("go_nowd", std::fabs(q_no - q_at_timeout), 3);
  RC_CHECK(std::fabs(q_wd - q_at_timeout) < 1e-12);
  RC_CHECK(std::fabs(q_no - q_at_timeout) > 0.05);
  RC_CHECK(q_wd < 0.95);
}
