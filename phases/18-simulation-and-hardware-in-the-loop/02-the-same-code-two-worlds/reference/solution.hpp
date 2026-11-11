#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <functional>
#include <memory>
#include <string>
#include <vector>

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

#endif  // LESSON_SOLUTION_HPP
