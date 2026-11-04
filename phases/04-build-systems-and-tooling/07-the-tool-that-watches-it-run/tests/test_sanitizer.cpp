#include <rc/test/rc_test.hpp>

#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

#include "solution.hpp"

// File scope, per audit rule L022.
constexpr bool kAddressOn = address_sanitizer_on();
constexpr bool kTrustAllocator = allocator_measurements_trustworthy();
constexpr bool kUbsanDetectable = undefined_sanitizer_detectable();

// At file scope, not inside the test, because audit rule L022 says so and
// caught this exact line. That is the sixth time in this repository.
constexpr int kBranchesCompiledIn = allocator_measurements_trustworthy() ? 1 : 2;

// The suite states its own expectation, independently of the header, so that
// a detection which is simply wrong is caught rather than agreed with. This
// is deliberate duplication: without it, a header that always answered false
// would pass every test in a build that has no sanitizer, which is the build
// a learner runs by hand.
// The nesting is not style. Written as one expression,
//
//   #if defined(__has_feature) && __has_feature(address_sanitizer)
//
// gcc refuses it with "missing binary operator before token (", because the
// preprocessor parses the whole expression before evaluating it and the && does
// not protect the right hand side. An unknown identifier becomes 0; an unknown
// identifier followed by a parenthesis is a syntax error. So the test for
// whether the facility exists has to be a separate, outer #if.
#define RC_EXPECT_ASAN 0
#if defined(__has_feature)
#  if __has_feature(address_sanitizer)
#    undef RC_EXPECT_ASAN
#    define RC_EXPECT_ASAN 1
#  endif
#endif
#if defined(__SANITIZE_ADDRESS__)
#  undef RC_EXPECT_ASAN
#  define RC_EXPECT_ASAN 1
#endif
constexpr bool kExpectedAddress = RC_EXPECT_ASAN == 1;

namespace {

// Reading unsigned char from storage nobody wrote is defined, which is what
// made lesson 02-09's heap measurement legal to take at all.
std::size_t bytes_matching(const unsigned char* data, std::size_t count, unsigned char value) {
  std::size_t matched = 0;
  for (std::size_t i = 0; i < count; ++i)
    if (data[i] == value) ++matched;
  return matched;
}

}  // namespace

RC_TEST("the answer is fixed when the program is built, not when it runs") {
  // That these compile at all is the proof: a constexpr variable may only be
  // initialised by a constant expression, so each of these was decided by the
  // compiler. A runtime probe could not be used this way.
  static_assert(kAddressOn || !kAddressOn, "it has an answer either way");
  static_assert(!kUbsanDetectable, "nobody defines a macro for it");

  // The two agree by construction, and the test says so rather than trusting
  // that nobody will later change one without the other.
  RC_CHECK(kTrustAllocator == !kAddressOn);
  RC_CHECK(!kUbsanDetectable);

  // Calling twice gives the same answer. A detection with a side effect, or
  // one that read an environment variable, would not.
  RC_CHECK(address_sanitizer_on() == kExpectedAddress);
  RC_CHECK(address_sanitizer_on() == kAddressOn);
  RC_CHECK(address_sanitizer_on() == address_sanitizer_on());

  std::cout << "  this build was compiled with: " << sanitizer_name() << "\n\n";
  std::cout << "    address sanitizer detected       "
            << (kAddressOn ? "yes" : "no") << "\n";
  std::cout << "    undefined sanitizer detectable   "
            << (kUbsanDetectable ? "yes" : "no") << "\n";
  std::cout << "    allocator measurements trusted   "
            << (kTrustAllocator ? "yes" : "no") << "\n\n";
  std::cout << "  gcc defines __SANITIZE_ADDRESS__ and has no __has_feature.\n";
  std::cout << "  clang has __has_feature(address_sanitizer) and does not\n";
  std::cout << "  define the macro. Neither defines anything at all for the\n";
  std::cout << "  undefined behaviour sanitizer, so the second row is not a\n";
  std::cout << "  claim that it is off. It is a refusal to guess.\n\n";
}

RC_TEST("the sanitizer replaces the allocator, which changes what a test can claim") {
  const std::size_t size = 32;
  unsigned char* first = new unsigned char[size];
  for (std::size_t i = 0; i < size; ++i) first[i] = 0x7e;
  delete[] first;

  unsigned char* second = new unsigned char[size];
  const std::size_t recovered = bytes_matching(second, size, 0x7e);
  delete[] second;

  std::cout << "  bytes of " << size << " that came back holding the pattern: "
            << recovered << "\n\n";

  if (kTrustAllocator) {
    // The ordinary allocator hands the block straight back. Lesson 02-09
    // measured 16 of 32 here, the other 16 being the allocator's own
    // bookkeeping for the freed block. Not asserted, because it is a fact
    // about one allocator and not about C++.
    std::cout << "  no address sanitizer, so this is the real allocator and\n";
    std::cout << "  the number means something. 02-09 measured 16 of 32.\n\n";
  } else {
    // Under the sanitizer the freed block goes into a quarantine and the
    // memory is poisoned, so nothing comes back. This is asserted, because
    // it is the whole point: the instrumentation is not a passive observer.
    RC_CHECK(recovered == 0u);
    std::cout << "  the address sanitizer is on, so the freed block went into\n";
    std::cout << "  a quarantine and nothing came back. Measured 0 of 32.\n\n";
    std::cout << "  this is why a test must say which build it is in. Both\n";
    std::cout << "  answers are correct and only one of them is about C++.\n\n";
  }

  // True in both builds, which is what makes it worth asserting.
  RC_CHECK(recovered <= size);
}

RC_TEST("a guard the compiler removes, rather than a branch it takes") {
  // Because the detection is constexpr, a branch on it costs nothing at
  // runtime: the compiler knows which way it goes and keeps one side. That is
  // what makes it reasonable to leave these guards in a shipped test suite
  // instead of maintaining two of them.
  int taken = 0;
  if (allocator_measurements_trustworthy()) ++taken; else ++taken;
  RC_CHECK(taken == 1);

  // The same decision, made at compile time, usable where only a constant
  // will do. kBranchesCompiledIn at the top of this file could not have been
  // written at all if the detection were a runtime probe.
  RC_CHECK(kBranchesCompiledIn == (kTrustAllocator ? 1 : 2));

  std::cout << "  the guard is a compile time constant, so a suite can carry\n";
  std::cout << "  both paths and pay for neither\n\n";
}

RC_TEST("which tool finds which defect, exhaustively") {
  // Five defects, five answers, every one of them measured for this lesson
  // rather than looked up. The whole table is checked because there are only
  // five rows and a sampled test of five things is a choice to know less for
  // nothing.
  RC_CHECK(std::string(finds(Defect::heap_overflow)) == "address");
  RC_CHECK(std::string(finds(Defect::use_after_free)) == "address");
  RC_CHECK(std::string(finds(Defect::signed_overflow)) == "undefined");
  RC_CHECK(std::string(finds(Defect::uninitialised_read)) == "neither");
  RC_CHECK(std::string(finds(Defect::data_race)) == "thread");

  // The pair this curriculum's lane builds with catches three of the five.
  RC_CHECK(found_by_the_lane(Defect::heap_overflow));
  RC_CHECK(found_by_the_lane(Defect::use_after_free));
  RC_CHECK(found_by_the_lane(Defect::signed_overflow));
  RC_CHECK(!found_by_the_lane(Defect::uninitialised_read));
  RC_CHECK(!found_by_the_lane(Defect::data_race));

  const Defect all[] = {Defect::heap_overflow, Defect::use_after_free,
                        Defect::signed_overflow, Defect::uninitialised_read,
                        Defect::data_race};
  const char* names[] = {"heap overflow", "use after free", "signed overflow",
                         "uninitialised read", "data race"};

  std::cout << "  defect                found by      in this lane\n";
  int covered = 0;
  for (std::size_t i = 0; i < 5; ++i) {
    if (found_by_the_lane(all[i])) ++covered;
    std::cout << "  " << std::string(names[i]) << std::string(22 - std::string(names[i]).size(), ' ')
              << std::string(finds(all[i])) << std::string(14 - std::string(finds(all[i])).size(), ' ')
              << (found_by_the_lane(all[i]) ? "yes" : "no") << "\n";
  }
  RC_CHECK(covered == 3);

  std::cout << "\n  " << covered << " of 5. The two the lane misses are the two this\n";
  std::cout << "  curriculum has measured most carefully, which is not a\n";
  std::cout << "  coincidence: a defect no tool reports is one you have to\n";
  std::cout << "  design against instead, which is what 02-09 and 03-11 did.\n\n";
  std::cout << "  and a sanitizer only ever reports what the test actually\n";
  std::cout << "  ran. The lane is as good as the suite's coverage, which is\n";
  std::cout << "  why this one runs over every reference test rather than a\n";
  std::cout << "  smoke test.\n\n";
}
