#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "solution.hpp"

namespace {

// Plants that each break one rule, written as a careless person might, to see whether the contract notices.
class Unclamped : public Plant {
 public:
  void reset() override { x_ = v_ = 0.0; }
  void apply(double command, double dt) override {
    v_ += (2.0 * command - v_) / 0.1 * dt;              // no clamp
    x_ += v_ * dt;
  }
  double position() const override { return x_; }
  double velocity() const override { return v_; }
  double limit() const override { return 12.0; }
 private:
  double x_ = 0.0;
  double v_ = 0.0;
};

class Stale : public Plant {                              // reports where it was before the step
 public:
  void reset() override { x_ = v_ = shown_ = 0.0; }
  void apply(double command, double dt) override {
    const double u = command > 12.0 ? 12.0 : (command < -12.0 ? -12.0 : command);
    shown_ = x_;
    v_ += (2.0 * u - v_) / 0.1 * dt;
    x_ += v_ * dt;
  }
  double position() const override { return shown_; }
  double velocity() const override { return v_; }
  double limit() const override { return 12.0; }
 private:
  double x_ = 0.0;
  double v_ = 0.0;
  double shown_ = 0.0;
};

class Drifting : public Plant {                           // a different answer each time a new one is made
 public:
  Drifting() { bias_ = 1e-6 * ++made_; }
  void reset() override { x_ = v_ = 0.0; }
  void apply(double command, double dt) override {
    const double u = command > 12.0 ? 12.0 : (command < -12.0 ? -12.0 : command);
    v_ += (2.0 * u - v_) / 0.1 * dt;
    x_ += (v_ + bias_) * dt;
  }
  double position() const override { return x_; }
  double velocity() const override { return v_; }
  double limit() const override { return 12.0; }
 private:
  static int made_;
  double bias_ = 0.0;
  double x_ = 0.0;
  double v_ = 0.0;
};
int Drifting::made_ = 0;

class LeakyReset : public Plant {                         // forgets the speed
 public:
  void reset() override { x_ = 0.0; }
  void apply(double command, double dt) override {
    const double u = command > 12.0 ? 12.0 : (command < -12.0 ? -12.0 : command);
    v_ += (2.0 * u - v_) / 0.1 * dt;
    x_ += v_ * dt;
  }
  double position() const override { return x_; }
  double velocity() const override { return v_; }
  double limit() const override { return 12.0; }
 private:
  double x_ = 0.0;
  double v_ = 0.0;
};

class Lopsided : public Plant {                           // does more in one direction than the other
 public:
  void reset() override { x_ = v_ = 0.0; }
  void apply(double command, double dt) override {
    double u = command > 12.0 ? 12.0 : (command < -12.0 ? -12.0 : command);
    if (u < 0.0) u *= 0.9;
    v_ += (2.0 * u - v_) / 0.1 * dt;
    x_ += v_ * dt;
  }
  double position() const override { return x_; }
  double velocity() const override { return v_; }
  double limit() const override { return 12.0; }
 private:
  double x_ = 0.0;
  double v_ = 0.0;
};

std::string joined(const std::vector<std::string>& v) {
  std::string s;
  for (const std::string& x : v) s += (s.empty() ? "" : ", ") + x;
  return s.empty() ? "none" : s;
}

bool has(const std::vector<std::string>& v, const char* name) {
  for (const std::string& x : v)
    if (x == name) return true;
  return false;
}

}  // namespace

RC_TEST("a motor that follows its command with a lag reaches the speed gain times command, and does it in about one time constant for each 63 percent") {
  SimMotor m(2.0, 0.1, 12.0);
  const double u = 3.0;
  double at_tau = 0.0;
  double at_5tau = 0.0;
  for (int i = 1; i <= 500; ++i) {
    m.apply(u, 0.001);
    if (i == 100) at_tau = m.velocity();
    if (i == 500) at_5tau = m.velocity();
  }
  const double final_speed = 2.0 * u;
  std::cout << "  command 3 on a motor of gain 2 and time constant 0.1 s: speed " << std::fixed << std::setprecision(3) << at_tau << " after 0.1 s (" << std::setprecision(1)
            << 100.0 * at_tau / final_speed << " percent of " << final_speed << "), " << std::setprecision(3) << at_5tau << " after 0.5 s\n\n";
  RC_CHECK(at_tau > 0.60 * final_speed && at_tau < 0.66 * final_speed);
  RC_CHECK(at_5tau > 0.98 * final_speed && at_5tau < final_speed);
  RC_CHECK(m.position() > 0.0);
}

RC_TEST("a command beyond the limit does exactly what the limit does, in either direction") {
  SimMotor a(2.0, 0.1, 12.0);
  SimMotor b(2.0, 0.1, 12.0);
  SimMotor c(2.0, 0.1, 12.0);
  SimMotor d(2.0, 0.1, 12.0);
  for (int i = 0; i < 100; ++i) {
    a.apply(12.0, 0.01);
    b.apply(500.0, 0.01);
    c.apply(-12.0, 0.01);
    d.apply(-500.0, 0.01);
  }
  RC_CHECK(a.position() > 0.0);
  RC_CHECK(a.position() == b.position());
  RC_CHECK(c.position() == d.position());
  RC_CHECK(c.position() < 0.0);
  RC_CHECK_NEAR(a.position(), -c.position(), 1e-12);
  SimMotor degenerate(1.0, 0.0, -5.0);                  // a zero time constant and a negative limit become usable
  degenerate.apply(1.0, 0.01);
  RC_CHECK(std::isfinite(degenerate.position()));
  RC_CHECK(degenerate.limit() > 0.0);
}

RC_TEST("the simulated motor keeps every rule of the contract") {
  const std::vector<std::string> broken = check_contract([]() { return std::unique_ptr<Plant>(new SimMotor(2.0, 0.1, 12.0)); });
  std::cout << "  a SimMotor: rules broken: " << joined(broken) << "\n\n";
  RC_CHECK(broken.empty());
  const std::vector<std::string> slow = check_contract([]() { return std::unique_ptr<Plant>(new SimMotor(0.5, 2.0, 1.0)); });
  RC_CHECK(slow.empty());
  // And a contract that nothing can fail is no contract: it must find fault with a plant that has one.
  RC_CHECK(!check_contract([]() { return std::unique_ptr<Plant>(new Unclamped()); }).empty());
}

RC_TEST("a plant with no clamp, one that reports where it was, one that is not repeatable, one that forgets its speed and one that favours a direction are each caught by the rule they break") {
  const std::vector<std::string> unclamped = check_contract([]() { return std::unique_ptr<Plant>(new Unclamped()); });
  const std::vector<std::string> stale = check_contract([]() { return std::unique_ptr<Plant>(new Stale()); });
  const std::vector<std::string> drifting = check_contract([]() { return std::unique_ptr<Plant>(new Drifting()); });
  const std::vector<std::string> leaky = check_contract([]() { return std::unique_ptr<Plant>(new LeakyReset()); });
  const std::vector<std::string> lopsided = check_contract([]() { return std::unique_ptr<Plant>(new Lopsided()); });
  std::cout << "  rules broken by each careless plant\n  no clamp:         " << joined(unclamped) << "\n  reports the past: " << joined(stale) << "\n  not repeatable:   "
            << joined(drifting) << "\n  forgets its speed: " << joined(leaky) << "\n  favours a side:   " << joined(lopsided) << "\n\n";
  RC_CHECK(has(unclamped, "saturation"));
  RC_CHECK(has(stale, "consistency"));
  RC_CHECK(has(drifting, "determinism"));
  RC_CHECK(has(leaky, "reset"));
  RC_CHECK(has(lopsided, "symmetry"));
  RC_CHECK(!has(unclamped, "reset"));
  RC_CHECK(!has(leaky, "saturation"));
}

RC_TEST("a controller written against the interface drives either plant, and the plant with no clamp behaves differently where it matters") {
  // The same proportional controller, run on any Plant. It asks for a command of 40 at the start, more than the motor is allowed to act on.
  struct Result {
    double final_position = 0.0;
    double peak_speed = 0.0;
  };
  const auto run = [](Plant& p) {
    Result r;
    p.reset();
    for (int i = 0; i < 300; ++i) {
      const double command = 40.0 * (1.0 - p.position());       // go to position 1
      p.apply(command, 0.01);
      if (std::fabs(p.velocity()) > r.peak_speed) r.peak_speed = std::fabs(p.velocity());
    }
    r.final_position = p.position();
    return r;
  };
  SimMotor good(2.0, 0.1, 12.0);
  Unclamped bad;
  SimMotor again(2.0, 0.1, 12.0);
  const Result a = run(good);
  const Result b = run(bad);
  const Result c = run(again);
  std::cout << "  the same controller to position 1\n  plant                  ends at   peak speed\n  keeps the contract     " << std::fixed << std::setprecision(4) << a.final_position << "    " << std::setprecision(2)
            << a.peak_speed << "\n  no clamp               " << std::setprecision(4) << b.final_position << "    " << std::setprecision(2) << b.peak_speed << "\n  another of the first   "
            << std::setprecision(4) << c.final_position << "    " << std::setprecision(2) << c.peak_speed << "\n\n";
  RC_CHECK(a.final_position == c.final_position);
  RC_CHECK(a.peak_speed == c.peak_speed);
  RC_CHECK(std::fabs(a.final_position - 1.0) < 0.1);
  RC_CHECK(a.peak_speed <= 2.0 * 12.0 + 1e-9);                      // the motor cannot go faster than gain times limit
  RC_CHECK(b.peak_speed > a.peak_speed + 5.0);                      // the plant that does not clamp goes past what the real one can do
}
