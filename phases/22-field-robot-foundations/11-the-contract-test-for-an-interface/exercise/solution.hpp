#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <cstddef>
#include <functional>
#include <memory>
#include <rc/hal/interfaces.hpp>
#include <string>
#include <vector>

using namespace rc::hal;

// What a drive is expected to be, so that the contract can tell a promise kept from one broken.
struct DriveExpect {
  double max_v = 1.0;
  double max_w = 2.0;
  double stop_seconds = 1.0;  // the time within which a stop must have taken effect
  double watchdog = 0.5;      // the time without a command after which the drive stops itself
};

using DriveFactory = std::function<std::unique_ptr<Drive>()>;

// Steps a drive for a time, resending a command every 0.1 s when one is given (so that a watchdog stays quiet).
inline void drive_run(Drive& d, double seconds, double dt, const DriveCommand* resend) {
  const int steps = static_cast<int>(seconds / dt + 0.5);
  const int every = static_cast<int>(0.1 / dt + 0.5);
  for (int i = 0; i < steps; ++i) {
    if (resend != nullptr && i % every == 0) d.command(*resend);
    d.step(dt);
  }
}

inline bool state_is_finite(const DriveState& s) {
  return std::isfinite(s.v) && std::isfinite(s.w) && std::isfinite(s.x) && std::isfinite(s.y) && std::isfinite(s.theta);
}

// Every promise a drive broke, each as "tag: what happened". An empty list is a drive that keeps them all.
inline std::vector<std::string> check_drive_contract(const DriveFactory& make, const DriveExpect& e) {
  std::vector<std::string> broken;
  (void)make;  // TODO: for each promise make a fresh drive, give it the commands the promise needs, step it, and add "tag: what happened" to broken when the promise is not kept
  (void)e;
  return broken;
}

// The same for a row of joints. Bus is any class with joint_count, set_targets, step, positions and at_target; make gives a fresh one.
template <class Bus, class Make>
std::vector<std::string> check_joint_contract(Make make, std::size_t joints, double max_rate) {
  std::vector<std::string> broken;
  {
    Bus b = make();
    const std::vector<double> start = b.positions();
    bool finite = true;
    for (std::size_t i = 0; i < start.size(); ++i) finite = finite && std::isfinite(start[i]);
    if (start.size() != joints || !finite) broken.push_back("size: it did not have the joints it was made with, or they were not numbers");
    if (b.set_targets(std::vector<double>(joints + 1, 0.0))) broken.push_back("size: a command with the wrong number of joints was accepted");
    std::vector<double> bad(joints, 0.0);
    bad[0] = std::nan("");
    if (b.set_targets(bad)) broken.push_back("nan: a target that is not a number was accepted");
    for (int i = 0; i < 50; ++i) b.step(0.01);
    const std::vector<double> after = b.positions();
    for (std::size_t i = 0; i < after.size() && i < start.size(); ++i) {
      if (!std::isfinite(after[i]) || std::fabs(after[i] - start[i]) > 1e-9) {
        broken.push_back("nan: a refused command still moved it, or spoiled it");
        break;
      }
    }
  }
  {
    Bus b = make();
    b.set_targets(std::vector<double>(joints, 1.0));
    bool too_fast = false;
    std::vector<double> last = b.positions();
    const double dt = 0.01;
    for (int i = 0; i < 20; ++i) {
      b.step(dt);
      const std::vector<double> now = b.positions();
      for (std::size_t k = 0; k < now.size() && k < last.size(); ++k)
        if (std::fabs(now[k] - last[k]) > max_rate * dt * (1.0 + 1e-6) + 1e-12) too_fast = true;
      last = now;
    }
    if (too_fast) broken.push_back("rate: a joint moved faster than its rate");
    double t = 0.2;
    while (!b.at_target(1e-6) && t < 1.0 / max_rate + 1.0) {
      b.step(dt);
      t += dt;
    }
    if (!b.at_target(1e-6)) broken.push_back("reach: it never got to its target");
    const std::vector<double> there = b.positions();
    for (int i = 0; i < 100; ++i) b.step(dt);
    const std::vector<double> later = b.positions();
    for (std::size_t k = 0; k < there.size() && k < later.size(); ++k) {
      if (std::fabs(later[k] - there[k]) > 1e-9) {
        broken.push_back("hold: it drifted off its target");
        break;
      }
    }
  }
  return broken;
}


#endif
