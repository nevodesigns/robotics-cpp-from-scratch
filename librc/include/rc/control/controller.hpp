// rc/control/controller.hpp
//
// A controller interface, from lesson 03-14, graduated.
//
// Three ways a class hierarchy goes wrong without a word from the compiler. A function that differs from the base's only by a missing const is a new
// function and replaces nothing: with a gain of 5 and an error of 2 it gave 10 when called on its own type and 0 through a reference to the base,
// where `override` makes the build fail. A virtual call in a base constructor runs the base's version (it was told "base" while building a Special that says
// "special"). And a parameter that is the base type, not a reference to it, copies only the base part: gain 3, error 2 gave 6 by reference and 0 by value.

#ifndef RC_CONTROL_CONTROLLER_HPP
#define RC_CONTROL_CONTROLLER_HPP

namespace rc {
namespace control {

// What every controller can do: turn an error into an output. The base version does nothing, so a
// controller nobody has written yet is a safe one.
class Controller {
 public:
  virtual ~Controller() = default;
  virtual double output(double /*error*/) const { return 0.0; }
  virtual const char* name() const { return "none"; }
};

// output = gain * error.
//
// `override` after each function is a request: "this must replace a function the base class has,
// exactly". If the signature is even slightly different, such as a missing const, the compiler refuses.
// Without the word, a slightly different signature is a new function that hides nothing and replaces
// nothing, and the base version runs.
class Proportional : public Controller {
 public:
  explicit Proportional(double gain) : gain_(gain) {}

  double output(double error) const override { return gain_ * error; }
  const char* name() const override { return "proportional"; }

 private:
  double gain_;
};

// Holds the output of another controller to plus or minus a limit.
//
// It keeps a reference to the controller it wraps, so that controller must outlive this one.
class Saturated : public Controller {
 public:
  Saturated(const Controller& inner, double limit) : inner_(inner), limit_(limit) {}

  double output(double error) const override {
    const double raw = inner_.output(error);
    if (raw > limit_) return limit_;
    if (raw < -limit_) return -limit_;
    return raw;
  }

  const char* name() const override { return "saturated"; }

 private:
  const Controller& inner_;
  double limit_;
};

// Two ways to hand a controller to a function. These are given.
inline double run_by_reference(const Controller& c, double error) { return c.output(error); }

// A parameter that is a Controller and not a reference to one is a new Controller, built by copying only the
// Controller part of what was passed.
inline double run_by_value(Controller c, double error) { return c.output(error); }

}  // namespace control
}  // namespace rc

#endif  // RC_CONTROL_CONTROLLER_HPP
