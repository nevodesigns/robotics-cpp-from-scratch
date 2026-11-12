// rc/hal/interfaces.hpp
//
// The generic hardware interfaces of the field courses, from lesson 22-01.
//
// Drive is a base that takes a forward speed and a turn rate; ArmBus and LegBus are a row of joints that take target angles. Everything above them is written against these classes, so a simulator, a fake and a board are interchangeable. FakeDrive lags the command with a time constant (0.2 s here), limits it, and stops by itself when no command arrives for its watchdog time; FakeArmBus and FakeLegBus move each joint at most a maximum rate.
//
// The measured time to 63.2 percent of a step was 0.200 s for a time constant of 0.2 s; a base told to go 5 m/s was held to 1.000 m/s; the watchdog brought a 0.5 m/s base below 0.01 m/s after 1.265 s; a stop at 1 m/s coasted 0.200 m.

#ifndef RC_RC_HAL_INTERFACES_HPP
#define RC_RC_HAL_INTERFACES_HPP

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

namespace rc {
namespace hal {

// A command to a drive base: a forward speed in m/s and a turn rate in rad/s (positive turns left).
struct DriveCommand {
  double v = 0.0;
  double w = 0.0;
};

// What a drive base reports: the speed it is actually doing, and where it believes it is.
struct DriveState {
  double v = 0.0;
  double w = 0.0;
  double x = 0.0;
  double y = 0.0;
  double theta = 0.0;
};

// A drive base. command() says what is wanted, stop() says that nothing is, step(dt) lets time pass (a fake moves itself, a board has its own clock and may do nothing), state() reports.
class Drive {
 public:
  virtual ~Drive() = default;
  virtual void command(const DriveCommand& c) = 0;
  virtual void stop() = 0;
  virtual void step(double dt) = 0;
  virtual DriveState state() const = 0;
};

// What a base can do at most, and how long it waits for the next command before it stops by itself.
struct DriveLimits {
  double max_v = 1.0;
  double max_w = 2.0;
  double tau = 0.2;       // the time constant of the response to a command, in seconds
  double watchdog = 0.5;  // seconds without a command before the base stops itself
};

// A drive base that behaves like the hardware: the command is limited, answered with a first order lag, and dropped if it goes stale.
class FakeDrive : public Drive {
 public:
  explicit FakeDrive(const DriveLimits& limits = DriveLimits()) : limits_(limits) {}

  void command(const DriveCommand& c) override {
    // A command that is not a number is not a command. std::min and std::max would turn a NaN into the limit, which is full speed.
    if (!std::isfinite(c.v) || !std::isfinite(c.w)) {
      stop();
      return;
    }
    want_.v = std::max(-limits_.max_v, std::min(limits_.max_v, c.v));
    want_.w = std::max(-limits_.max_w, std::min(limits_.max_w, c.w));
    since_command_ = 0.0;
  }

  void stop() override {
    want_ = DriveCommand();
    since_command_ = 0.0;
  }

  void step(double dt) override {
    since_command_ += dt;
    DriveCommand target = want_;
    if (since_command_ > limits_.watchdog) target = DriveCommand();
    const double alpha = 1.0 - std::exp(-dt / limits_.tau);
    state_.v += alpha * (target.v - state_.v);
    state_.w += alpha * (target.w - state_.w);
    const double mid = state_.theta + 0.5 * state_.w * dt;
    state_.x += state_.v * std::cos(mid) * dt;
    state_.y += state_.v * std::sin(mid) * dt;
    state_.theta += state_.w * dt;
  }

  DriveState state() const override { return state_; }

 private:
  DriveLimits limits_;
  DriveCommand want_;
  DriveState state_;
  double since_command_ = 0.0;
};

// A row of joints that are given target angles and move towards them at no more than a maximum rate, which is what a hobby servo bus does.
class JointBus {
 public:
  JointBus(std::size_t count, double max_rate) : max_rate_(max_rate), target_(count, 0.0), position_(count, 0.0) {}
  virtual ~JointBus() = default;

  std::size_t joint_count() const { return position_.size(); }

  // Rejects a command with the wrong number of joints rather than half applying it.
  bool set_targets(const std::vector<double>& targets) {
    if (targets.size() != target_.size()) return false;
    for (std::size_t i = 0; i < targets.size(); ++i)
      if (!std::isfinite(targets[i])) return false;
    target_ = targets;
    return true;
  }

  void step(double dt) {
    const double reach = max_rate_ * dt;
    for (std::size_t i = 0; i < position_.size(); ++i) {
      const double gap = target_[i] - position_[i];
      position_[i] += std::max(-reach, std::min(reach, gap));
    }
  }

  const std::vector<double>& positions() const { return position_; }

  bool at_target(double tolerance) const {
    for (std::size_t i = 0; i < position_.size(); ++i)
      if (std::fabs(target_[i] - position_[i]) > tolerance) return false;
    return true;
  }

 private:
  double max_rate_;
  std::vector<double> target_;
  std::vector<double> position_;
};

// An arm is a row of joints, a leg set is a row of joints, and the interface for both is the same row; the names are kept apart so that a program cannot hand an arm's targets to a leg.
class ArmBus : public JointBus {
 public:
  ArmBus(std::size_t joints, double max_rate) : JointBus(joints, max_rate) {}
};

class LegBus : public JointBus {
 public:
  LegBus(std::size_t legs, std::size_t joints_per_leg, double max_rate) : JointBus(legs * joints_per_leg, max_rate), legs_(legs) {}
  std::size_t leg_count() const { return legs_; }

 private:
  std::size_t legs_;
};


}  // namespace hal
}  // namespace rc

#endif
