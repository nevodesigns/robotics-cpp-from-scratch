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
    // TODO: clamp the command to plus or minus limit_. Then, in this order: velocity_ += (gain_ * u - velocity_) / tau_ * dt; position_ += velocity_ * dt (the new velocity moves the position)
    (void)command;
    (void)dt;
    (void)gain_;
    (void)tau_;
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
  // TODO: dt = 0.01. Each rule gets new plants from make() and adds its name to the answer if it is broken:
  // reset: drive one at half the limit for 50 steps, reset it: position and velocity must be exactly 0.
  // rest: reset, then 20 steps of command 0: both stay within 1e-12 of 0.
  // saturation: two plants, 100 steps, one at the limit and one at ten times it: position and velocity exactly equal.
  // symmetry: two plants, 100 steps at +0.3 and -0.3 of the limit: the two positions add to within 1e-12 of 0.
  // determinism: two plants, 100 steps of 0.7 * limit * sin(0.1 * i) each: exactly the same position and velocity.
  // consistency: one plant, 50 steps at 0.6 of the limit: after each, the position moved by velocity * dt, to within 1e-9.
  (void)make;
  return std::vector<std::string>();
}

#endif  // LESSON_SOLUTION_HPP
