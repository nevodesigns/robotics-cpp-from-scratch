#include <rc/test/rc_test.hpp>

#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "solution.hpp"

// assert, with NDEBUG forced off, so this file can show both states whatever
// the build is configured as.
#undef NDEBUG
#include <cassert>

namespace {

// Everything the handler saw, so a failure can be inspected rather than
// terminating the run.
std::vector<CheckFailure> seen;

void record(const CheckFailure& failure) { seen.push_back(failure); }

// Restores the default handler however the test leaves, which matters because
// the handler is one object shared by every test in this binary.
struct HandlerFor {
  explicit HandlerFor(CheckHandler handler) : previous(current_check_handler()) {
    seen.clear();
    set_check_handler(handler);
  }
  ~HandlerFor() { set_check_handler(previous); }
  CheckHandler previous;
};

int counted_increment(int* counter) {
  ++(*counter);
  return *counter;
}

}  // namespace

// Three functions, identical but for the state of NDEBUG when each was
// compiled. <cassert> is the one standard header you may include more than
// once to get different behaviour, and this is what it is for.
namespace {
int assert_while_on(int* counter) {
  assert(counted_increment(counter) > 0);
  return *counter;
}
}  // namespace

#define NDEBUG
#include <cassert>
namespace {
int assert_while_off(int* counter) {
  assert(counted_increment(counter) > 0);
  return *counter;
}
}  // namespace
#undef NDEBUG
#include <cassert>

RC_TEST("an assert with NDEBUG defined does not evaluate its condition") {
  int counter = 0;

  assert_while_on(&counter);
  const int after_on = counter;

  assert_while_off(&counter);
  const int after_off = counter;

  std::cout << "\n  a counter incremented inside assert(...)\n\n";
  std::cout << "    with NDEBUG undefined   " << after_on << "\n";
  std::cout << "    with NDEBUG defined     " << after_off
            << "   <- the call inside it never happened\n\n";

  RC_CHECK(after_on == 1);

  // Not "the check was skipped". The expression was not evaluated at all, so
  // everything it was doing stopped happening. A check with a side effect in
  // it silently loses the side effect.
  RC_CHECK(after_off == 1);

  // And the contrast, in the same file, with NDEBUG defined above it being
  // irrelevant: RC_EXPECT is not conditional on anything, so it still runs its
  // condition and still reports.
  const HandlerFor recording(&record);
  int surviving = 0;
  RC_EXPECT(counted_increment(&surviving) > 99, "deliberately false");
  RC_CHECK(surviving == 1);
  RC_CHECK(seen.size() == 1u);

  std::cout << "  the same file's RC_EXPECT evaluated its condition and reported\n\n";
  std::cout << "  NDEBUG is defined by every release configuration this\n";
  std::cout << "  curriculum builds, so that is the build you ship\n\n";
}

RC_TEST("RC_EXPECT is still there, and evaluates its condition exactly once") {
  const HandlerFor quiet(&record);

  int counter = 0;
  const bool held = RC_EXPECT(counted_increment(&counter) > 0, "must be positive");

  RC_CHECK(held);
  RC_CHECK(counter == 1);

  // And once on the failing path too, which is where a macro that names its
  // argument twice goes wrong.
  int failing = 0;
  const bool broke = RC_EXPECT(counted_increment(&failing) > 99, "will not hold");

  RC_CHECK(!broke);
  RC_CHECK(failing == 1);

  // The one that held reported nothing and the one that did not reported
  // once, so "evaluated once" is not being satisfied by never running.
  RC_CHECK(seen.size() == 1u);

  std::cout << "  the condition ran once when it held and once when it did not,\n";
  std::cout << "  and only the failing one was reported\n\n";
}

RC_TEST("a failure says what failed and where") {
  const HandlerFor recording(&record);

  const int line_before = __LINE__;
  const bool held = RC_EXPECT(1 > 2, "one is not greater than two");
  RC_CHECK(!held);

  RC_REQUIRE(seen.size() == 1u);
  const CheckFailure& failure = seen.front();

  std::cout << "  expression  " << failure.expression << "\n";
  std::cout << "  message     " << failure.message << "\n";
  std::cout << "  line        " << failure.line << " (the check is on "
            << (line_before + 1) << ")\n\n";

  // The expression as it was written, so a reader does not have to guess which
  // check in the function it was.
  RC_CHECK(std::string(failure.expression) == "1 > 2");
  RC_CHECK(std::string(failure.message) == "one is not greater than two");
  RC_CHECK(failure.line == line_before + 1);
  RC_CHECK(std::string(failure.file).find("test_expect") != std::string::npos);
}

RC_TEST("a check that holds reports nothing") {
  const HandlerFor recording(&record);

  for (int i = 0; i < 100; ++i) RC_CHECK(RC_EXPECT(i >= 0, "loop index"));

  RC_CHECK(seen.empty());
  std::cout << "  a hundred checks that held produced " << seen.size()
            << " reports\n";

  // And one that does not hold produces exactly one, so the silence above is
  // the handler being quiet rather than the handler never being called.
  RC_CHECK(!RC_EXPECT(false, "the hundred and first"));
  RC_CHECK(seen.size() == 1u);
  std::cout << "  and one that did not produced " << seen.size() << "\n\n";
}

RC_TEST("the caller decides what a violation means") {
  const HandlerFor recording(&record);

  const double limit = 2.0;

  // Inside the limit: no report, and the value comes back untouched.
  RC_CHECK_NEAR(safe_speed(1.5, limit), 1.5, 1e-12);
  RC_CHECK_NEAR(safe_speed(-1.5, limit), -1.5, 1e-12);
  RC_CHECK_NEAR(safe_speed(limit, limit), limit, 1e-12);
  RC_CHECK(seen.empty());

  // Outside it: clamped, and reported, and the loop carries on.
  RC_CHECK_NEAR(safe_speed(9.0, limit), 2.0, 1e-12);
  RC_CHECK(seen.size() == 1u);

  RC_CHECK_NEAR(safe_speed(-9.0, limit), -2.0, 1e-12);
  RC_CHECK(seen.size() == 2u);

  std::cout << "  " << std::left << std::setw(16) << "requested" << std::setw(12)
            << "returned" << "reported\n" << std::right;
  const double requests[] = {1.5, 9.0, -9.0};
  for (double requested : requests) {
    const std::size_t before = seen.size();
    const double given = safe_speed(requested, limit);
    std::cout << "  " << std::left << std::setw(16) << requested << std::setw(12)
              << given << (seen.size() > before ? "yes" : "no") << "\n" << std::right;
  }
  std::cout << "\n  reporting and deciding are different jobs, and the clamp is\n";
  std::cout << "  the caller's because only the caller knows what is safe here\n\n";
}

RC_TEST("the handler is one object, and aborting is a choice not a default") {
  // The default is to report and carry on, because a robot that is moving does
  // not become safer by stopping its own control loop.
  RC_CHECK(current_check_handler() == &log_failure);

  {
    const HandlerFor recording(&record);
    RC_CHECK(current_check_handler() == &record);
    RC_CHECK(!RC_EXPECT(false, "provoked"));
    RC_CHECK(seen.size() == 1u);
  }

  // Restored by the guard, so one test cannot leave the next one blind.
  RC_CHECK(current_check_handler() == &log_failure);

  // A null handler would be a crash on the first failure, so it is refused and
  // replaced with the silent one.
  set_check_handler(nullptr);
  RC_CHECK(current_check_handler() == &ignore_failure);
  RC_CHECK(!RC_EXPECT(false, "no handler installed"));

  set_check_handler(&log_failure);
  RC_CHECK(current_check_handler() == &log_failure);

  std::cout << "  abort_on_failure exists and is not the default, because the\n";
  std::cout << "  right answer to a violated precondition is a decision\n\n";
}
