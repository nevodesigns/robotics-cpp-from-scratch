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
  {
    auto d = make();
    const DriveState s = d->state();
    if (!state_is_finite(s) || std::fabs(s.v) > 1e-9 || std::fabs(s.w) > 1e-9) broken.push_back("rest: it did not start at rest, or its state was not a number");
  }
  {
    auto d = make();
    DriveCommand c;
    c.v = 0.5;
    drive_run(*d, 1.0, 0.001, &c);
    const DriveState s = d->state();
    if (!(s.x > 0.1)) broken.push_back("forward: a positive speed did not move it forward");
    else if (std::fabs(s.y) > 0.05 || std::fabs(s.theta) > 0.05) broken.push_back("forward: a straight command made it turn");
  }
  {
    auto d = make();
    DriveCommand c;
    c.w = 0.5;
    drive_run(*d, 1.0, 0.001, &c);
    if (!(d->state().theta > 0.1)) broken.push_back("turn: a positive turn rate did not turn it to the left");
  }
  {
    auto d = make();
    DriveCommand c;
    c.v = 100.0;
    c.w = 100.0;
    drive_run(*d, 3.0, 0.001, &c);
    const DriveState s = d->state();
    if (std::fabs(s.v) > e.max_v * 1.01 || std::fabs(s.w) > e.max_w * 1.01) broken.push_back("limit: it went faster than its limit");
    d->stop();
    drive_run(*d, e.stop_seconds, 0.001, nullptr);
    const DriveState t = d->state();
    if (std::fabs(t.v) > 0.02 || std::fabs(t.w) > 0.02) broken.push_back("stop: it was still moving after the time given for a stop");
  }
  {
    auto d = make();
    DriveCommand c;
    c.v = 0.5;
    d->command(c);
    drive_run(*d, e.watchdog + e.stop_seconds, 0.001, nullptr);
    if (std::fabs(d->state().v) > 0.02) broken.push_back("watchdog: it kept going after the commands stopped");
  }
  {
    auto d = make();
    DriveCommand c;
    c.v = std::nan("");
    c.w = std::nan("");
    d->command(c);
    drive_run(*d, 1.0, 0.001, nullptr);
    const DriveState s = d->state();
    if (!state_is_finite(s) || std::fabs(s.v) > 0.02) broken.push_back("nan: a command that is not a number moved it or spoiled its state");
  }
  {
    auto d = make();
    DriveCommand c;
    c.v = 0.5;
    double travelled = 0.0;
    for (int i = 0; i < 2000; ++i) {
      if (i % 100 == 0) d->command(c);
      d->step(0.001);
      travelled += d->state().v * 0.001;
    }
    const DriveState s = d->state();
    const double away = std::hypot(s.x, s.y);
    if (std::fabs(away - travelled) > 0.05 * std::max(travelled, 1e-9)) broken.push_back("odometry: its pose disagrees with its own speed");
  }
  {
    auto a = make();
    auto b = make();
    DriveCommand c;
    c.v = 0.7;
    c.w = 0.4;
    drive_run(*a, 2.0, 0.001, &c);
    drive_run(*b, 2.0, 0.001, &c);
    const DriveState p = a->state();
    const DriveState q = b->state();
    if (std::fabs(p.x - q.x) + std::fabs(p.y - q.y) + std::fabs(p.theta - q.theta) > 1e-12) broken.push_back("repeatable: the same commands gave a different pose");
  }
  {
    auto a = make();
    auto b = make();
    DriveCommand c;
    c.v = 0.5;
    c.w = 0.3;
    drive_run(*a, 1.0, 0.001, &c);
    drive_run(*b, 1.0, 0.01, &c);
    const DriveState p = a->state();
    const DriveState q = b->state();
    if (std::hypot(p.x - q.x, p.y - q.y) > 0.01) broken.push_back("step: a step ten times longer gave a different place");
  }
  {
    auto d = make();
    DriveCommand c;
    c.v = 0.5;
    drive_run(*d, 0.5, 0.001, &c);
    const DriveState before = d->state();
    d->step(0.0);
    const DriveState after = d->state();
    if (!state_is_finite(after) || std::fabs(after.x - before.x) > 1e-12 || std::fabs(after.v - before.v) > 1e-12) broken.push_back("zero: a step of no time changed the state or spoiled it");
  }
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
