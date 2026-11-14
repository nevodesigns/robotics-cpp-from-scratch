#include <rc/test/rc_test.hpp>

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include "solution.hpp"

namespace {

struct Rng {
  std::uint32_t state;
  std::uint32_t next() {
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
  }
  double uniform() { return static_cast<double>(next() >> 8) / 16777216.0; }
};

bool reads_back(const std::string& text, double value) { return std::strtod(text.c_str(), nullptr) == value; }

// A mixed set of readings: ordinary sizes, tiny ones and huge ones, with a full random mantissa.
double reading(Rng& rng) {
  const double m0 = rng.uniform();   // one draw at a time: the order in which the terms of a sum are evaluated is the compiler's choice
  const double m1 = rng.uniform();
  const double m2 = rng.uniform();
  const double mantissa = 1.0 + m0 + m1 * 1e-8 + m2 * 1e-16;
  const int exponent = static_cast<int>(rng.next() % 41) - 20;
  const double sign = (rng.next() & 1u) ? 1.0 : -1.0;
  return sign * mantissa * std::pow(10.0, exponent);
}

}  // namespace

RC_TEST("short numbers stay short and the sum that is not 0.3 shows it") {
  RC_CHECK_EQ(shortest_text(0.5), std::string("0.5"));
  RC_CHECK_EQ(shortest_text(100.0), std::string("100"));
  RC_CHECK_EQ(shortest_text(0.1), std::string("0.1"));
  RC_CHECK_EQ(shortest_text(-2.25), std::string("-2.25"));
  RC_CHECK_EQ(shortest_text(0.0), std::string("0"));
  RC_CHECK_EQ(shortest_text(-0.0), std::string("-0"));

  const double sum = 0.1 + 0.2;
  std::cout << "  0.1 + 0.2 is written \"" << shortest_text(sum) << "\" and 0.3 is written \"" << shortest_text(0.3)
            << "\"; six digits write both as \"" << default_text(sum) << "\"\n\n";
  RC_CHECK_EQ(shortest_text(0.3), std::string("0.3"));
  RC_CHECK_EQ(shortest_text(sum), std::string("0.30000000000000004"));
  RC_CHECK_EQ(default_text(sum), default_text(0.3));
}

RC_TEST("not a number and infinity are written as words") {
  RC_CHECK_EQ(shortest_text(std::nan("")), std::string("nan"));
  RC_CHECK_EQ(shortest_text(HUGE_VAL), std::string("inf"));
  RC_CHECK_EQ(shortest_text(-HUGE_VAL), std::string("-inf"));
}

RC_TEST("written and read back, the shortest text is exact every time and the other two often are not") {
  Rng rng{2024};
  const int n = 20000;
  int shortest_ok = 0;
  int default_ok = 0;
  int to_string_ok = 0;
  for (int i = 0; i < n; ++i) {
    const double v = reading(rng);
    if (reads_back(shortest_text(v), v)) ++shortest_ok;
    if (reads_back(default_text(v), v)) ++default_ok;
    if (reads_back(std::to_string(v), v)) ++to_string_ok;
  }
  std::cout << "  " << n << " readings of mixed size, written as text and read back exactly:\n"
            << "    shortest_text          " << std::setw(6) << shortest_ok << "\n"
            << "    stream, six digits     " << std::setw(6) << default_ok << "\n"
            << "    std::to_string         " << std::setw(6) << to_string_ok << "\n\n";
  RC_CHECK_EQ(shortest_ok, n);
  RC_CHECK(default_ok < n / 100);
  RC_CHECK(to_string_ok < n / 2);
}

RC_TEST("std::to_string writes a small number as zero") {
  const double tiny = 3.5e-9;
  std::cout << "  3.5e-9 through std::to_string: \"" << std::to_string(tiny) << "\", through shortest_text: \""
            << shortest_text(tiny) << "\"\n\n";
  RC_CHECK_EQ(std::to_string(tiny), std::string("0.000000"));
  RC_CHECK(reads_back(shortest_text(tiny), tiny));
  RC_CHECK(shortest_text(tiny) != "0.000000");
}

RC_TEST("the shortest text is much shorter than always writing seventeen digits") {
  Rng rng{9};
  const int n = 5000;
  double shortest_total = 0.0;
  double seventeen_total = 0.0;
  for (int i = 0; i < n; ++i) {
    // A reading from a sensor with two decimal places, which is what most real ones are.
    const double v = std::round(rng.uniform() * 100000.0) / 100.0;
    std::ostringstream seventeen;
    seventeen << std::setprecision(17) << v;
    shortest_total += static_cast<double>(shortest_text(v).size());
    seventeen_total += static_cast<double>(seventeen.str().size());
    RC_REQUIRE(reads_back(shortest_text(v), v));
  }
  std::cout << "  readings to two decimal places: mean length " << std::fixed << std::setprecision(1)
            << shortest_total / n << " characters shortest, " << seventeen_total / n << " at seventeen digits\n\n";
  RC_CHECK(shortest_total / n < 9.0);
  RC_CHECK(seventeen_total / n > 12.0);
}

RC_TEST("a number is read from text, and anything else on the line, or no number, or not a finite one, is refused") {
  double v = 0.0;
  RC_CHECK(parse_number("1.5", &v));
  RC_CHECK_NEAR(v, 1.5, 1e-15);
  RC_CHECK(parse_number("  -2e3  ", &v));
  RC_CHECK_NEAR(v, -2000.0, 1e-12);

  const char* refused[] = {"", "   ", "1.5x", "1,5", "two", "1 2", "nan", "inf", "-inf", "1e999"};
  int count = 0;
  for (const char* text : refused) {
    double before = 7.0;
    if (!parse_number(text, &before)) ++count;
    RC_CHECK_NEAR(before, 7.0, 1e-15);   // a refusal leaves the output alone
  }
  std::cout << "  " << count << " of " << sizeof(refused) / sizeof(refused[0]) << " bad texts refused\n\n";
  RC_CHECK_EQ(count, 10);

  // And what shortest_text writes, parse_number reads.
  RC_CHECK(parse_number(shortest_text(0.1 + 0.2).c_str(), &v));
  RC_CHECK(v == 0.1 + 0.2);
}
