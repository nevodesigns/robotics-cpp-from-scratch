#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iomanip>
#include <iostream>

#include "solution.hpp"

namespace {

// A robot whose left wheel is 1 percent bigger than the software thinks, driving along a corridor that it cannot see.
LocalisationConfig slip() {
  LocalisationConfig c;
  c.error.left_scale = 1.01;
  return c;
}

LocalisationResult run(const LocalisationConfig& c, double seconds = 70.0) { return run_localisation(c, seconds); }

}  // namespace

RC_TEST("the error between two positions is the distance, the buffer is the stopping distance plus the drift since the last fix, and a fix pulls the belief towards the measurement") {
  rc::sim::Pose a;
  rc::sim::Pose b;
  b.x = 3.0;
  b.y = 4.0;
  RC_CHECK_NEAR(position_error(a, b), 5.0, 1e-12);
  RC_CHECK_NEAR(stop_buffer(0.5, 0.1, 5.0), 1.0, 1e-12);
  RC_CHECK_NEAR(stop_buffer(0.5, 0.1, 0.0), 0.5, 1e-12);
  RC_CHECK_NEAR(stop_buffer(0.5, -1.0, -3.0), 0.5, 1e-12);
  rc::nav::Odometry belief(0.3, a);
  rc::nav::Filter1D fx(rc::nav::Estimate{0.0, 1.0}, 0.0004);
  rc::nav::Filter1D fy(rc::nav::Estimate{0.0, 1.0}, 0.0004);
  apply_fix(fx, fy, 1.0, -1.0, 0.1, belief);                          // a prior of variance 1 and a fix of variance 0.01: nearly all of the way
  RC_CHECK(belief.pose().x > 0.97 && belief.pose().x < 1.0);
  RC_CHECK(belief.pose().y < -0.97 && belief.pose().y > -1.0);
  RC_CHECK_NEAR(belief.pose().theta, 0.0, 1e-12);                     // the heading is not touched
  RC_CHECK(fx.estimate().variance < 0.01);
  std::cout << "  a prior of variance 1 and a fix of variance 0.01 put the belief at (" << std::fixed << std::setprecision(3) << belief.pose().x << ", " << belief.pose().y << ") for a fix at (1, -1)\n\n";
}

RC_TEST("a wheel base that is wrong costs nothing driving straight, and a wheel that is wrong by 1 percent costs a distance that grows with the square of the distance") {
  LocalisationConfig base;
  base.error.base_scale = 1.05;
  const LocalisationResult b = run(base);
  const LocalisationResult s = run(slip());
  const LocalisationResult perfect = run(LocalisationConfig());
  std::cout << "  driving straight on, the position error after 10, 20 and 30 m:\n";
  std::cout << "  wheel base 5 percent out:    " << std::fixed << std::setprecision(2) << b.error_at_10 << " " << b.error_at_20 << " " << b.error_at_30 << " m\n";
  std::cout << "  left wheel 1 percent big:    " << s.error_at_10 << " " << s.error_at_20 << " " << s.error_at_30 << " m (" << std::setprecision(1) << s.error_at_20 / s.error_at_10 << " and " << s.error_at_30 / s.error_at_10 << " times the first)\n\n";
  RC_CHECK(perfect.max_error == 0.0);
  RC_CHECK(b.max_error < 1e-9);
  RC_CHECK(s.error_at_10 > 1.5 && s.error_at_10 < 1.8);
  RC_CHECK(s.error_at_20 / s.error_at_10 > 3.5 && s.error_at_20 / s.error_at_10 < 4.5);
  RC_CHECK(s.error_at_30 / s.error_at_10 > 8.0 && s.error_at_30 / s.error_at_10 < 9.5);
}

RC_TEST("beacon fixes bound the error, and the more often they come the closer it is held, and when they stop the error grows again") {
  const double periods[] = {-1.0, 10.0, 5.0, 2.0};
  LocalisationResult r[4];
  std::cout << "  the same 1 percent wheel, with a position fix:\n  every      fixes   error at 30 m   largest\n";
  for (int i = 0; i < 4; ++i) {
    LocalisationConfig c = slip();
    c.beacon_period = periods[i];
    r[i] = run(c);
    std::cout << "  " << (periods[i] < 0 ? std::string("never ") : std::to_string(static_cast<int>(periods[i])) + " s  ") << "   " << std::setw(4) << r[i].fixes << "    " << std::fixed << std::setprecision(2) << std::setw(6) << r[i].error_at_30 << " m      " << std::setw(6) << r[i].max_error << " m\n";
  }
  LocalisationConfig dying = slip();
  dying.beacon_period = 5.0;
  dying.beacon_dead_at = 20.0;
  const LocalisationResult d = run(dying);
  std::cout << "  fixes every 5 s that die at 20 s: " << d.fixes << " fixes, error at 30 m " << d.error_at_30 << " m\n\n";
  RC_CHECK(r[0].fixes == 0);
  RC_CHECK(r[1].error_at_30 < r[0].error_at_30 / 2.0);
  RC_CHECK(r[2].error_at_30 < r[1].error_at_30);
  RC_CHECK(r[3].error_at_30 < r[2].error_at_30);
  RC_CHECK(r[3].error_at_30 < 0.15 * r[0].error_at_30);
  RC_CHECK(r[3].max_error < 0.15 * r[0].max_error);
  RC_CHECK(d.error_at_30 > 4.0 * r[2].error_at_30);
  RC_CHECK(d.fixes == 3);
}

RC_TEST("a robot that dead reckons believes it is in the middle of the corridor while it leaves it, and a fence on the believed position never trips") {
  const LocalisationResult r = run(slip());
  std::cout << "  no fence at all: the true position passed the edge of the corridor (1.5 m) after " << std::fixed << std::setprecision(1) << r.truth_left_at << " m, and the robot went on to y = " << r.final_lateral << " m, believing itself on the axis\n";
  RC_CHECK(r.truth_left_at > 9.0 && r.truth_left_at < 10.5);
  RC_CHECK(!r.stopped);
  RC_CHECK(r.final_lateral < -10.0);
  LocalisationConfig fenced = slip();
  fenced.use_buffer = true;
  fenced.assumed_drift = 0.0;
  fenced.stopping_distance = 0.0;
  const LocalisationResult f = run(fenced);
  std::cout << "  a fence on the believed position with no buffer: stopped " << (f.stopped ? "yes" : "NO") << "\n\n";
  RC_CHECK(!f.stopped);
}

RC_TEST("the buffer stops the robot before the truth leaves only if the drift it assumes is as big as the drift there is, and drift per metre is not a constant") {
  const double rates[] = {0.05, 0.1, 0.2, 0.4};
  LocalisationResult r[4];
  std::cout << "  a corridor 3 m wide, the robot's true position leaves it after about 9.6 m. The buffer stopping distance 0.5 m plus an assumed drift for each metre since the last fix:\n";
  std::cout << "  assumed drift    stops after    the truth was then\n";
  for (int i = 0; i < 4; ++i) {
    LocalisationConfig c = slip();
    c.use_buffer = true;
    c.assumed_drift = rates[i];
    r[i] = run(c);
    std::cout << "  " << std::fixed << std::setprecision(2) << std::setw(8) << rates[i] << " m/m   " << std::setprecision(1) << std::setw(6) << r[i].stopped_at << " m     " << (r[i].truth_outside_at_stop ? "OUTSIDE the corridor" : "inside") << "\n";
  }
  const double actual = 1.5 / r[0].truth_left_at;
  std::cout << "  the real drift when the truth reaches the edge: 1.5 m over " << r[0].truth_left_at << " m is " << std::setprecision(3) << actual << " m for each metre, and it is still growing\n\n";
  RC_CHECK(r[0].stopped && r[1].stopped && r[2].stopped && r[3].stopped);
  RC_CHECK(r[0].stopped_at > r[1].stopped_at && r[1].stopped_at > r[2].stopped_at && r[2].stopped_at > r[3].stopped_at);
  RC_CHECK(r[0].truth_outside_at_stop && r[1].truth_outside_at_stop);
  RC_CHECK(!r[2].truth_outside_at_stop && !r[3].truth_outside_at_stop);
  RC_CHECK(r[2].stopped_at < r[0].truth_left_at);
  RC_CHECK(r[1].stopped_at > r[0].truth_left_at);
  RC_CHECK(actual > 0.1 && actual < 0.2);
}

RC_TEST("fixes shorten the distance the buffer has to cover, so the robot with the same assumed drift goes further before it has to stop") {
  const double rates[] = {0.05, 0.1, 0.2};
  std::cout << "  the same fence, with a fix every 5 s:\n";
  LocalisationResult r[3];
  for (int i = 0; i < 3; ++i) {
    LocalisationConfig c = slip();
    c.use_buffer = true;
    c.assumed_drift = rates[i];
    c.beacon_period = 5.0;
    r[i] = run(c);
    std::cout << "  assumed " << std::fixed << std::setprecision(2) << rates[i] << " m/m: stopped after " << std::setprecision(1) << r[i].stopped_at << " m, the truth " << (r[i].truth_outside_at_stop ? "OUTSIDE" : "inside") << " (lateral " << std::setprecision(2) << r[i].final_lateral << " m)\n";
  }
  LocalisationConfig nofix = slip();
  nofix.use_buffer = true;
  nofix.assumed_drift = 0.2;
  const LocalisationResult n = run(nofix);
  std::cout << "  and without fixes the same 0.20 stopped after " << std::setprecision(1) << n.stopped_at << " m\n\n";
  RC_CHECK(r[0].stopped && r[1].stopped && r[2].stopped);
  RC_CHECK(r[2].stopped_at > n.stopped_at + 2.0);
  RC_CHECK(r[2].stopped_at > 8.0 && r[2].stopped_at < 9.5);
  RC_CHECK(!r[2].truth_outside_at_stop);
  RC_CHECK(r[0].truth_outside_at_stop);
  RC_CHECK(r[2].stopped_at > 0.0);
}
