#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <type_traits>

#include "solution.hpp"

namespace {

// A function that wants a distance, so the tests have something to hand one to.
double stopping_distance(Metres speed_times_time) { return speed_times_time.value(); }

// At file scope, because MSVC rejects a local constexpr used inside a lambda,
// which is E-CPP-0023 and rule L022.
constexpr Metres kWheelbase = metres(0.35);
static_assert(kWheelbase.value() > 0.3, "a constant quantity is still constant");

}  // namespace

RC_TEST("a quantity is one double, and costs nothing to be one") {
  // The whole mechanism is a type, so there is nothing of it left at runtime.
  RC_CHECK_EQ(sizeof(Metres), sizeof(double));
  RC_CHECK_EQ(alignof(Metres), alignof(double));
  RC_CHECK(std::is_trivially_copyable<Metres>::value);

  // And it is usable at compile time, so a constant stays a constant. The
  // static_assert sits beside the constant at file scope.
  RC_CHECK_NEAR(kWheelbase.value(), 0.35, 1e-12);
}

RC_TEST("the arithmetic that makes sense, and what it produces") {
  const Metres first = metres(2.0);
  const Metres second = metres(0.5);

  RC_CHECK_NEAR((first + second).value(), 2.5, 1e-12);
  RC_CHECK_NEAR((first - second).value(), 1.5, 1e-12);
  RC_CHECK_NEAR((-first).value(), -2.0, 1e-12);

  // Scaling by a plain number keeps the unit, because a number has none.
  RC_CHECK_NEAR((first * 3.0).value(), 6.0, 1e-12);
  RC_CHECK_NEAR((3.0 * first).value(), 6.0, 1e-12);
  RC_CHECK_NEAR((first / 4.0).value(), 0.5, 1e-12);

  // Dividing two of the same thing gives a plain number, which is a ratio, and
  // that is a different return type from every other operation here.
  const double ratio = first / second;
  RC_CHECK_NEAR(ratio, 4.0, 1e-12);
  static_assert(std::is_same<decltype(first / second), double>::value,
                "a length over a length is a number");
  static_assert(std::is_same<decltype(first / 2.0), Metres>::value,
                "a length over a number is a length");

  // Comparison, and accumulation.
  RC_CHECK(second < first);
  RC_CHECK(first > second);
  RC_CHECK(first != second);
  RC_CHECK(metres(1.0) == metres(1.0));

  Metres total = metres(0.0);
  for (int i = 0; i < 4; ++i) total += metres(0.25);
  RC_CHECK_NEAR(total.value(), 1.0, 1e-12);

  RC_CHECK_NEAR(abs(metres(-3.0)).value(), 3.0, 1e-12);
}

RC_TEST("the conversion lives in one place instead of everywhere") {
  std::cout << "\n    the same distance, said four ways\n\n";
  std::cout << "    " << std::left << std::setw(28) << "millimetres(1500)"
            << std::right << std::fixed << std::setprecision(4)
            << millimetres(1500.0).value() << " m\n";
  std::cout << "    " << std::left << std::setw(28) << "metres(1.5)" << std::right
            << metres(1.5).value() << " m\n";
  std::cout << "    " << std::left << std::setw(28) << "as_millimetres(metres(1.5))"
            << std::right << std::setprecision(1) << as_millimetres(metres(1.5))
            << " mm\n";

  // The two ways of saying it are the same value, and the compiler will let you
  // add them because they are the same thing.
  RC_CHECK(millimetres(1500.0) == metres(1.5));
  RC_CHECK_NEAR((millimetres(500.0) + metres(1.0)).value(), 1.5, 1e-12);

  // A factor of a thousand exists in exactly two lines of the program, rather
  // than wherever somebody remembered it.
  RC_CHECK_NEAR(as_millimetres(millimetres(37.0)), 37.0, 1e-9);
  RC_CHECK_NEAR(as_milliseconds(milliseconds(250.0)), 250.0, 1e-9);

  // Degrees the same way, which is where lesson 01-03's angle mistakes came
  // from: sin and cos want radians and nothing about a double says which it is.
  RC_CHECK_NEAR(degrees(180.0).value(), 3.14159265358979, 1e-12);
  RC_CHECK_NEAR(as_degrees(radians(3.14159265358979323846 / 2.0)), 90.0, 1e-9);
  RC_CHECK_NEAR(as_degrees(degrees(37.5)), 37.5, 1e-9);

  std::cout << "\n    a factor of a thousand appears in two lines of the whole\n";
  std::cout << "    program, instead of wherever somebody remembered it\n";
}

RC_TEST("the conversion is explicit at the edge, and only there") {
  // Making one takes a named function, so the unit is written down.
  const Metres from_a_sensor = millimetres(1234.0);
  RC_CHECK_NEAR(from_a_sensor.value(), 1.234, 1e-12);

  // Getting the number back out also takes asking, which is the other edge.
  RC_CHECK_NEAR(stopping_distance(from_a_sensor), 1.234, 1e-12);
  RC_CHECK_NEAR(from_a_sensor.value(), 1.234, 1e-12);

  // And an explicit conversion is still available where it is genuinely meant,
  // because refusing that would make the type unusable at a boundary somebody
  // does not control.
  const Metres deliberate{2.5};
  RC_CHECK_NEAR(deliberate.value(), 2.5, 1e-12);

  // What is refused is the accident. Three snippets in checks/ prove it: adding
  // a length to a duration, passing a bare double where a length was wanted,
  // and multiplying two lengths. Each is compiled by the build and expected to
  // fail, because a guarantee that only exists in a comment is not a guarantee.
  std::cout << "\n    three snippets in checks/ are compiled and expected to\n";
  std::cout << "    fail. A test can only assert what runs, so a promise the\n";
  std::cout << "    compiler makes has to be checked by the compiler\n";
}

RC_TEST("what it does not catch") {
  // Two different lengths are the same type, so this is arithmetic the compiler
  // has no opinion about, and it is nonsense.
  const Metres height = metres(1.8);
  const Metres wavelength = metres(0.0000005);
  const Metres meaningless = height + wavelength;
  RC_CHECK(meaningless > height);

  // A unit system checks that quantities are of the same kind. It does not
  // check that they are of the same thing, that a frame matches, that a sign
  // convention matches, or that a number is sensible.
  const Radians turn = degrees(370.0);
  RC_CHECK(turn.value() > 6.28);   // nothing here wraps it, and nothing should

  std::cout << "\n    a unit is not a meaning. Both of these are lengths in\n";
  std::cout << "    metres and adding them is still nonsense, and an angle of\n";
  std::cout << "    370 degrees is a perfectly good Radians. The type removes\n";
  std::cout << "    one class of mistake completely and leaves the rest\n";
}
