#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iomanip>
#include <iostream>

#include "solution.hpp"

namespace {
const double kKp = 100.0;
const double kKd = 10.0;
}  // namespace

RC_TEST("sampled fast enough, the loop settles on the target with a modest overshoot") {
  const LoopResult r = run_sampled(0.001, kKp, kKd);
  std::cout << "  kp 100, kd 10, period 1 ms: overshoot " << std::fixed << std::setprecision(3) << r.overshoot
            << ", error at the end " << std::scientific << std::setprecision(1) << r.final_error << "\n\n";
  RC_CHECK(r.stable);
  RC_CHECK(r.overshoot > 0.05 && r.overshoot < 0.3);
  RC_CHECK(std::fabs(r.final_error) < 1e-3);
}

RC_TEST("a slower controller overshoots more, and the overshoot grows faster the slower it gets") {
  std::cout << "  period (s)   overshoot\n";
  double previous = 0.0;
  bool rises = true;
  double first = 0.0;
  double last = 0.0;
  for (double period : {0.001, 0.01, 0.05, 0.1, 0.15}) {
    const LoopResult r = run_sampled(period, kKp, kKd);
    std::cout << "  " << std::setw(9) << std::fixed << std::setprecision(3) << period << "    " << std::setw(8)
              << std::setprecision(3) << r.overshoot << (r.stable ? "" : "   unstable") << "\n";
    if (period > 0.001 && !(r.overshoot > previous)) rises = false;
    if (period == 0.001) first = r.overshoot;
    last = r.overshoot;
    previous = r.overshoot;
  }
  std::cout << "\n";
  RC_CHECK(first > 0.0);
  RC_CHECK(rises);
  RC_CHECK(last > 3.0 * first);
}

RC_TEST("past a limit the same gains no longer hold the joint") {
  const LoopResult held = run_sampled(0.05, kKp, kKd);
  const LoopResult past = run_sampled(0.3, kKp, kKd);
  std::cout << "  kp 100, kd 10: period 0.05 s stable " << (held.stable ? "yes" : "no") << ", period 0.3 s stable "
            << (past.stable ? "yes" : "no") << "\n\n";
  RC_CHECK(held.stable);
  RC_CHECK(!past.stable);
  RC_CHECK(!run_sampled(2.0, kKp, kKd).stable);
}

RC_TEST("the largest stable period is a sharp edge: stable just under it, not just over") {
  const double limit = largest_stable_period(kKp, kKd);
  std::cout << "  largest stable period at kp 100, kd 10: " << std::fixed << std::setprecision(4) << limit << " s\n\n";
  RC_CHECK(limit > 0.15 && limit < 0.22);
  RC_CHECK(run_sampled(limit * 0.97, kKp, kKd).stable);
  RC_CHECK(!run_sampled(limit * 1.05, kKp, kKd).stable);
}

RC_TEST("stiffer gains need a faster controller, roughly in proportion to one over the square root of kp") {
  std::cout << "  kp    kd   largest period (s)   limit times natural frequency\n";
  const double kps[] = {25.0, 100.0, 400.0};
  const double kds[] = {5.0, 10.0, 20.0};
  double limits[3] = {0.0, 0.0, 0.0};
  for (int i = 0; i < 3; ++i) {
    limits[i] = largest_stable_period(kps[i], kds[i]);
    std::cout << "  " << std::setw(3) << std::fixed << std::setprecision(0) << kps[i] << "  " << std::setw(3)
              << kds[i] << "   " << std::setw(18) << std::setprecision(4) << limits[i] << "   " << std::setw(6)
              << std::setprecision(2) << limits[i] * std::sqrt(kps[i]) << "\n";
  }
  std::cout << "\n";
  RC_CHECK(limits[0] > limits[1]);
  RC_CHECK(limits[1] > limits[2]);
  RC_CHECK(limits[0] / limits[2] > 3.4 && limits[0] / limits[2] < 4.6);
}

RC_TEST("a period chosen as a share of the limit costs more overshoot the larger the share") {
  const double baseline = run_sampled(0.001, kKp, kKd).overshoot;
  std::cout << "  kp 100, kd 10, overshoot at 1 ms: " << std::fixed << std::setprecision(3) << baseline << "\n"
            << "  share of the limit   period (s)   overshoot   against 1 ms\n";
  double tenth_ratio = 0.0;
  double half_ratio = 0.0;
  for (double share : {0.05, 0.1, 0.25, 0.5}) {
    const double period = period_for(kKp, kKd, share);
    const LoopResult r = run_sampled(period, kKp, kKd);
    const double ratio = baseline > 0.0 ? r.overshoot / baseline : 0.0;
    std::cout << "  " << std::setw(18) << std::setprecision(2) << share << "   " << std::setw(10)
              << std::setprecision(4) << period << "   " << std::setw(9) << std::setprecision(3) << r.overshoot
              << "   " << std::setw(8) << std::setprecision(2) << ratio << "\n";
    if (share == 0.1) tenth_ratio = ratio;
    if (share == 0.5) half_ratio = ratio;
  }
  std::cout << "\n";
  RC_CHECK_NEAR(period_for(kKp, kKd, 0.0), 0.0, 1e-12);
  RC_CHECK(tenth_ratio > 1.0 && tenth_ratio < 1.25);
  RC_CHECK(half_ratio > 1.5);
}
