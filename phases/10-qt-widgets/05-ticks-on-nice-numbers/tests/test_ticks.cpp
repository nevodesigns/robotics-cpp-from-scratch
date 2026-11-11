#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include "solution.hpp"

RC_TEST("adding 0.1 ten times is not 1, and 3 times 0.1 prints as 0.30000000000000004") {
  double sum = 0.0;
  for (int i = 0; i < 10; ++i) sum += 0.1;
  std::ostringstream long_form;
  long_form << std::setprecision(17) << 3 * 0.1;
  std::cout << "  0.1 added ten times: " << std::setprecision(17) << sum << "; 3 * 0.1 prints as " << long_form.str() << "\n";
  RC_CHECK(sum != 1.0);
  RC_CHECK_EQ(long_form.str(), std::string("0.30000000000000004"));

  // The ticks for 0 to 1 are on tenths and print as one place after the point.
  const Ticks t = ticks_for(0.0, 1.0, 5);
  std::cout << "  ticks for 0 to 1, about 5: step " << t.step << ", " << t.count << " ticks:";
  for (int k = 0; k < t.count; ++k) std::cout << " " << tick_label(t.value(k), t.decimals);
  std::cout << "\n\n";
  RC_CHECK_EQ(t.count, 6);
  RC_CHECK_NEAR(t.step, 0.2, 1e-12);
  RC_CHECK_EQ(t.decimals, 1);
  if (t.count == 6) {
    RC_CHECK_EQ(tick_label(t.value(0), t.decimals), std::string("0.0"));
    RC_CHECK_EQ(tick_label(t.value(3), t.decimals), std::string("0.6"));
    RC_CHECK_EQ(tick_label(t.value(5), t.decimals), std::string("1.0"));
    RC_CHECK(t.value(3) == 0.6);       // exactly the double nearest 0.6, not 0.6000000000000001
  }
}

RC_TEST("the step is 1, 2 or 5 times a power of ten, the smallest that is at least span over target") {
  RC_CHECK_NEAR(nice_step(1.0, 5), 0.2, 1e-12);
  RC_CHECK_NEAR(nice_step(10.0, 5), 2.0, 1e-12);
  RC_CHECK_NEAR(nice_step(7.3, 5), 2.0, 1e-12);        // 1.46 goes up to 2
  RC_CHECK_NEAR(nice_step(100.0, 4), 50.0, 1e-9);      // 25 goes up to 50
  RC_CHECK_NEAR(nice_step(0.0004, 5), 1e-4, 1e-15);    // 8e-5 goes up to 1e-4
  RC_CHECK_NEAR(nice_step(0.5, 5), 0.1, 1e-12);        // exactly 0.1 stays 0.1
  RC_CHECK_NEAR(nice_step(3.0, 6), 0.5, 1e-12);
  RC_CHECK_NEAR(nice_step(0.0, 5), 1.0, 1e-12);
  RC_CHECK_NEAR(nice_step(-3.0, 5), 1.0, 1e-12);
  RC_CHECK_NEAR(nice_step(5.0, 0), 1.0, 1e-12);
}

RC_TEST("every tick is inside the range, evenly spaced, and there are about as many as asked for") {
  const double lows[] = {-1.3, 0.0, 0.0003, 5.0, 1e6, -2e-9, -250.0};
  const double spans[] = {0.7, 1.0, 3.3, 12.0, 0.0004, 1000.0};
  int ranges = 0;
  int outside = 0;
  int uneven = 0;
  int bad_count = 0;
  for (double low : lows) {
    for (double span : spans) {
      const double high = low + span;
      const Ticks t = ticks_for(low, high, 6);
      ++ranges;
      if (t.count < 2 || t.count > 13) ++bad_count;
      for (int k = 0; k < t.count; ++k) {
        const double v = t.value(k);
        if (v < low - 1e-9 * span || v > high + 1e-9 * span) ++outside;
        if (k > 0 && std::fabs((v - t.value(k - 1)) - t.step) > 1e-6 * t.step) ++uneven;
      }
    }
  }
  std::cout << "  " << ranges << " ranges: " << outside << " ticks outside, " << uneven << " uneven gaps, " << bad_count << " with a count outside 2 to 13\n\n";
  RC_CHECK_EQ(ranges, 42);
  RC_CHECK_EQ(outside, 0);
  RC_CHECK_EQ(uneven, 0);
  RC_CHECK_EQ(bad_count, 0);

  // A range given backwards is the same range.
  const Ticks a = ticks_for(2.0, -1.0, 6);
  const Ticks b = ticks_for(-1.0, 2.0, 6);
  RC_CHECK_EQ(a.count, b.count);
  RC_CHECK(a.count > 0);
  RC_CHECK_NEAR(a.step, b.step, 1e-15);
}

RC_TEST("a label has the decimals of the axis, and a value that rounds to zero is not written minus zero") {
  RC_CHECK_EQ(tick_label(0.5, 1), std::string("0.5"));
  RC_CHECK_EQ(tick_label(12.0, 0), std::string("12"));
  RC_CHECK_EQ(tick_label(0.05, 2), std::string("0.05"));
  RC_CHECK_EQ(tick_label(-1.5, 1), std::string("-1.5"));
  std::cout << "  -0.004 with two decimals: \"" << tick_label(-0.004, 2) << "\"; plain fixed output gives \"";
  std::ostringstream plain;
  plain << std::fixed << std::setprecision(2) << -0.004;
  std::cout << plain.str() << "\"\n\n";
  RC_CHECK_EQ(plain.str(), std::string("-0.00"));
  RC_CHECK_EQ(tick_label(-0.004, 2), std::string("0.00"));
  RC_CHECK_EQ(tick_label(-0.006, 2), std::string("-0.01"));
  RC_CHECK_EQ(tick_label(-0.0, 1), std::string("0.0"));

  // Across zero, the tick at zero is labelled 0.0.
  const Ticks t = ticks_for(-0.3, 0.3, 6);
  int zeros = 0;
  for (int k = 0; k < t.count; ++k)
    if (tick_label(t.value(k), t.decimals) == "0.0") ++zeros;
  RC_CHECK_EQ(t.count, 7);
  RC_CHECK_EQ(zeros, 1);
}

RC_TEST("a steady signal has one tick, at its own value, and a nearly steady one has a step that is not zero") {
  const Ticks flat = ticks_for(5.0, 5.0, 5);
  std::cout << "  range 5 to 5: " << flat.count << " tick, value " << flat.value(0) << ", label " << tick_label(flat.value(0), flat.decimals) << "\n";
  RC_CHECK_EQ(flat.count, 1);
  RC_CHECK(flat.steady);
  RC_CHECK_NEAR(flat.value(0), 5.0, 1e-15);
  RC_CHECK_EQ(tick_label(flat.value(0), flat.decimals), std::string("5.00"));

  const Ticks near = ticks_for(5.0, 5.0 + 1e-14, 5);
  std::cout << "  range 5 to 5 + 1e-14: " << near.count << " tick, steady " << near.steady << "\n\n";
  RC_CHECK(near.steady);               // narrower than a double can show at this size: it is one value
  RC_CHECK_EQ(near.count, 1);

  const Ticks narrow = ticks_for(5.0, 5.0 + 1e-6, 5);
  RC_CHECK(!narrow.steady);
  RC_CHECK(narrow.step > 0.0);
  RC_CHECK(narrow.count >= 2 && narrow.count <= 11);
  RC_CHECK(narrow.decimals >= 6);      // the labels show the difference between the ticks
}
