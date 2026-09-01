#include <rc/test/rc_test.hpp>

#include "solution.hpp"

// These tests only run once the file compiles, which is the exercise. They are
// here so that fixing the errors by deleting the code does not count as fixing
// them: each function still has to do what its comment says it does.

RC_TEST("the distance from the origin is Pythagoras") {
  RC_CHECK_NEAR(distance_from_origin(3.0, 4.0), 5.0, 1e-12);
  RC_CHECK_NEAR(distance_from_origin(0.0, 0.0), 0.0, 1e-12);
  RC_CHECK_NEAR(distance_from_origin(-3.0, -4.0), 5.0, 1e-12);
}

RC_TEST("speed comes from both components, not one") {
  // A function that ignores its second argument passes a test using only the
  // first, which is why the second is not zero here.
  RC_CHECK_NEAR(speed_from_components(3.0, 4.0), 5.0, 1e-12);
  RC_CHECK_NEAR(speed_from_components(1.0, 0.0), 1.0, 1e-12);
  RC_CHECK_NEAR(speed_from_components(0.0, 2.5), 2.5, 1e-12);
}

RC_TEST("near zero is about size, not sign") {
  RC_CHECK(near_zero(0.0));
  RC_CHECK(near_zero(1e-12));
  RC_CHECK(near_zero(-1e-12));
  RC_CHECK(!near_zero(0.001));
  RC_CHECK(!near_zero(-0.001));
}

RC_TEST("clamping holds a percentage inside its range") {
  RC_CHECK_EQ(clamp_percent(50), 50);
  RC_CHECK_EQ(clamp_percent(0), 0);
  RC_CHECK_EQ(clamp_percent(100), 100);
  RC_CHECK_EQ(clamp_percent(-20), 0);
  RC_CHECK_EQ(clamp_percent(140), 100);
}

RC_TEST("the battery reading converts across its whole range") {
  RC_CHECK_EQ(battery_percent(3000), 0);
  RC_CHECK_EQ(battery_percent(4200), 100);
  RC_CHECK_EQ(battery_percent(3600), 50);
  RC_CHECK_EQ(battery_percent(2500), 0);     // below empty is still zero
  RC_CHECK_EQ(battery_percent(5000), 100);   // above full is still a hundred
}

RC_TEST("far means far in both directions") {
  // The one that catches a call passing only x. With y ignored, a point ten
  // metres up the y axis reads as being at the origin.
  RC_CHECK(is_far(20.0, 0.0));
  RC_CHECK(is_far(0.0, 20.0));
  RC_CHECK(is_far(-20.0, 0.0));
  RC_CHECK(!is_far(1.0, 1.0));
  RC_CHECK(!is_far(0.0, 0.0));
}
