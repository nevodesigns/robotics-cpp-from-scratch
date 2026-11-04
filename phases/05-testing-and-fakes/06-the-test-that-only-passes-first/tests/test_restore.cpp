#include <rc/test/rc_test.hpp>

#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "solution.hpp"

namespace {

// The shared thing. A calibration offset every probe below reads, exactly as
// a real suite shares a configuration, a registry, a logger or a clock.
double calibration_offset = 0.0;

// Two probes that share it. Neither is wrong on its own.

// Needs the offset at its default, and leaves it at 5.
bool dirties_the_offset() {
  const bool held = calibration_offset == 0.0;
  calibration_offset = 5.0;
  return held;
}

// Needs the offset at its default, and leaves it alone.
bool expects_the_default() { return calibration_offset == 0.0; }

// The same two, written so neither can leave anything behind.
bool dirties_but_restores() {
  const Restore<double> keep(calibration_offset);
  const bool held = calibration_offset == 0.0;
  calibration_offset = 5.0;
  return held;
}

bool expects_the_default_too() { return calibration_offset == 0.0; }

// What the probes above share, put back to its starting state.
void reset_the_offset() { calibration_offset = 0.0; }

std::string verdict(bool ok) { return ok ? "passed" : "FAILED"; }

}  // namespace

RC_TEST("two tests that share a value disagree about which order they ran in") {
  const Restore<double> keep(calibration_offset, 0.0);

  // Forwards: the dirtying one runs first, so the second sees 5 and fails.
  calibration_offset = 0.0;
  const bool first_forwards = dirties_the_offset();
  const bool second_forwards = expects_the_default();

  // Backwards: the same two tests, the other way round, and both hold.
  calibration_offset = 0.0;
  const bool second_backwards = expects_the_default();
  const bool first_backwards = dirties_the_offset();

  std::cout << "\n  the same two tests, run both ways\n\n";
  std::cout << "  " << std::left << std::setw(16) << "order" << std::setw(22)
            << "dirties the offset" << "expects the default\n" << std::right;
  std::cout << "  " << std::left << std::setw(16) << "forwards" << std::setw(22)
            << verdict(first_forwards) << verdict(second_forwards) << "\n";
  std::cout << "  " << std::left << std::setw(16) << "backwards" << std::setw(22)
            << verdict(first_backwards) << verdict(second_backwards) << "\n\n"
            << std::right;

  // Every one of these is a correct test of its own behaviour.
  RC_CHECK(first_forwards);
  RC_CHECK(first_backwards);
  RC_CHECK(second_backwards);

  // And one of them fails, for a reason that is nowhere in it.
  RC_CHECK(!second_forwards);

  // And the guard that fixes it, on the same shared value.
  calibration_offset = 1.0;
  {
    const Restore<double> keep(calibration_offset, 123.0);
    RC_CHECK_NEAR(calibration_offset, 123.0, 1e-12);
  }
  RC_CHECK_NEAR(calibration_offset, 1.0, 1e-12);
  calibration_offset = 0.0;

  std::cout << "  nothing in the failing test is wrong. The reason it failed is\n";
  std::cout << "  in a different function, which ran before it.\n\n";
}

RC_TEST("the order detector finds it, and says nothing when there is nothing") {
  const Restore<double> keep(calibration_offset, 0.0);

  const OrderReport leaky = run_in_both_orders(
      {&dirties_the_offset, &expects_the_default}, &reset_the_offset);

  const OrderReport clean = run_in_both_orders(
      {&dirties_but_restores, &expects_the_default_too}, &reset_the_offset);

  // Without the reset, the backwards pass starts on whatever the forwards
  // pass left, which is the lesson's own subject biting its own tool. Shown
  // here rather than described, by doing it by hand.
  calibration_offset = 0.0;
  int unreset_forwards = 0;
  if (dirties_the_offset()) ++unreset_forwards;
  if (expects_the_default()) ++unreset_forwards;
  int unreset_backwards = 0;
  if (expects_the_default()) ++unreset_backwards;
  if (dirties_the_offset()) ++unreset_backwards;

  std::cout << "  " << std::left << std::setw(22) << "suite" << std::right
            << std::setw(12) << "forwards" << std::setw(12) << "backwards"
            << std::setw(18) << "order dependent" << "\n";
  const auto row = [](const char* name, const OrderReport& r) {
    std::cout << "  " << std::left << std::setw(22) << name << std::right
              << std::setw(12) << r.forwards_passed << std::setw(12)
              << r.backwards_passed << std::setw(18)
              << (r.order_dependent() ? "YES" : "no") << "\n";
  };
  row("sharing a value", leaky);
  row("restoring it", clean);
  std::cout << "\n  the same leaky pair with no reset between the passes: "
            << unreset_forwards << " forwards, " << unreset_backwards
            << " backwards\n\n";

  // The leaky pair passes a different number of tests depending on direction,
  // which is the signal. Which test is at fault is a separate question.
  RC_CHECK(leaky.order_dependent());
  RC_CHECK(leaky.forwards_passed == 1);
  RC_CHECK(leaky.backwards_passed == 2);
  RC_CHECK(!leaky.all_passed());

  // The restoring pair passes everything in both directions.
  RC_CHECK(!clean.order_dependent());
  RC_CHECK(clean.all_passed());
  RC_CHECK(clean.count == 2u);

  // Without a reset the backwards pass scores zero rather than two, because
  // it inherits the offset the forwards pass left at 5 and both probes then
  // fail, including the one that is fine. The numbers still differ, so the
  // detection happens to work and reports a figure that is not true.
  RC_CHECK(unreset_forwards == 1);
  RC_CHECK(unreset_backwards == 0);
  RC_CHECK(unreset_backwards != leaky.backwards_passed);

  std::cout << "  reversing the suite is one extra run and finds the common case,\n";
  std::cout << "  because leaked state flows forwards: reversing puts the victim\n";
  std::cout << "  in front of the culprit. The reset between the passes is not\n";
  std::cout << "  optional: without it the detector inherits its own mess.\n\n";
}

RC_TEST("the guard puts it back however the scope is left") {
  calibration_offset = 1.25;

  {
    const Restore<double> keep(calibration_offset);
    calibration_offset = 99.0;
    RC_CHECK_NEAR(calibration_offset, 99.0, 1e-12);
    RC_CHECK_NEAR(keep.saved(), 1.25, 1e-12);
  }
  RC_CHECK_NEAR(calibration_offset, 1.25, 1e-12);

  // Installing a value for the duration, which is the common shape: a test
  // that needs a particular configuration and must not impose it on the rest.
  {
    const Restore<double> keep(calibration_offset, 7.5);
    RC_CHECK_NEAR(calibration_offset, 7.5, 1e-12);
  }
  RC_CHECK_NEAR(calibration_offset, 1.25, 1e-12);

  // An early return is the path a save and restore pair gets wrong, and the
  // reason this is a destructor rather than two calls. Lesson 04-05 met the
  // same thing with a timing section.
  const auto gives_up = [](bool bail) {
    const Restore<double> keep(calibration_offset, 42.0);
    if (bail) return false;
    return true;
  };
  RC_CHECK(!gives_up(true));
  RC_CHECK_NEAR(calibration_offset, 1.25, 1e-12);
  RC_CHECK(gives_up(false));
  RC_CHECK_NEAR(calibration_offset, 1.25, 1e-12);

  calibration_offset = 0.0;
}

RC_TEST("nested guards unwind inside out") {
  const Restore<double> outer(calibration_offset, 1.0);
  RC_CHECK_NEAR(calibration_offset, 1.0, 1e-12);

  {
    const Restore<double> inner(calibration_offset, 2.0);
    RC_CHECK_NEAR(calibration_offset, 2.0, 1e-12);
    {
      const Restore<double> innermost(calibration_offset, 3.0);
      RC_CHECK_NEAR(calibration_offset, 3.0, 1e-12);
    }
    RC_CHECK_NEAR(calibration_offset, 2.0, 1e-12);
  }
  RC_CHECK_NEAR(calibration_offset, 1.0, 1e-12);

  // Each one remembers what it found rather than a global original, so three
  // of them nest without any of them knowing about the others.
  std::cout << "  three nested guards unwound to 2, then 1, each restoring what\n";
  std::cout << "  it personally found\n\n";
}

RC_TEST("it guards anything, not just a double") {
  std::string mode = "idle";
  {
    const Restore<std::string> keep(mode, "calibrating");
    RC_CHECK(mode == "calibrating");
  }
  RC_CHECK(mode == "idle");

  std::vector<double> window = {1.0, 2.0};
  {
    const Restore<std::vector<double>> keep(window);
    window.clear();
    window.push_back(99.0);
    RC_CHECK(window.size() == 1u);
  }
  RC_CHECK(window.size() == 2u);
  RC_CHECK_NEAR(window[1], 2.0, 1e-12);

  int count = 3;
  {
    const Restore<int> keep(count, 0);
    RC_CHECK(count == 0);
  }
  RC_CHECK(count == 3);

  std::cout << "  the same guard held a string, a vector and an int\n\n";
}
