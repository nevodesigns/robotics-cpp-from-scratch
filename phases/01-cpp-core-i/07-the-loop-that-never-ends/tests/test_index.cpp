#include <rc/test/rc_test.hpp>

#include <cstddef>
#include <iomanip>
#include <iostream>
#include <limits>
#include <vector>

#include "solution.hpp"

namespace {

// Ten iterations is far more than a three element vector needs. A loop still
// running at ten is a loop that is not going to stop.
const int kCap = 10;

std::vector<double> three_readings() { return {1.5, 2.5, 3.5}; }

}  // namespace

RC_TEST("subtracting one from an empty size does not give minus one") {
  const std::vector<double> nothing;

  const std::size_t wrapped = nothing.size() - 1;
  const std::size_t biggest = std::numeric_limits<std::size_t>::max();

  std::cout << "\n  an empty vector\n\n";
  std::cout << "    size()                    " << std::setw(24) << nothing.size() << "\n";
  std::cout << "    size() - 1                " << std::setw(24) << wrapped << "\n";
  std::cout << "    last_index(size())        " << std::setw(24)
            << last_index(nothing.size()) << "\n\n";

  // Not a large number by accident. Exactly the largest there is, because
  // unsigned arithmetic wraps rather than going negative.
  RC_CHECK(wrapped == biggest);
  RC_CHECK(last_index(nothing.size()) == -1);

  // And the same expression on a container that does have something in it is
  // perfectly fine, which is why this survives testing.
  const std::vector<double> readings = three_readings();
  RC_CHECK(readings.size() - 1 == 2u);
  RC_CHECK(last_index(readings.size()) == 2);
}

RC_TEST("counting down with an unsigned index does not stop") {
  const std::vector<double> readings = three_readings();
  const std::vector<double> nothing;

  // A runtime zero. The comparison below is always true and the compiler
  // cannot see that here, which is the position a reader is in as well.
  const std::size_t floor_value = nothing.size();

  int steps = 0;
  for (std::size_t index = readings.size() - 1; index >= floor_value; --index) {
    ++steps;
    if (steps >= kCap) break;
  }

  std::cout << "  counting down over three readings\n\n";
  std::cout << "    unsigned index, capped at " << kCap << ":  " << steps << " steps\n";

  // Three readings, and it is still going after ten passes.
  RC_CHECK(steps == kCap);

  // The reason, on its own. Nothing is ever less than zero, so the condition
  // cannot become false, and the counter wraps at the bottom instead.
  std::size_t at_zero = nothing.size();
  RC_CHECK(at_zero == 0u);
  --at_zero;
  RC_CHECK(at_zero == std::numeric_limits<std::size_t>::max());

  // The same loop with a signed index, which is the entire fix.
  int signed_steps = 0;
  for (std::ptrdiff_t index = last_index(readings.size()); index >= 0; --index)
    ++signed_steps;

  int empty_steps = 0;
  for (std::ptrdiff_t index = last_index(nothing.size()); index >= 0; --index)
    ++empty_steps;

  std::cout << "    signed index:              " << signed_steps << " steps\n";
  std::cout << "    signed index, empty:       " << empty_steps << " steps\n\n";

  RC_CHECK(signed_steps == 3);
  RC_CHECK(empty_steps == 0);
}

RC_TEST("a negative number compared against a size is not negative any more") {
  const std::vector<double> readings = three_readings();

  const int missing = -1;

  // This is the conversion the language performs: the signed value goes to
  // unsigned, and -1 becomes the largest size there is. Written out, because
  // the compiler refuses the comparison itself, and rightly.
  const std::size_t converted = static_cast<std::size_t>(missing);

  std::cout << "  a sentinel of -1, compared against a size\n\n";
  std::cout << "    (std::size_t)(-1)         " << std::setw(24) << converted << "\n";
  std::cout << "    is it less than 3         " << std::setw(24)
            << ((converted < readings.size()) ? "true" : "false") << "\n";
  std::cout << "    as a signed comparison    " << std::setw(24)
            << ((missing < ssize(readings.size())) ? "true" : "false") << "\n\n";

  RC_CHECK(converted == std::numeric_limits<std::size_t>::max());

  // The answer flips. Read as numbers, -1 is less than 3. Read the way the
  // language actually evaluates it, it is not.
  RC_CHECK(!(converted < readings.size()));
  RC_CHECK(missing < ssize(readings.size()));

  // Which is what ssize is for: it makes both sides signed, so the comparison
  // means what it says.
  RC_CHECK(ssize(readings.size()) == 3);
  RC_CHECK(ssize(std::size_t{0}) == 0);
}

RC_TEST("an index is checked at both ends, because both ends go wrong") {
  const std::vector<double> readings = three_readings();
  const std::vector<double> nothing;

  RC_CHECK(is_valid_index(0, readings.size()));
  RC_CHECK(is_valid_index(2, readings.size()));

  // One past the end, the classic.
  RC_CHECK(!is_valid_index(3, readings.size()));

  // And negative, which is what last_index hands back for an empty container.
  RC_CHECK(!is_valid_index(-1, readings.size()));

  // Nothing at all is a valid index into an empty container.
  RC_CHECK(!is_valid_index(0, nothing.size()));
  RC_CHECK(!is_valid_index(last_index(nothing.size()), nothing.size()));

  // The two functions agree with each other, which is the property that makes
  // a loop written with both of them correct.
  for (std::ptrdiff_t index = last_index(readings.size()); index >= 0; --index)
    RC_CHECK(is_valid_index(index, readings.size()));
}

RC_TEST("a size that does not survive being put in an int") {
  const std::vector<double> readings = three_readings();

  RC_CHECK(fits_in_int(readings.size()));
  RC_CHECK(fits_in_int(0));
  RC_CHECK(fits_in_int(static_cast<std::size_t>(std::numeric_limits<int>::max())));

  const std::size_t one_too_many =
      static_cast<std::size_t>(std::numeric_limits<int>::max()) + 1;
  RC_CHECK(!fits_in_int(one_too_many));
  RC_CHECK(!fits_in_int(std::numeric_limits<std::size_t>::max()));

  std::cout << "  the largest size that fits in an int: "
            << static_cast<std::size_t>(std::numeric_limits<int>::max()) << "\n";
  std::cout << "  a point cloud with one more reading than that does not\n\n";
}

RC_TEST("walking a real vector, forwards and backwards, and off neither end") {
  const std::vector<double> readings = three_readings();

  double forwards = 0.0;
  for (std::ptrdiff_t index = 0; index < ssize(readings.size()); ++index) {
    RC_REQUIRE(is_valid_index(index, readings.size()));
    forwards += readings[static_cast<std::size_t>(index)];
  }

  double backwards = 0.0;
  for (std::ptrdiff_t index = last_index(readings.size()); index >= 0; --index) {
    RC_REQUIRE(is_valid_index(index, readings.size()));
    backwards += readings[static_cast<std::size_t>(index)];
  }

  RC_CHECK_NEAR(forwards, 7.5, 1e-12);
  RC_CHECK_NEAR(backwards, 7.5, 1e-12);

  // And the same two loops over nothing at all, which is where the unsigned
  // versions read memory that is not theirs.
  const std::vector<double> nothing;
  int touched = 0;
  for (std::ptrdiff_t index = 0; index < ssize(nothing.size()); ++index) ++touched;
  for (std::ptrdiff_t index = last_index(nothing.size()); index >= 0; --index) ++touched;
  RC_CHECK(touched == 0);
}
