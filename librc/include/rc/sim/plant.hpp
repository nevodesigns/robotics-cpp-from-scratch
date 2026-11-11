// rc/sim/plant.hpp
//
// A plant interface and the contract a plant must keep, from lesson 18-02, graduated.
//
// A controller is written against `Plant` (reset, apply a command for a step, position, velocity, limit) and runs unchanged against a simulation, a replay or hardware. SimMotor follows the command with a lag and clamps it.
// `check_contract` runs six rules against any plant and names each one broken: reset, rest, saturation, symmetry, determinism and consistency.
//
// A motor of gain 2 and time constant 0.1 s given a command of 3 reached 3.804 (63.4 percent of 6.0) after 0.1 s and 5.961 after 0.5 s. A SimMotor broke no rule. A plant with no clamp broke saturation, one reporting the previous position
// broke consistency, one that was not repeatable broke determinism, one that forgot its speed on reset broke reset and one that did more in one direction broke symmetry. The same controller had a peak speed of 14.70 on the contract-keeping plant and 22.71 on the one with no clamp.

#ifndef RC_SIM_PLANT_HPP
#define RC_SIM_PLANT_HPP

#include <cmath>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace rc {
namespace sim {

// What a controller may ask of the thing it controls. The controller is written against this and nothing else, so the same controller
// runs against a simulation, a replay, or a motor on a bench, and each of those is one class that implements it.
class Plant {
 public:
  virtual ~Plant() {}
  virtual void reset() = 0;                                 // back to rest at the origin
  virtual void apply(double command, double dt) = 0;       // drive with `command` for `dt` seconds
  virtual double position() const = 0;
  virtual double velocity() const = 0;
  virtual double limit() const = 0;                         // the largest command it will act on, either way
};

// A motor with a lag: the speed follows `gain` times the command with time constant `tau`, and the command is clamped to plus or minus `limit`.
class SimMotor : public Plant {
 public:
  SimMotor(double gain, double tau, double limit) : gain_(gain), tau_(tau > 1e-6 ? tau : 1e-6), limit_(limit > 0.0 ? limit : 1.0) {}

  void reset() override {
    position_ = 0.0;
    velocity_ = 0.0;
  }

  void apply(double command, double dt) override {
    const double u = command > limit_ ? limit_ : (command < -limit_ ? -limit_ : command);
    velocity_ += (gain_ * u - velocity_) / tau_ * dt;
    position_ += velocity_ * dt;
  }

  double position() const override { return position_; }
  double velocity() const override { return velocity_; }
  double limit() const override { return limit_; }

 private:
  double gain_ = 1.0;
  double tau_ = 0.1;
  double limit_ = 1.0;
  double position_ = 0.0;
  double velocity_ = 0.0;
};

// Check a kind of plant against what every plant must do. `make` gives a new plant each time it is called. The answer is the name of each
// rule that was broken: empty means the plant keeps the contract.
//   "reset"        after being driven and reset, the position and velocity are zero
//   "rest"         a zero command from rest changes nothing
//   "saturation"   a command ten times the limit does exactly what the limit does
//   "symmetry"     the same command with the opposite sign gives the opposite position
//   "determinism"  two new plants given the same commands are in the same state
//   "consistency"  over one step the position moves by the new velocity times the step
inline std::vector<std::string> check_contract(const std::function<std::unique_ptr<Plant>()>& make) {
  std::vector<std::string> broken;
  const double dt = 0.01;

  {
    std::unique_ptr<Plant> p = make();
    for (int i = 0; i < 50; ++i) p->apply(0.5 * p->limit(), dt);
    p->reset();
    if (p->position() != 0.0 || p->velocity() != 0.0) broken.push_back("reset");
  }
  {
    std::unique_ptr<Plant> p = make();
    p->reset();
    for (int i = 0; i < 20; ++i) p->apply(0.0, dt);
    if (std::fabs(p->position()) > 1e-12 || std::fabs(p->velocity()) > 1e-12) broken.push_back("rest");
  }
  {
    std::unique_ptr<Plant> a = make();
    std::unique_ptr<Plant> b = make();
    for (int i = 0; i < 100; ++i) {
      a->apply(a->limit(), dt);
      b->apply(10.0 * b->limit(), dt);
    }
    if (a->position() != b->position() || a->velocity() != b->velocity()) broken.push_back("saturation");
  }
  {
    std::unique_ptr<Plant> a = make();
    std::unique_ptr<Plant> b = make();
    for (int i = 0; i < 100; ++i) {
      a->apply(0.3 * a->limit(), dt);
      b->apply(-0.3 * b->limit(), dt);
    }
    if (std::fabs(a->position() + b->position()) > 1e-12) broken.push_back("symmetry");
  }
  {
    std::unique_ptr<Plant> a = make();
    std::unique_ptr<Plant> b = make();
    for (int i = 0; i < 100; ++i) {
      const double u = 0.7 * a->limit() * std::sin(0.1 * i);
      a->apply(u, dt);
      b->apply(u, dt);
    }
    if (a->position() != b->position() || a->velocity() != b->velocity()) broken.push_back("determinism");
  }
  {
    std::unique_ptr<Plant> p = make();
    bool consistent = true;
    for (int i = 0; i < 50; ++i) {
      const double before = p->position();
      p->apply(0.6 * p->limit(), dt);
      if (std::fabs((p->position() - before) - p->velocity() * dt) > 1e-9) consistent = false;
    }
    if (!consistent) broken.push_back("consistency");
  }
  return broken;
}

}  // namespace sim
}  // namespace rc

#endif  // RC_SIM_PLANT_HPP
