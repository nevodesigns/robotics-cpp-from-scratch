#include <rc/test/rc_test.hpp>

#include <cmath>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <limits>
#include <vector>

#include "solution.hpp"

// File scope, per audit rule L022.
const int kIntMax = std::numeric_limits<int>::max();
const int kIntMin = std::numeric_limits<int>::min();
const long long kMicrosPerSecond = 1000000LL;

namespace {

// The rounding people write when they have not met the problem: add a half and
// let the cast throw the rest away.
int plus_half_trick(double value) { return static_cast<int>(value + 0.5); }

}  // namespace

RC_TEST("a cast throws the fraction away toward zero, which is not rounding and not flooring") {
  struct Row {
    double value;
    int cast;
    int floor;
    int round;
  };
  const std::vector<Row> rows = {
      {2.9, 2, 2, 3},   {-2.9, -2, -3, -3}, {0.5, 0, 0, 1},
      {-0.5, 0, -1, -1}, {2.5, 2, 2, 3},    {-2.5, -2, -3, -3},
  };

  std::cout << "   value   static_cast   +0.5 trick   floor_to_int   round_to_int\n";
  for (const Row& row : rows) {
    const IntResult floored = floor_to_int(row.value);
    const IntResult rounded = round_to_int(row.value);
    std::cout << std::setw(8) << row.value << std::setw(13) << static_cast<int>(row.value)
              << std::setw(13) << plus_half_trick(row.value) << std::setw(15) << floored.value
              << std::setw(15) << rounded.value << "\n";

    // What the language does, so the table is not taken on trust.
    RC_CHECK_EQ(static_cast<int>(row.value), row.cast);

    // What the artifact must do. Known answers, written by hand.
    RC_REQUIRE(floored.ok);
    RC_REQUIRE(rounded.ok);
    RC_CHECK_EQ(floored.value, row.floor);
    RC_CHECK_EQ(rounded.value, row.round);
  }
  std::cout << "\n";
}

RC_TEST("adding a half before casting is right for positives and wrong for negatives") {
  int differs = 0;
  int differs_on_positive = 0;
  int total = 0;
  int matches_lround = 0;

  for (int i = -300; i <= 300; ++i) {
    const double value = i / 100.0;
    ++total;

    const IntResult rounded = round_to_int(value);
    RC_REQUIRE(rounded.ok);
    if (rounded.value == static_cast<int>(std::lround(value))) ++matches_lround;

    if (plus_half_trick(value) != rounded.value) {
      ++differs;
      if (value > 0.0) ++differs_on_positive;
    }
  }

  std::cout << "  " << total << " values from -3.00 to 3.00 in steps of 0.01:\n";
  std::cout << "    the +0.5 trick disagrees with round_to_int   " << differs << "\n";
  std::cout << "    of those, on a positive value                " << differs_on_positive << "\n\n";

  RC_CHECK_EQ(matches_lround, total);
  RC_CHECK_EQ(differs_on_positive, 0);
  RC_CHECK(differs > 0);
}

RC_TEST("truncating puts both sides of zero in the same cell") {
  // A map of one metre cells, and twenty positions from -1.0 to 0.9.
  int cast_in_zero = 0;
  int cast_in_minus_one = 0;
  int floor_in_zero = 0;
  int floor_in_minus_one = 0;

  for (int i = -10; i < 10; ++i) {
    const double position = i / 10.0;
    const int by_cast = static_cast<int>(position);
    if (by_cast == 0) ++cast_in_zero;
    if (by_cast == -1) ++cast_in_minus_one;

    const IntResult by_floor = floor_to_int(position);
    RC_REQUIRE(by_floor.ok);
    if (by_floor.value == 0) ++floor_in_zero;
    if (by_floor.value == -1) ++floor_in_minus_one;
  }

  std::cout << "  twenty positions from -1.0 to 0.9 on a map of one metre cells:\n";
  std::cout << "                  in cell 0   in cell -1\n";
  std::cout << "    static_cast   " << std::setw(9) << cast_in_zero << "   " << std::setw(10)
            << cast_in_minus_one << "\n";
  std::cout << "    floor_to_int  " << std::setw(9) << floor_in_zero << "   " << std::setw(10)
            << floor_in_minus_one << "\n\n";

  RC_CHECK_EQ(cast_in_zero, 19);
  RC_CHECK_EQ(cast_in_minus_one, 1);
  RC_CHECK_EQ(floor_in_zero, 10);
  RC_CHECK_EQ(floor_in_minus_one, 10);
}

RC_TEST("a double that does not fit an int is refused, whatever the cast would have said") {
  // Known answers on both sides of both edges. The raw cast of the refused
  // ones is undefined, so this suite never runs it: the lesson reports what it
  // gave when run once, separately, and says so.
  const double nan = std::numeric_limits<double>::quiet_NaN();
  const double inf = std::numeric_limits<double>::infinity();

  const IntResult top = round_to_int(2147483647.0);
  const IntResult bottom = round_to_int(-2147483648.0);
  RC_REQUIRE(top.ok);
  RC_REQUIRE(bottom.ok);
  RC_CHECK_EQ(top.value, kIntMax);
  RC_CHECK_EQ(bottom.value, kIntMin);

  RC_CHECK(!round_to_int(2147483648.0).ok);
  RC_CHECK(!round_to_int(-2147483649.0).ok);
  RC_CHECK(!round_to_int(1e10).ok);
  RC_CHECK(!round_to_int(-1e10).ok);
  RC_CHECK(!round_to_int(nan).ok);
  RC_CHECK(!round_to_int(inf).ok);
  RC_CHECK(!round_to_int(-inf).ok);

  RC_CHECK(!floor_to_int(2147483648.0).ok);
  RC_CHECK(!floor_to_int(nan).ok);
  RC_CHECK(!floor_to_int(inf).ok);

  // The edge is different for the two. 2147483647.5 floors to the largest int
  // and rounds to one past it.
  const IntResult floored_edge = floor_to_int(2147483647.5);
  RC_REQUIRE(floored_edge.ok);
  RC_CHECK_EQ(floored_edge.value, kIntMax);
  RC_CHECK(!round_to_int(2147483647.5).ok);

  std::cout << "  on this machine the raw cast of 1e10, -1e10, nan, inf and 2147483648.0\n";
  std::cout << "  all gave -2147483648 when run once, outside this suite, at -O0 and -O2\n";
  std::cout << "  on gcc and clang: the same answer for a number and for not a number.\n\n";
}

RC_TEST("a 64 bit microsecond count in a 32 bit int wraps after 36 minutes") {
  std::cout << "   seconds   microseconds   static_cast<int>   to_int\n";

  int first_refused = 0;
  for (int seconds = 1; seconds <= 4000; ++seconds) {
    const IntResult result = to_int(seconds * kMicrosPerSecond);
    if (!result.ok && first_refused == 0) first_refused = seconds;
  }

  for (const int seconds : {60, 2000, 2147, 2148, 3000}) {
    const long long micros = seconds * kMicrosPerSecond;
    const IntResult result = to_int(micros);
    std::cout << std::setw(9) << seconds << std::setw(15) << micros << std::setw(19)
              << static_cast<int>(micros) << std::setw(9) << (result.ok ? "ok" : "refused") << "\n";
  }
  std::cout << "\n  the first second to_int refuses: " << first_refused << " (about "
            << first_refused / 60.0 << " minutes)\n\n";

  RC_CHECK_EQ(first_refused, 2148);

  // Converting down is implementation defined rather than undefined, and the
  // four toolchains this curriculum claims all wrap, which is why the answer
  // is a believable negative number and not a crash.
  RC_CHECK_EQ(static_cast<int>(2148 * kMicrosPerSecond), -2146967296);
  RC_CHECK_EQ(static_cast<int>(3000 * kMicrosPerSecond), -1294967296);

  const IntResult fits = to_int(2147000000LL);
  RC_REQUIRE(fits.ok);
  RC_CHECK_EQ(fits.value, 2147000000);
  const IntResult edge = to_int(2147483647LL);
  RC_REQUIRE(edge.ok);
  RC_CHECK_EQ(edge.value, kIntMax);
  RC_CHECK(!to_int(2147483648LL).ok);
  RC_CHECK(!to_int(-2147483649LL).ok);
  const IntResult low = to_int(-2147483648LL);
  RC_REQUIRE(low.ok);
  RC_CHECK_EQ(low.value, kIntMin);
}

RC_TEST("a float holds whole numbers exactly only up to 16777216") {
  RC_CHECK(survives_float(0));
  RC_CHECK(survives_float(1));
  RC_CHECK(survives_float(-1));
  RC_CHECK(survives_float(16777216));
  RC_CHECK(!survives_float(16777217));
  RC_CHECK(!survives_float(-16777217));
  RC_CHECK(survives_float(16777218));

  int lost = 0;
  for (int i = 16777216; i < 16777216 + 100; ++i)
    if (!survives_float(i)) ++lost;

  // A double holds every 32 bit integer, sampled across the whole range.
  long long double_lost = 0;
  long long sampled = 0;
  for (long long i = -2147483648LL; i <= 2147483647LL; i += 65537) {
    ++sampled;
    if (static_cast<long long>(static_cast<double>(i)) != i) ++double_lost;
  }

  std::cout << "  of 100 integers starting at 16777216, a float cannot hold  " << lost << "\n";
  std::cout << "  of " << sampled << " integers across the whole int range, a double cannot hold  "
            << double_lost << "\n\n";
  std::cout << "  16777217 as a float: "
            << static_cast<long long>(static_cast<float>(16777217)) << "\n";
  std::cout << "  100000001 as a float: "
            << static_cast<long long>(static_cast<float>(100000001)) << "\n\n";

  RC_CHECK_EQ(lost, 50);
  RC_CHECK_EQ(double_lost, 0LL);
}
