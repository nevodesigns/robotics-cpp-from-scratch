#include <rc/test/rc_test.hpp>

#include <iomanip>
#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

#include "solution.hpp"

namespace {

// The two ways to give a member its first value, written as everyone writes
// them. Both are correct. They are not the same work.
struct ByList {
  Tracked first_;
  Tracked second_;
  ByList(const Tracked& first, const Tracked& second) : first_(first), second_(second) {}
};

struct ByBody {
  Tracked first_;
  Tracked second_;
  ByBody(const Tracked& first, const Tracked& second) {
    first_ = first;
    second_ = second;
  }
};

// A member that says when it was built.
std::vector<std::string> g_construction_log;
struct Noisy {
  explicit Noisy(const char* name) { g_construction_log.push_back(name); }
};

// gcc and clang both warn that the list below is in the wrong order, with
// -Wreorder, and they are right to. The suite writes it that way on purpose
// and says so, because the warning is the lesson's second measurement.
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreorder"
#endif
struct Reversed {
  Noisy first_;
  Noisy second_;
  Reversed() : second_("second"), first_("first") {}
};
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

// A constructor that takes one number, without the word explicit.
struct Loose {
  double value;
  Loose(double v) : value(v) {}
};
int whole_part(Loose loose) { return static_cast<int>(loose.value); }

// Declares a constructor and nothing else, and the same again with the default
// asked for.
struct NoDefault {
  explicit NoDefault(int) {}
};
struct AskedFor {
  AskedFor() = default;
  explicit AskedFor(int) {}
};

}  // namespace

RC_TEST("building a member in the list is one step and assigning it in the body is two") {
  const Tracked offset(0.5);
  const Tracked scale(2.0);

  tracked_counts() = TrackedCounts{};
  { const ByList list(offset, scale); (void)list; }
  const TrackedCounts listed = tracked_counts();

  tracked_counts() = TrackedCounts{};
  { const ByBody body(offset, scale); (void)body; }
  const TrackedCounts assigned = tracked_counts();

  std::cout << "  two members, set two ways, counted:\n\n";
  std::cout << "                          built blank   built as a copy   overwritten\n";
  std::cout << "    in the list           " << std::setw(11) << listed.defaults << std::setw(18)
            << listed.copies << std::setw(14) << listed.assigns << "\n";
  std::cout << "    in the body           " << std::setw(11) << assigned.defaults << std::setw(18)
            << assigned.copies << std::setw(14) << assigned.assigns << "\n\n";

  RC_CHECK_EQ(listed.defaults, 0);
  RC_CHECK_EQ(listed.copies, 2);
  RC_CHECK_EQ(listed.assigns, 0);
  RC_CHECK_EQ(assigned.defaults, 2);
  RC_CHECK_EQ(assigned.copies, 0);
  RC_CHECK_EQ(assigned.assigns, 2);

  // The learner's constructor is held to the cheap one.
  tracked_counts() = TrackedCounts{};
  const Calibration calibration(offset, scale);
  const TrackedCounts made = tracked_counts();
  std::cout << "  Calibration, yours:     " << std::setw(11) << made.defaults << std::setw(18)
            << made.copies << std::setw(14) << made.assigns << "\n\n";
  RC_CHECK_EQ(made.defaults, 0);
  RC_CHECK_EQ(made.copies, 2);
  RC_CHECK_EQ(made.assigns, 0);

  // And it does what it is for: subtract the offset, then scale.
  RC_CHECK_NEAR(calibration.apply(1.5), 2.0, 1e-12);
  RC_CHECK_NEAR(calibration.apply(0.5), 0.0, 1e-12);
  RC_CHECK_NEAR(calibration.apply(-0.5), -2.0, 1e-12);
}

RC_TEST("members are built in the order they are declared, whatever order the list is in") {
  g_construction_log.clear();
  const Reversed reversed;
  (void)reversed;

  std::cout << "  the list was written second_, first_. built, in order:";
  for (const std::string& name : g_construction_log) std::cout << " " << name;
  std::cout << "\n\n";
  std::cout << "  gcc 11 and clang 14 both warn about that list with -Wreorder, and\n";
  std::cout << "  the warning is right: whatever the list says, the declaration wins.\n\n";

  RC_REQUIRE_EQ(g_construction_log.size(), 2u);
  RC_CHECK_EQ(g_construction_log[0], std::string("first"));
  RC_CHECK_EQ(g_construction_log[1], std::string("second"));

  // The learner's class, held to the same fact. Calibration declares its offset
  // before its scale, so the offset is copied first however the list is written.
  tracked_counts() = TrackedCounts{};
  const Tracked offset(0.5);
  const Tracked scale(2.0);
  const Calibration calibration(offset, scale);
  (void)calibration;
  const std::vector<double> copied = tracked_counts().copied_values;
  RC_REQUIRE_EQ(copied.size(), 2u);
  RC_CHECK_NEAR(copied[0], 0.5, 1e-12);
  RC_CHECK_NEAR(copied[1], 2.0, 1e-12);
}

RC_TEST("a constructor taking one value converts silently, unless it is explicit") {
  // Without the word, any number is a Loose.
  RC_CHECK_EQ(whole_part(7), 7);
  RC_CHECK((std::is_convertible<double, Loose>::value));

  std::cout << "  whole_part(7) with no explicit: " << whole_part(7)
            << ", the 7 became a Loose without being asked\n";
  std::cout << "  is a double convertible to Loose   "
            << (std::is_convertible<double, Loose>::value ? "yes" : "no") << "\n";
  std::cout << "  is a double convertible to Meters  "
            << (std::is_convertible<double, Meters>::value ? "yes" : "no") << "\n";
  std::cout << "  can a Meters be made from a double "
            << (std::is_constructible<Meters, double>::value ? "yes" : "no") << "\n\n";

  // The learner's Meters can be made on purpose and not by accident.
  RC_CHECK((std::is_constructible<Meters, double>::value));
  RC_CHECK(!(std::is_convertible<double, Meters>::value));

  // It does its job.
  RC_CHECK_NEAR(Meters(2.5).value(), 2.5, 1e-12);
  RC_CHECK_NEAR(to_feet(Meters(1.0)), 3.28084, 1e-9);
  RC_CHECK_NEAR(to_feet(Meters(0.0)), 0.0, 1e-12);
  RC_CHECK_NEAR(to_feet(Meters(-2.0)), -6.56168, 1e-9);
}

RC_TEST("declaring a constructor takes the default one away, unless you ask for it back") {
  std::cout << "  default constructible, when the class declares:\n";
  std::cout << "    only a constructor taking an int      "
            << (std::is_default_constructible<NoDefault>::value ? "yes" : "no") << "\n";
  std::cout << "    that, and = default                   "
            << (std::is_default_constructible<AskedFor>::value ? "yes" : "no") << "\n";
  std::cout << "    Meters, which takes one value         "
            << (std::is_default_constructible<Meters>::value ? "yes" : "no") << "\n";
  std::cout << "    Ramp, which asks for it back          "
            << (std::is_default_constructible<Ramp>::value ? "yes" : "no") << "\n\n";

  RC_CHECK(!(std::is_default_constructible<NoDefault>::value));
  RC_CHECK((std::is_default_constructible<AskedFor>::value));

  // A distance with no value is not a distance.
  RC_CHECK(!(std::is_default_constructible<Meters>::value));
  // A ramp that has not been given a target is a ramp that is not going anywhere.
  RC_CHECK((std::is_default_constructible<Ramp>::value));

  // Asking for the default back is only worth anything if it starts somewhere
  // true. A stub that is default constructible and starts at the wrong place
  // would otherwise pass this test.
  const Ramp idle;
  RC_CHECK_NEAR(idle.current(), 0.0, 1e-12);
  RC_CHECK_NEAR(idle.target(), 0.0, 1e-12);
  RC_CHECK_NEAR(idle.step(), 1.0, 1e-12);
}

RC_TEST("a member's default is used by every constructor that does not set it") {
  const Ramp idle;
  RC_CHECK_NEAR(idle.current(), 0.0, 1e-12);
  RC_CHECK_NEAR(idle.target(), 0.0, 1e-12);
  RC_CHECK_NEAR(idle.step(), 1.0, 1e-12);

  // One value sets the target and leaves the rest at their defaults.
  const Ramp to_ten(10.0);
  RC_CHECK_NEAR(to_ten.current(), 0.0, 1e-12);
  RC_CHECK_NEAR(to_ten.target(), 10.0, 1e-12);
  RC_CHECK_NEAR(to_ten.step(), 1.0, 1e-12);

  // Two values set the target and the step.
  const Ramp fast(10.0, 4.0);
  RC_CHECK_NEAR(fast.current(), 0.0, 1e-12);
  RC_CHECK_NEAR(fast.target(), 10.0, 1e-12);
  RC_CHECK_NEAR(fast.step(), 4.0, 1e-12);

  // A constructor taking one value is explicit for the same reason Meters is.
  RC_CHECK(!(std::is_convertible<double, Ramp>::value));

  // It behaves as a ramp: toward the target, one step at a time, and never past.
  Ramp ramp(10.0, 4.0);
  RC_CHECK_NEAR(ramp.advance(), 4.0, 1e-12);
  RC_CHECK_NEAR(ramp.advance(), 8.0, 1e-12);
  RC_CHECK_NEAR(ramp.advance(), 10.0, 1e-12);
  RC_CHECK_NEAR(ramp.advance(), 10.0, 1e-12);

  Ramp down(-6.0, 4.0);
  RC_CHECK_NEAR(down.advance(), -4.0, 1e-12);
  RC_CHECK_NEAR(down.advance(), -6.0, 1e-12);
}
