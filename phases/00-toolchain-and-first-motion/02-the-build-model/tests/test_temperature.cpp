#include <rc/test/rc_test.hpp>

#include "solution.hpp"

RC_TEST("zero counts is zero degrees") {
  RC_CHECK_NEAR(celsius_from_raw(0), 0.0, 1e-9);
}

RC_TEST("320 counts is twenty degrees") {
  RC_CHECK_NEAR(celsius_from_raw(320), 20.0, 1e-9);
}

RC_TEST("negative counts read below zero") {
  RC_CHECK_NEAR(celsius_from_raw(-160), -10.0, 1e-9);
}

RC_TEST("a cool sensor is not overheating") {
  RC_CHECK(!is_overheating(320));
}

RC_TEST("a hot sensor is overheating") {
  RC_CHECK(is_overheating(1600));
}

RC_TEST("the overheating threshold sits at eighty degrees") {
  RC_CHECK(!is_overheating(1280));   // exactly 80.0, which is not above 80
  RC_CHECK(is_overheating(1281));
}
