#include <rc/test/rc_test.hpp>

#include <iostream>
#include <string>
#include <vector>

#include "solution.hpp"

namespace {

// The same idea as Proportional, with the const left off and no override to say so.
//
// The compiler's own warning about this (it hides a virtual function) is switched off here, because
// the mistake is the point.
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#endif
class Forgetful : public Controller {
 public:
  explicit Forgetful(double gain) : gain_(gain) {}
  double output(double error) { return gain_ * error; }   // a new function, not a replacement

 private:
  double gain_;
};
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

// A base class whose constructor asks a virtual function who it is.
std::vector<std::string>& seen() {
  static std::vector<std::string> names;
  return names;
}

class Named {
 public:
  Named() { seen().push_back(who()); }       // which who()?
  virtual ~Named() = default;
  virtual std::string who() const { return "base"; }
};

class Special : public Named {
 public:
  std::string who() const override { return "special"; }
};

}  // namespace

RC_TEST("a proportional controller works through a reference to its base") {
  Proportional p(3.0);
  const Controller& base = p;
  RC_CHECK_NEAR(base.output(2.0), 6.0, 1e-12);
  RC_CHECK_NEAR(base.output(-1.0), -3.0, 1e-12);
  RC_CHECK_EQ(std::string(base.name()), std::string("proportional"));
  RC_CHECK_EQ(std::string(Controller().name()), std::string("none"));
}

RC_TEST("a saturated controller holds the output of the one it wraps") {
  Proportional p(10.0);
  Saturated s(p, 1.0);
  const Controller& base = s;
  RC_CHECK_NEAR(base.output(0.05), 0.5, 1e-12);
  RC_CHECK_NEAR(base.output(0.5), 1.0, 1e-12);
  RC_CHECK_NEAR(base.output(-0.5), -1.0, 1e-12);
  RC_CHECK_EQ(std::string(base.name()), std::string("saturated"));

  // Wrapping a wrapper works because both are controllers.
  Saturated tighter(s, 0.25);
  RC_CHECK_NEAR(tighter.output(0.5), 0.25, 1e-12);
}

RC_TEST("without override, a missing const makes a new function and the base version runs") {
  Forgetful f(5.0);
  const Controller& base = f;
  std::cout << "  gain 5, error 2, called on the Forgetful itself: " << f.output(2.0) << ", through a reference to the base: "
            << base.output(2.0) << "\n\n";
  RC_CHECK_NEAR(f.output(2.0), 10.0, 1e-12);       // it does what it says when you call it by its own type
  RC_CHECK_NEAR(base.output(2.0), 0.0, 1e-12);     // and nothing when anyone else holds it as a Controller

  // The same gain written with override works through the base.
  Proportional p(5.0);
  const Controller& as_base = p;
  RC_CHECK_NEAR(as_base.output(2.0), 10.0, 1e-12);
}

RC_TEST("a virtual call in a constructor runs the version of the class being built, not the derived one") {
  seen().clear();
  Special s;
  std::cout << "  made a Special: the base constructor asked who() and was told \"" << seen()[0]
            << "\"; asked afterwards it says \"" << s.who() << "\"\n\n";
  RC_REQUIRE(seen().size() == 1);
  RC_CHECK_EQ(seen()[0], std::string("base"));
  RC_CHECK_EQ(s.who(), std::string("special"));

  // The same applies to a controller's name: a plain Saturated is itself once it is built.
  Proportional p(1.0);
  Saturated sat(p, 1.0);
  RC_CHECK_EQ(std::string(sat.name()), std::string("saturated"));
}

RC_TEST("passing a controller by value copies only the base part, and by reference does not") {
  Proportional p(3.0);
  const double by_reference = run_by_reference(p, 2.0);
  const double by_value = run_by_value(p, 2.0);
  std::cout << "  gain 3, error 2: by reference " << by_reference << ", by value " << by_value << "\n\n";
  RC_CHECK_NEAR(by_reference, 6.0, 1e-12);
  RC_CHECK_NEAR(by_value, 0.0, 1e-12);
}
