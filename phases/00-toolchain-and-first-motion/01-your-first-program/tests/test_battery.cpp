// The tests for lesson 00-01.
//
// This exact file runs against your code and against the worked implementation.
// If it passes for you, you are done, by the same standard the repository holds
// itself to.

#include <rc/test/rc_test.hpp>

#include "solution.hpp"

RC_TEST("an empty battery reads zero percent") {
  RC_CHECK_EQ(battery_percent(3000), 0);
}

RC_TEST("a full battery reads one hundred percent") {
  RC_CHECK_EQ(battery_percent(4200), 100);
}

RC_TEST("the halfway voltage reads fifty percent") {
  RC_CHECK_EQ(battery_percent(3600), 50);
}

RC_TEST("a reading below empty never goes negative") {
  RC_CHECK_EQ(battery_percent(2500), 0);
  RC_CHECK_EQ(battery_percent(0), 0);
}

RC_TEST("a reading above full never passes one hundred") {
  RC_CHECK_EQ(battery_percent(4500), 100);
  RC_CHECK_EQ(battery_percent(100000), 100);
}

RC_TEST("the percentage rises as the voltage rises") {
  // Whatever formula you chose, more volts must never mean less charge.
  int previous = battery_percent(3000);
  for (int mv = 3000; mv <= 4200; mv += 25) {
    const int current = battery_percent(mv);
    RC_CHECK(current >= previous);
    previous = current;
  }
}
