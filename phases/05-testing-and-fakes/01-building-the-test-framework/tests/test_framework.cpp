// The real framework, testing the one you are building. Two separate pieces of
// code, only one of them under test, so there is nothing circular about it.
#include <rc/test/rc_test.hpp>

#include <string>

#include "solution.hpp"

// These register themselves before main, through the mechanism the lesson is
// about. Three pass and two fail, deliberately, so the report can be checked.
MINI_TEST("arithmetic works") {
  MINI_CHECK(2 + 2 == 4);
}

MINI_TEST("this one is meant to fail") {
  MINI_CHECK(1 == 2);
}

MINI_TEST("several checks in one test") {
  MINI_CHECK(true);
  MINI_CHECK(1 < 2);
  MINI_CHECK(!false);
}

MINI_TEST("this one is also meant to fail, twice") {
  MINI_CHECK(false);
  MINI_CHECK(1 > 2);
}

MINI_TEST("a test with no checks at all passes") {
  // Nothing recorded means nothing failed.
}

RC_TEST("every test registered itself before main") {
  // Nothing in this file calls a register function. If the count is right, the
  // constructors of the file scope registrar objects ran on their own.
  RC_CHECK_EQ(mini::registry().size(), std::size_t{5});
}

RC_TEST("registered tests keep the names they were given") {
  bool found = false;
  for (const mini::Case& test : mini::registry())
    if (test.name == "arithmetic works") found = true;
  RC_CHECK(found);
}

RC_TEST("running reports the right number of passes and failures") {
  const mini::Report report = mini::run_all();
  RC_CHECK_EQ(report.passed, 3);
  RC_CHECK_EQ(report.failed, 2);
}

RC_TEST("a test is counted once however many checks fail inside it") {
  // The test with two failing checks must still count as one failed test.
  const mini::Report report = mini::run_all();
  RC_CHECK_EQ(report.passed + report.failed, 5);
}

RC_TEST("every failing check is recorded, not only the first per test") {
  mini::run_all();
  RC_CHECK_EQ(mini::failures().size(), std::size_t{3});
}

RC_TEST("a failure names the test it came from") {
  mini::run_all();
  RC_REQUIRE(!mini::failures().empty());
  bool named = false;
  for (const mini::Failure& failure : mini::failures())
    if (failure.test == "this one is meant to fail") named = true;
  RC_CHECK(named);
}

RC_TEST("a failure carries the file and line it happened on") {
  mini::run_all();
  RC_REQUIRE(!mini::failures().empty());
  const mini::Failure& first = mini::failures().front();
  RC_CHECK(!first.file.empty());
  RC_CHECK(first.line > 0);
}

RC_TEST("a failure quotes the expression that failed") {
  // This is what the stringify operator buys: the report names the code rather
  // than only the values.
  mini::run_all();
  bool quoted = false;
  for (const mini::Failure& failure : mini::failures())
    if (failure.message.find("1 == 2") != std::string::npos) quoted = true;
  RC_CHECK(quoted);
}

RC_TEST("running twice gives the same answer") {
  // A framework whose result depends on how many times it has been asked is not
  // much of a framework. This catches failures being accumulated across runs.
  const mini::Report first = mini::run_all();
  const mini::Report second = mini::run_all();
  RC_CHECK_EQ(first.passed, second.passed);
  RC_CHECK_EQ(first.failed, second.failed);
  RC_CHECK_EQ(mini::failures().size(), std::size_t{3});
}

RC_TEST("the current test name is cleared once the run is over") {
  mini::run_all();
  RC_CHECK(mini::current_test().empty());
}
