#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

#include "solution.hpp"

namespace {
const double kWn = 8.0;
const double kZeta = 0.6;
}  // namespace

RC_TEST("a table lookup joins its points with lines and holds the ends beyond them") {
  const std::vector<double> xs = {0.0, 1.0, 3.0};
  const std::vector<double> ys = {10.0, 20.0, 0.0};
  RC_CHECK_NEAR(lookup(xs, ys, 0.5), 15.0, 1e-12);
  RC_CHECK_NEAR(lookup(xs, ys, 2.0), 10.0, 1e-12);
  RC_CHECK_NEAR(lookup(xs, ys, 1.0), 20.0, 1e-12);
  RC_CHECK_NEAR(lookup(xs, ys, -5.0), 10.0, 1e-12);
  RC_CHECK_NEAR(lookup(xs, ys, 9.0), 0.0, 1e-12);
  const std::vector<double> none;
  RC_CHECK_NEAR(lookup(none, none, 1.0), 0.0, 1e-12);
  const std::vector<double> one_x = {2.0};
  const std::vector<double> one_y = {7.0};
  RC_CHECK_NEAR(lookup(one_x, one_y, 100.0), 7.0, 1e-12);
}

RC_TEST("gains for a joint come from its inertia: kp is J w squared and kd is 2 zeta J w") {
  const Gains g = scheduled_gains(2.0, 8.0, 0.9);
  RC_CHECK_NEAR(g.kp, 128.0, 1e-9);
  RC_CHECK_NEAR(g.kd, 28.8, 1e-9);
  const Gains none = scheduled_gains(0.0, 8.0, 0.9);
  RC_CHECK_NEAR(none.kp, 0.0, 1e-12);
  RC_CHECK_NEAR(none.kd, 0.0, 1e-12);
}

RC_TEST("gains tuned for one pose are wrong at the others, and worst at the heaviest") {
  const Gains folded = scheduled_gains(arm_inertia(0.0), kWn, kZeta);
  std::cout << "  gains tuned folded: kp " << std::fixed << std::setprecision(1) << folded.kp << ", kd " << folded.kd
            << "\n  extension   inertia   overshoot   settling (s)\n";
  double at_folded = 0.0;
  double at_full = 0.0;
  double previous = -1.0;
  bool rises = true;
  for (double e : {0.0, 0.25, 0.5, 0.75, 1.0}) {
    const double j = arm_inertia(e);
    const JointResult r = run_joint(j, folded.kp, folded.kd);
    std::cout << "  " << std::setw(9) << std::setprecision(2) << e << "   " << std::setw(7) << j << "   "
              << std::setw(9) << std::setprecision(3) << r.overshoot << "   " << std::setw(12) << std::setprecision(2)
              << r.settling_time << "\n";
    if (e == 0.0) at_folded = r.overshoot;
    if (e == 1.0) at_full = r.overshoot;
    if (e > 0.0 && !(r.overshoot > previous)) rises = false;
    previous = r.overshoot;
  }
  std::cout << "\n";
  RC_CHECK(at_folded < 0.12);
  RC_CHECK(rises);
  RC_CHECK(at_full > 0.3);
}

RC_TEST("gains matched to the inertia give the same response at every pose") {
  std::cout << "  gains matched at each pose: extension   kp      kd     overshoot   settling (s)\n";
  double lowest = 1e9;
  double highest = -1e9;
  double slowest = 0.0;
  double fastest = 1e9;
  for (double e : {0.0, 0.25, 0.5, 0.75, 1.0}) {
    const double j = arm_inertia(e);
    const Gains g = scheduled_gains(j, kWn, kZeta);
    const JointResult r = run_joint(j, g.kp, g.kd);
    std::cout << "                              " << std::setw(9) << std::fixed << std::setprecision(2) << e << "  "
              << std::setw(6) << std::setprecision(1) << g.kp << "  " << std::setw(5) << g.kd << "   " << std::setw(9)
              << std::setprecision(3) << r.overshoot << "   " << std::setw(12) << std::setprecision(2)
              << r.settling_time << "\n";
    if (r.overshoot < lowest) lowest = r.overshoot;
    if (r.overshoot > highest) highest = r.overshoot;
    if (r.settling_time > slowest) slowest = r.settling_time;
    if (r.settling_time < fastest) fastest = r.settling_time;
  }
  std::cout << "\n";
  RC_CHECK(fastest > 0.0);
  RC_CHECK(highest - lowest < 0.03);
  RC_CHECK(slowest < 1.2 * fastest);
}

RC_TEST("a schedule made at a few poses does the same between them, and more knots do better") {
  const std::vector<double> coarse = {0.0, 1.0};
  const std::vector<double> medium = {0.0, 0.5, 1.0};
  const std::vector<double> fine = {0.0, 0.25, 0.5, 0.75, 1.0};
  RC_CHECK(GainSchedule(fine, kWn, kZeta).knots() == 5);

  // The design settles in the same time at every pose. How far the settling times of 21 poses spread, slowest minus
  // fastest, says how far a table strays from that between its knots.
  std::cout << "  knots   settling, fastest to slowest over 21 poses (s)   spread (s)\n";
  double spread[3] = {0.0, 0.0, 0.0};
  const std::vector<double>* tables[3] = {&coarse, &medium, &fine};
  for (int t = 0; t < 3; ++t) {
    const GainSchedule schedule(*tables[t], kWn, kZeta);
    double fastest = 1e9;
    double slowest = 0.0;
    for (int i = 0; i <= 20; ++i) {
      const double e = i / 20.0;
      const Gains g = schedule.at(e);
      const JointResult r = run_joint(arm_inertia(e), g.kp, g.kd);
      const double settle = r.settling_time < 0.0 ? 99.0 : r.settling_time;
      if (settle < fastest) fastest = settle;
      if (settle > slowest) slowest = settle;
    }
    spread[t] = slowest - fastest;
    std::cout << "  " << std::setw(5) << tables[t]->size() << "   " << std::setw(22) << std::fixed << std::setprecision(2)
              << fastest << " to " << std::setw(5) << slowest << std::setw(22) << std::setprecision(3) << spread[t] << "\n";
  }
  std::cout << "\n";
  RC_CHECK(spread[0] > 0.0);
  RC_CHECK(spread[1] < spread[0]);
  RC_CHECK(spread[2] < spread[1]);
  RC_CHECK(spread[2] < 0.05);
}

RC_TEST("outside the table the schedule holds its end gains instead of extrapolating") {
  const GainSchedule schedule({0.0, 0.5, 1.0}, kWn, kZeta);
  const Gains below = schedule.at(-3.0);
  const Gains first = schedule.at(0.0);
  const Gains above = schedule.at(7.0);
  const Gains last = schedule.at(1.0);
  RC_CHECK(first.kp > 0.0);
  RC_CHECK_NEAR(below.kp, first.kp, 1e-12);
  RC_CHECK_NEAR(below.kd, first.kd, 1e-12);
  RC_CHECK_NEAR(above.kp, last.kp, 1e-12);
  RC_CHECK_NEAR(above.kd, last.kd, 1e-12);
  std::cout << "  gains at extension -3: kp " << std::fixed << std::setprecision(1) << below.kp << ", at 0: " << first.kp
            << "; at 7: " << above.kp << ", at 1: " << last.kp << "\n\n";
}
