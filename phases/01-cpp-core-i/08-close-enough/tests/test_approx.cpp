#include <rc/test/rc_test.hpp>

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <vector>

#include "solution.hpp"

namespace {

// Written at file scope because MSVC refuses a local constexpr used inside a
// lambda, which this curriculum has now tripped over three times.
constexpr double kTolerance = 1e-9;

const double kMagnitudes[] = {1.0, 1e3, 1e6, 1e9, 1e12};

double next_above(double value) {
  return std::nextafter(value, std::numeric_limits<double>::infinity());
}

}  // namespace

RC_TEST("the classic, to its last digit") {
  // Through a vector so that nothing here is folded at compile time.
  const std::vector<double> parts = {0.1, 0.2};
  const double sum = parts[0] + parts[1];

  std::cout << "\n  0.1 + 0.2   " << std::fixed << std::setprecision(20) << sum << "\n";
  std::cout << "  0.3         " << 0.3 << "\n";
  std::cout << "  difference  " << std::scientific << std::setprecision(3) << (sum - 0.3)
            << "\n\n";

  RC_CHECK(sum != 0.3);
  RC_CHECK(nearly_equal(sum, 0.3, 1e-15, 0.0));

  // And it is one representable double away, not some unknowable distance.
  RC_CHECK(ulps_between(sum, 0.3) < 2.0);
}

RC_TEST("doubles are not spread evenly, and near the top adding one does nothing") {
  std::cout << "\n  the gap to the next representable double\n\n";
  std::cout << "  " << std::setw(12) << "magnitude" << std::setw(16) << "gap to next"
            << std::setw(22) << "does v + 1.0 differ" << "\n";

  const double big[] = {1.0, 1e3, 1e6, 1e9, 1e12, 1e15, 1e16, 1e17};
  int unchanged = 0;
  for (double value : big) {
    // volatile so the comparison is made on stored doubles rather than folded.
    volatile double before = value;
    volatile double after = value + 1.0;
    const bool differs = (before != after);
    if (!differs) ++unchanged;

    std::cout << "  " << std::scientific << std::setprecision(0) << std::setw(12) << value
              << std::setprecision(3) << std::setw(16) << gap_above(value) << std::setw(22)
              << (differs ? "yes" : "NO") << "\n";
  }
  std::cout << "\n  " << unchanged << " of 8 magnitudes could not notice a whole unit\n\n";

  // The gap grows with magnitude, always.
  for (std::size_t i = 1; i < sizeof(big) / sizeof(big[0]); ++i)
    RC_CHECK(gap_above(big[i]) > gap_above(big[i - 1]));

  // At 1e16 the gap is 2.0, so a unit cannot be represented there at all.
  RC_CHECK_NEAR(gap_above(1e16), 2.0, 1e-12);
  RC_CHECK(unchanged >= 2);
}

RC_TEST("one absolute tolerance means two different things") {
  std::cout << "\n  what an absolute tolerance of 1e-9 buys, by magnitude\n\n";
  std::cout << "  " << std::setw(12) << "magnitude" << std::setw(15) << "gap to next"
            << std::setw(19) << "doubles it spans" << std::setw(26)
            << "adjacent pair equal?" << "\n";

  int unsatisfiable = 0;
  for (double value : kMagnitudes) {
    const double neighbour = next_above(value);
    const bool equal = nearly_equal(value, neighbour, 0.0, kTolerance);
    if (!equal) ++unsatisfiable;

    std::cout << "  " << std::scientific << std::setprecision(0) << std::setw(12) << value
              << std::setprecision(3) << std::setw(15) << gap_above(value) << std::setw(19)
              << (kTolerance / gap_above(value)) << std::setw(26)
              << (equal ? "yes" : "NO, nothing can") << "\n";
  }
  std::cout << "\n  on " << unsatisfiable << " of 5 magnitudes no two distinct doubles"
            << " are within 1e-9 of each other\n\n";

  // At magnitude 1 it spans millions of distinct values, which is loose rather
  // than strict.
  RC_CHECK(kTolerance / gap_above(1.0) > 1e6);

  // At 1e9 and above it cannot even reach the next representable double, so no
  // two different numbers can ever satisfy it. A tolerance like that is not
  // strict, it is impossible.
  RC_CHECK(kTolerance / gap_above(1e9) < 1.0);
  RC_CHECK(!nearly_equal(1e9, next_above(1e9), 0.0, kTolerance));
  RC_CHECK(unsatisfiable >= 2);
}

RC_TEST("a relative tolerance means the same thing everywhere") {
  std::cout << "\n  the same 1e-9, read relatively\n\n";
  std::cout << "  " << std::setw(12) << "magnitude" << std::setw(19) << "doubles it spans"
            << std::setw(22) << "adjacent pair equal?" << "\n";

  double smallest_span = std::numeric_limits<double>::infinity();
  double largest_span = 0.0;
  for (double value : kMagnitudes) {
    const double span = (kTolerance * value) / gap_above(value);
    smallest_span = std::min(smallest_span, span);
    largest_span = std::max(largest_span, span);

    const bool equal = nearly_equal(value, next_above(value), kTolerance, 1e-12);
    std::cout << "  " << std::scientific << std::setprecision(0) << std::setw(12) << value
              << std::setprecision(3) << std::setw(19) << span << std::setw(22)
              << (equal ? "yes" : "NO") << "\n";
    RC_CHECK(equal);
  }

  std::cout << "\n  spans " << std::setprecision(2) << smallest_span << " to "
            << largest_span << " doubles across twelve orders of magnitude\n\n";

  // Within a small factor at every magnitude, which is the whole property an
  // absolute tolerance does not have.
  RC_CHECK(largest_span / smallest_span < 4.0);
}

RC_TEST("and near zero the relative tolerance is the one that fails") {
  // No size to be relative to, so two values a factor of two apart are as far
  // apart as any two numbers can be.
  RC_CHECK(!nearly_equal(1e-18, 2e-18, 1e-9, 0.0));
  RC_CHECK(!nearly_equal(0.0, 1e-300, 1e-9, 0.0));

  // The absolute floor exists for exactly this and nothing else.
  RC_CHECK(nearly_equal(1e-18, 2e-18, 1e-9, 1e-12));
  RC_CHECK(nearly_equal(0.0, 1e-300, 1e-9, 1e-12));

  // Both together, which is why the function takes both.
  RC_CHECK(nearly_equal(1e9, next_above(1e9), 1e-9, 1e-12));
  RC_CHECK(!nearly_equal(1.0, 1.5, 1e-9, 1e-12));

  std::cout << "\n  relative alone cannot reach zero, absolute alone cannot reach 1e9,"
            << " so the test is either\n\n";
}

RC_TEST("a time that is accumulated never equals the time you were waiting for") {
  const double step = 0.1;
  const int kCap = 1000;

  int exact_steps = 0;
  double exact_clock = 0.0;
  while (exact_clock != 10.0 && exact_steps < kCap) {
    exact_clock += step;
    ++exact_steps;
  }

  int bounded_steps = 0;
  double bounded_clock = 0.0;
  while (bounded_clock < 10.0 && bounded_steps < kCap) {
    bounded_clock += step;
    ++bounded_steps;
  }

  std::cout << "\n  stepping a clock by 0.1 until it reaches 10.0\n\n";
  std::cout << "  waiting for == 10.0   " << std::setw(14)
            << (exact_steps >= kCap ? "never arrived" : "arrived") << "  after "
            << exact_steps << " steps\n";
  std::cout << "  waiting for >= 10.0   " << std::setw(14) << "arrived"
            << "  after " << bounded_steps << " steps\n\n";

  // The equality never fires, at all, ever.
  RC_CHECK(exact_steps == kCap);

  // The comparison that does fire takes one step more than arithmetic says,
  // because a hundred additions of 0.1 land just under 10.
  RC_CHECK(bounded_steps == 101);

  double hundred = 0.0;
  for (int i = 0; i < 100; ++i) hundred += step;
  RC_CHECK(hundred < 10.0);
  std::cout << "  a hundred additions of 0.1 reach " << std::setprecision(17)
            << std::fixed << hundred << ", which is under ten\n\n";
  std::cout << std::scientific << std::setprecision(3);

  // Multiplying instead of accumulating is exact here, and is one fix.
  RC_CHECK(100 * step == 10.0);

  // The other fix is to ask the question that was meant. The accumulated
  // clock is two representable doubles away from ten, which is the same as
  // equal for any purpose a robot has.
  std::cout << "  it is " << std::setprecision(1) << ulps_between(hundred, 10.0)
            << " representable doubles short, and nearly_equal says "
            << (nearly_equal(hundred, 10.0, 1e-12, 1e-15) ? "equal" : "not equal")
            << "\n\n";
  std::cout << std::setprecision(3);
  RC_CHECK(ulps_between(hundred, 10.0) < 100.0);
  RC_CHECK(nearly_equal(hundred, 10.0, 1e-12, 1e-15));
}

RC_TEST("drift grows with the number of steps") {
  std::cout << "\n  " << std::setw(10) << "steps" << std::setw(16) << "error"
            << std::setw(18) << "doubles adrift" << "\n";

  const int counts[] = {10, 100, 1000, 10000, 100000, 1000000};
  std::vector<double> errors;
  std::vector<double> ulps;
  for (int count : counts) {
    double accumulated = 0.0;
    for (int i = 0; i < count; ++i) accumulated += 0.1;
    const double exact = count * 0.1;
    const double error = std::fabs(accumulated - exact);
    errors.push_back(error);
    ulps.push_back(ulps_between(accumulated, exact));
    std::cout << "  " << std::setw(10) << count << std::setw(16) << error
              << std::setw(18) << std::setprecision(1) << ulps.back()
              << std::setprecision(3) << "\n";
  }
  std::cout << "\n";

  // Not a fixed cost. The longer the robot runs, the further out it is.
  RC_CHECK(errors.back() > errors.front());
  RC_CHECK(errors.back() > 1e-7);

  // Counted in representable doubles rather than in metres, which is the
  // measure that does not change meaning as the total grows.
  RC_CHECK(ulps.back() > ulps.front());
  RC_CHECK(ulps.front() < 2.0);
}

RC_TEST("adding one a hundred thousand times to a large total changes nothing") {
  std::vector<double> values;
  values.push_back(1e16);
  for (int i = 0; i < 100000; ++i) values.push_back(1.0);
  const double exact = 1e16 + 100000.0;

  double forwards = 0.0;
  for (double value : values) forwards += value;

  std::vector<double> smallest_first = values;
  std::sort(smallest_first.begin(), smallest_first.end());
  double ascending = 0.0;
  for (double value : smallest_first) ascending += value;

  const double compensated = kahan_sum(rc::span<const double>(values.data(), values.size()));

  std::cout << "\n  one value of 1e16 and a hundred thousand of 1.0\n\n";
  std::cout << "  " << std::left << std::setw(26) << "order" << std::right << std::setw(22)
            << "sum" << std::setw(14) << "error" << "\n";
  const auto row = [&](const char* name, double total) {
    std::cout << "  " << std::left << std::setw(26) << name << std::right << std::fixed
              << std::setprecision(1) << std::setw(22) << total << std::setw(14)
              << (total - exact) << "\n";
  };
  row("largest first", forwards);
  row("smallest first", ascending);
  row("compensated, largest first", compensated);
  row("exact", exact);
  std::cout << "\n";

  // Every one of the hundred thousand additions was discarded, because 1.0 is
  // below the gap between doubles at 1e16.
  RC_CHECK(forwards == 1e16);
  RC_CHECK_NEAR(forwards - exact, -100000.0, 1e-6);

  // Both repairs land exactly, by different routes: one changes the order so
  // the small values accumulate together first, the other keeps what did not
  // fit and feeds it back.
  RC_CHECK(ascending == exact);
  RC_CHECK(compensated == exact);

  // And on an ordinary list it agrees with plain addition.
  const std::vector<double> ordinary = {1.5, 2.25, 3.125, 4.0};
  RC_CHECK_NEAR(kahan_sum(rc::span<const double>(ordinary.data(), ordinary.size())), 10.875,
                1e-12);
  RC_CHECK_NEAR(kahan_sum(rc::span<const double>()), 0.0, 1e-12);
}
