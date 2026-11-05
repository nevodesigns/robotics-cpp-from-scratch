#include <rc/test/rc_test.hpp>

#include <iostream>
#include <iterator>
#include <type_traits>
#include <vector>

#include "solution.hpp"

RC_TEST("one name in two namespaces is two functions, each reaching the global clamp") {
  clamp_calls() = 0;

  const double fast = drive::limit(2.0);
  const double slow = drive::limit(-2.0);
  const double fine = drive::limit(0.25);
  RC_CHECK_NEAR(fast, 1.0, 1e-12);
  RC_CHECK_NEAR(slow, -1.0, 1e-12);
  RC_CHECK_NEAR(fine, 0.25, 1e-12);
  RC_CHECK_EQ(clamp_calls(), 3);

  const double high = arm::limit(5.0);
  const double low = arm::limit(-5.0);
  const double mid = arm::limit(1.0);
  RC_CHECK_NEAR(high, arm::kQuarterTurn, 1e-12);
  RC_CHECK_NEAR(low, -arm::kQuarterTurn, 1e-12);
  RC_CHECK_NEAR(mid, 1.0, 1e-12);
  RC_CHECK_EQ(clamp_calls(), 6);

  std::cout << "  the same call, limit(5.0), in two namespaces:\n";
  std::cout << "    drive::limit(5.0)   " << drive::limit(5.0) << "\n";
  std::cout << "    arm::limit(5.0)     " << arm::limit(5.0) << "\n\n";
  RC_CHECK(drive::limit(5.0) != arm::limit(5.0));
}

RC_TEST("a using-declaration names one function and an alias shortens a prefix") {
  {
    using drive::limit;
    RC_CHECK_NEAR(limit(2.0), 1.0, 1e-12);
  }
  {
    using arm::limit;
    RC_CHECK_NEAR(limit(5.0), arm::kQuarterTurn, 1e-12);
  }
  {
    namespace a = arm;
    RC_CHECK_NEAR(a::limit(-5.0), -arm::kQuarterTurn, 1e-12);
  }
}

RC_TEST("using namespace can change which function a call reaches, and nothing says so") {
  // The global clamp takes doubles. general::clamp takes ints. A call with ints
  // fits the second exactly and the first only after a conversion.
  clamp_calls() = 0;
  const auto before = clamp(5, 0, 3);
  const int global_calls_before = clamp_calls();

  int global_calls_after = -1;
  bool after_is_int = false;
  double through_drive = 0.0;
  int global_calls_through_drive = -1;
  {
    using namespace general;
    clamp_calls() = 0;
    const auto after = clamp(5, 0, 3);
    global_calls_after = clamp_calls();
    after_is_int = std::is_same<decltype(after), const int>::value;

    // The learner's functions name the global clamp with a leading ::, so a
    // using-directive somewhere else cannot redirect them.
    clamp_calls() = 0;
    through_drive = drive::limit(2.0);
    global_calls_through_drive = clamp_calls();
  }

  std::cout << "  clamp(5, 0, 3), written the same way twice:\n\n";
  std::cout << "                                   result type   global clamp ran\n";
  std::cout << "    as written                     "
            << (std::is_same<decltype(before), const double>::value ? "double" : "int   ")
            << "        " << global_calls_before << " time\n";
  std::cout << "    after using namespace general  " << (after_is_int ? "int   " : "double")
            << "        " << global_calls_after << " times\n\n";
  std::cout << "  With using namespace std, the same thing happened to std::clamp.\n";
  std::cout << "  Run once outside this suite on gcc 11 and clang 14, a call that had\n";
  std::cout << "  reached the global double clamp returned an int from std::clamp, with\n";
  std::cout << "  no warning and the global function never called.\n\n";

  RC_CHECK((std::is_same<decltype(before), const double>::value));
  RC_CHECK_EQ(global_calls_before, 1);
  RC_CHECK(after_is_int);
  RC_CHECK_EQ(global_calls_after, 0);

  // And the learner's function is not redirected by it.
  RC_CHECK_NEAR(through_drive, 1.0, 1e-12);
  RC_CHECK_EQ(global_calls_through_drive, 1);
}

RC_TEST("an unqualified name can be found through its arguments, for some containers") {
  std::vector<int> readings = {4, 8, 15};

  // distance is in std, and nothing here says std::. The iterators of a
  // std::vector belong to std, so the compiler looks there too.
  const auto unqualified = distance(readings.begin(), readings.end());
  const auto qualified = std::distance(readings.begin(), readings.end());

  std::cout << "  distance(v.begin(), v.end()) with no std::   " << unqualified << "\n";
  std::cout << "  std::distance(v.begin(), v.end())            " << qualified << "\n";
  std::cout << "  the same line with an int array is refused by the compiler, which\n";
  std::cout << "  checks/a_name_that_only_worked_for_a_vector.cpp proves.\n\n";

  RC_CHECK_EQ(unqualified, 3);
  RC_CHECK_EQ(qualified, 3);

  // The learner's namespaced functions are reached by writing their prefix, and
  // a prefix is the one thing that cannot be found by accident.
  RC_CHECK_NEAR(::drive::limit(9.0), 1.0, 1e-12);
  RC_CHECK_NEAR(::arm::limit(-9.0), -arm::kQuarterTurn, 1e-12);
}
