#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iomanip>
#include <iostream>

#include "solution.hpp"

namespace {

using rc::safety::CheckState;
using rc::safety::FenceState;
using rc::safety::PowerLimit;
using rc::safety::PowerState;
using rc::safety::TiltState;

// Everything healthy: the loop closed, a fresh heartbeat, inside the fence, level, sensors agree, full power, and a long clear range.
Inputs healthy() {
  Inputs in;
  in.estop_closed = true;
  in.heartbeat_fresh = true;
  in.fence = FenceState::inside;
  in.tilt = TiltState::ok;
  in.sensors = CheckState::ok;
  in.power.state = PowerState::ok;
  in.power.factor = 1.0;
  in.range_cap = 5.0;
  return in;
}

const char* mode_name(Mode m) { return m == Mode::normal ? "normal" : (m == Mode::degraded ? "degraded" : "stopped"); }

// Ten conditions, each present or not, as the bits of a number; the worse of two exclusive ones wins.
Inputs from_bits(unsigned b) {
  Inputs in = healthy();
  if (b & 1u) in.estop_closed = false;
  if (b & 2u) in.heartbeat_fresh = false;
  if (b & 4u) in.fence = FenceState::warning;
  if (b & 8u) in.fence = FenceState::outside;
  if (b & 16u) in.tilt = TiltState::warn;
  if (b & 32u) in.tilt = TiltState::stop;
  if (b & 64u) in.sensors = CheckState::suspect;
  if (b & 128u) in.sensors = CheckState::faulted;
  if (b & 256u) { in.power.state = PowerState::derate; in.power.factor = 0.5; }
  if (b & 512u) { in.power.state = PowerState::stop; in.power.factor = 0.0; }
  return in;
}

Decision once(const Inputs& in) {
  Supervisor s{SupervisorConfig()};
  s.reset(healthy());
  return s.step(in);
}

}  // namespace

RC_TEST("when nothing is known, the answer is stop, and everything is a cause") {
  Supervisor s{SupervisorConfig()};
  RC_CHECK(s.latched());                                       // it starts stopped
  const Decision d = s.step(Inputs());
  std::cout << "  an Inputs that nobody filled in: " << mode_name(d.mode) << ", cap " << d.speed_cap << ", causes: " << causes_text(d.stop_causes) << "\n\n";
  RC_CHECK(d.mode == Mode::stopped);
  RC_CHECK(d.speed_cap == 0.0);
  RC_CHECK(d.stop_causes == (cause_estop | cause_link | cause_fence | cause_tilt | cause_sensors | cause_power));
  RC_CHECK(stop_causes(healthy()) == 0u);
  RC_CHECK(!s.reset(Inputs()));
  RC_CHECK(s.reset(healthy()));
  RC_CHECK(!s.latched());
  RC_CHECK(s.step(healthy()).mode == Mode::normal);
}

RC_TEST("a warning slows the robot, a derated battery slows it by its share, and the range caps it") {
  const SupervisorConfig cfg;
  Inputs in = healthy();
  RC_CHECK(once(in).mode == Mode::normal);
  RC_CHECK(once(in).speed_cap == cfg.top_speed);
  in.fence = FenceState::warning;
  RC_CHECK(once(in).mode == Mode::degraded);
  RC_CHECK(once(in).speed_cap == cfg.slow_speed);
  in = healthy();
  in.power.state = PowerState::derate;
  in.power.factor = 0.4;
  RC_CHECK_NEAR(once(in).speed_cap, 0.8, 1e-12);
  in = healthy();
  in.range_cap = 0.3;
  RC_CHECK_NEAR(once(in).speed_cap, 0.3, 1e-12);
  in.range_cap = std::nan("");
  RC_CHECK(once(in).speed_cap == 0.0);
  in = healthy();
  in.tilt = TiltState::warn;
  in.power.state = PowerState::derate;
  in.power.factor = 0.2;
  const Decision d = once(in);
  std::cout << "  a tilt warning and a power share of 0.2: cap " << d.speed_cap << " (the lowest of 0.5 and 0.4), degraded for: " << d.limit_causes << " (tilt 2 + power 8 = 10)\n\n";
  RC_CHECK_NEAR(d.speed_cap, 0.4, 1e-12);
  RC_CHECK(d.limit_causes == (limit_tilt | limit_power));
}

RC_TEST("a stop latches, every cause seen is kept, and the stop does not clear when the cause does") {
  Supervisor s{SupervisorConfig()};
  s.reset(healthy());
  Inputs in = healthy();
  RC_CHECK(s.step(in).mode == Mode::normal);
  in.tilt = TiltState::stop;
  Decision d = s.step(in);
  RC_CHECK(d.mode == Mode::stopped);
  RC_CHECK(d.stop_causes == cause_tilt);
  in.heartbeat_fresh = false;                                  // while stopped for the tilt, the link goes as well
  d = s.step(in);
  std::cout << "  stopped for a tilt, then the link was lost as well: causes on record: " << causes_text(d.stop_causes) << "\n";
  RC_CHECK(d.stop_causes == (cause_tilt | cause_link));
  in = healthy();                                              // both go away
  d = s.step(in);
  RC_CHECK(d.mode == Mode::stopped);                           // and it stays stopped
  RC_CHECK(d.stop_causes == (cause_tilt | cause_link));
  in.tilt = TiltState::stop;
  RC_CHECK(!s.reset(in));                                      // refused while a cause remains
  const Mode before = s.step(healthy()).mode;
  RC_CHECK(before == Mode::stopped);                           // and nothing changed
  RC_CHECK(s.reset(healthy()));
  d = s.step(healthy());
  std::cout << "  both went away: still " << mode_name(before) << " until reset; after the reset " << mode_name(d.mode) << ", causes " << causes_text(d.stop_causes) << "\n\n";
  RC_CHECK(d.mode == Mode::normal);
  RC_CHECK(d.stop_causes == 0u);
}

RC_TEST("over every combination of ten conditions, a stop cause always stops, and adding a fault never makes the answer less strict") {
  int stopped = 0;
  int degraded = 0;
  int normal = 0;
  int violations = 0;
  int cap_violations = 0;
  const int n = 1 << 10;
  for (int b = 0; b < n; ++b) {
    const Inputs in = from_bits(static_cast<unsigned>(b));
    const Decision d = once(in);
    const bool must_stop = stop_causes(in) != 0u;
    if (must_stop != (d.mode == Mode::stopped)) ++violations;
    if (d.mode == Mode::stopped) {
      ++stopped;
      if (d.speed_cap != 0.0) ++violations;
    } else if (d.mode == Mode::degraded) {
      ++degraded;
    } else {
      ++normal;
    }
    for (int k = 0; k < 10; ++k) {
      const Decision e = once(from_bits(static_cast<unsigned>(b) | (1u << k)));
      if (static_cast<int>(e.mode) < static_cast<int>(d.mode)) ++violations;       // normal < degraded < stopped
      if (e.speed_cap > d.speed_cap + 1e-12) ++cap_violations;
    }
  }
  std::cout << "  " << n << " combinations of 10 conditions: " << stopped << " stopped, " << degraded << " degraded, " << normal << " normal; " << violations << " mode violations, " << cap_violations << " cap violations when a fault was added (" << n * 10 << " additions)\n\n";
  RC_CHECK(violations == 0);
  RC_CHECK(cap_violations == 0);
  RC_CHECK(stopped + degraded + normal == n);
  RC_CHECK(normal == 1);                                         // only the all clear is normal
}

RC_TEST("the causes are written out in words for a log") {
  RC_CHECK(causes_text(0u) == "none");
  RC_CHECK(causes_text(cause_estop) == "estop");
  RC_CHECK(causes_text(cause_estop | cause_link) == "estop, link");
  RC_CHECK(causes_text(cause_fence | cause_tilt | cause_power) == "fence, tilt, power");
}
