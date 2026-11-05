#include <rc/test/rc_test.hpp>

#include <iomanip>
#include <iostream>

#include "solution.hpp"

namespace {

// The same job as shared_sensor_id, written without the word static: a Sensor
// built fresh on every call.
int fresh_sensor_id() {
  Sensor sensor;
  return sensor.id();
}

// A wheel that counts how many wheels exist by keeping the count in each wheel.
struct MemberCounted {
  double radius_ = 0.0;
  int count_ = 0;
};

}  // namespace

RC_TEST("a static local keeps its value from one call to the next") {
  const int first = next_id();
  const int second = next_id();
  const int third = next_id();

  std::cout << "  three calls to next_id(): " << first << " " << second << " " << third << "\n";
  std::cout << "  what the first one was depends on how many calls came before it, in\n";
  std::cout << "  this program, which is the cost: the function has a memory, and a test\n";
  std::cout << "  cannot start it from zero.\n\n";

  RC_CHECK_EQ(second, first + 1);
  RC_CHECK_EQ(third, first + 2);
  RC_CHECK(first >= 1);

  // The memory belongs to the function. Asking for ten more picks up where the
  // last left off.
  int last = third;
  for (int i = 0; i < 10; ++i) {
    const int next = next_id();
    RC_CHECK_EQ(next, last + 1);
    last = next;
  }
}

RC_TEST("a static local is built when it is first reached, and only then, and once") {
  // Nothing before this test has asked for the shared sensor.
  const int before = sensor_constructions();

  const int first_id = shared_sensor_id();
  const int after_first = sensor_constructions();

  int same_every_time = 1;
  for (int i = 0; i < 10; ++i)
    if (shared_sensor_id() != first_id) same_every_time = 0;
  const int after_many = sensor_constructions();

  const int before_fresh = sensor_constructions();
  for (int i = 0; i < 10; ++i) fresh_sensor_id();
  const int after_fresh = sensor_constructions();

  std::cout << "  sensors built:\n";
  std::cout << "    before shared_sensor_id() was ever called   " << before << "\n";
  std::cout << "    after the first call                        " << after_first << "\n";
  std::cout << "    after ten more calls                        " << after_many << "\n";
  std::cout << "    ten calls to a version without static       " << (after_fresh - before_fresh)
            << "\n\n";

  RC_CHECK_EQ(before, 0);
  RC_CHECK_EQ(after_first, 1);
  RC_CHECK_EQ(after_many, 1);
  RC_CHECK_EQ(same_every_time, 1);
  RC_CHECK_EQ(after_fresh - before_fresh, 10);
}

RC_TEST("a static member belongs to the class, not to any object") {
  RC_CHECK_EQ(Wheel::count(), 0);

  const Wheel front(0.15);
  RC_CHECK_EQ(Wheel::count(), 1);
  {
    const Wheel rear(0.20);
    RC_CHECK_EQ(Wheel::count(), 2);
    RC_CHECK_NEAR(rear.radius(), 0.20, 1e-12);
  }
  RC_CHECK_EQ(Wheel::count(), 1);

  // A copy is a new wheel. A counter that only the ordinary constructor
  // increments goes wrong here, and goes wrong in the other direction when the
  // copy is destroyed.
  {
    const Wheel spare(front);
    RC_CHECK_EQ(Wheel::count(), 2);
    RC_CHECK_NEAR(spare.radius(), 0.15, 1e-12);
  }
  RC_CHECK_EQ(Wheel::count(), 1);

  std::cout << "  sizeof(Wheel), the count kept in the class      " << sizeof(Wheel) << "\n";
  std::cout << "  sizeof of the same, with the count in each one  " << sizeof(MemberCounted)
            << "\n\n";
  RC_CHECK_EQ(sizeof(Wheel), sizeof(double));
  RC_CHECK(sizeof(MemberCounted) > sizeof(Wheel));
}

RC_TEST("static data is built before main and destroyed after it, in reverse") {
  // Not observable from inside a test, so this one reports what was measured
  // once, outside the suite, and asserts the part that can be asserted.
  std::cout << "  Experiment, run once on gcc 11 and clang 14: a global object, then two\n";
  std::cout << "  static locals reached in the order B, A.\n\n";
  std::cout << "    built     the global           before main began\n";
  std::cout << "    built     static local B       when second() was first called\n";
  std::cout << "    built     static local A       when first() was first called\n";
  std::cout << "    destroyed static local A       after main returned\n";
  std::cout << "    destroyed static local B\n";
  std::cout << "    destroyed the global\n\n";

  // What can be asserted: the learner's class-wide counter is back to zero once
  // every wheel is gone, which is the same bookkeeping at a smaller scale.
  {
    const Wheel a(1.0);
    const Wheel b(2.0);
    RC_CHECK_EQ(Wheel::count(), 2);
  }
  RC_CHECK_EQ(Wheel::count(), 0);
}
