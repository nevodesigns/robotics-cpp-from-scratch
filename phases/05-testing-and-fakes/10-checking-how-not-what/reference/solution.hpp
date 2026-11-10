#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>

// What a motor driver lets a program do. The real one talks to hardware, and a test cannot.
class MotorPort {
 public:
  virtual ~MotorPort() = default;
  virtual void enable() = 0;
  virtual void set_speed(double speed) = 0;      // metres a second, forward positive
  virtual void disable() = 0;
};

// A fake that behaves like the motor: it has a state, and it has the rules the real driver has.
//
// The rules are what a test needs to be able to check. A speed set while the driver is disabled is ignored by the
// real hardware, and a speed past the limit is refused. The fake counts each of those as a violation, so a test can
// ask "did the program ever break a rule?" without caring which calls it made.
class SimPort : public MotorPort {
 public:
  explicit SimPort(double limit) : limit_(limit) {}

  void enable() override { enabled_ = true; }

  void set_speed(double speed) override {
    if (!enabled_) {
      ++violations_;              // the driver would have ignored it
      return;
    }
    if (std::fabs(speed) > limit_) {
      ++violations_;              // the driver would have refused it
      return;
    }
    speed_ = speed;
  }

  void disable() override {
    enabled_ = false;
    speed_ = 0.0;                 // a disabled motor is not turning
  }

  bool enabled() const { return enabled_; }
  double speed() const { return speed_; }
  int violations() const { return violations_; }

 private:
  double limit_;
  bool enabled_ = false;
  double speed_ = 0.0;
  int violations_ = 0;
};

#endif  // LESSON_SOLUTION_HPP
