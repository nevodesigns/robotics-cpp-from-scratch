#include <rc/test/rc_test.hpp>

#include "solution.hpp"

RC_TEST("the middle of the range is about half the voltage") {
  RC_CHECK_NEAR(adc_to_volts(2048), 1.6504, 1e-3);
}

RC_TEST("the ends of the adc range are exact") {
  RC_CHECK_NEAR(adc_to_volts(0), 0.0, 1e-9);
  RC_CHECK_NEAR(adc_to_volts(4095), 3.3, 1e-9);
}

RC_TEST("small readings are not flattened to zero") {
  // This is the check the buggy version fails hardest. A count of 100 is a real
  // voltage, not nothing.
  RC_CHECK(adc_to_volts(100) > 0.07);
  RC_CHECK(adc_to_volts(100) < 0.09);
}

RC_TEST("percentages round to nearest") {
  RC_CHECK_EQ(percent_of(1, 3), 33);
  RC_CHECK_EQ(percent_of(2, 3), 67);
  RC_CHECK_EQ(percent_of(1, 2), 50);
}

RC_TEST("a zero denominator returns zero instead of crashing") {
  RC_CHECK_EQ(percent_of(5, 0), 0);
}

RC_TEST("an average keeps its fractional part") {
  const int samples[] = {1, 2};
  RC_CHECK_NEAR(average(samples, 2), 1.5, 1e-9);

  const int uneven[] = {1, 2, 2};
  RC_CHECK_NEAR(average(uneven, 3), 1.6666666, 1e-6);
}

RC_TEST("an empty sample set averages to zero without dividing by zero") {
  RC_CHECK_NEAR(average(nullptr, 0), 0.0, 1e-9);
}
