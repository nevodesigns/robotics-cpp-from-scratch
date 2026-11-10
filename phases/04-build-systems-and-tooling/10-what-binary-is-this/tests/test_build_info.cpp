#include <rc/test/rc_test.hpp>

#include <iostream>
#include <string>
#include <vector>

#include "solution.hpp"

namespace {

BuildInfo make(const char* compiler, int version, int standard, bool optimized, const char* sanitizer, int bits,
               bool little) {
  BuildInfo b;
  b.compiler = compiler;
  b.version = version;
  b.standard = standard;
  b.optimized = optimized;
  b.sanitizer = sanitizer;
  b.pointer_bits = bits;
  b.little_endian = little;
  return b;
}

bool same(const BuildInfo& a, const BuildInfo& b) {
  return a.compiler == b.compiler && a.version == b.version && a.standard == b.standard && a.optimized == b.optimized &&
         a.sanitizer == b.sanitizer && a.pointer_bits == b.pointer_bits && a.little_endian == b.little_endian;
}

}  // namespace

RC_TEST("a build is described in one line, and the line is read back as the same build") {
  const BuildInfo a = make("gcc", 11, 17, true, "none", 64, true);
  RC_CHECK_EQ(describe(a), std::string("gcc-11 c++17 release none 64bit little"));

  const BuildInfo all[] = {a,
                           make("clang", 14, 17, false, "address", 64, true),
                           make("msvc", 1938, 20, true, "none", 64, true),
                           make("gcc", 13, 17, false, "thread", 32, false),
                           make("clang", 18, 23, true, "none", 64, true)};
  int round_trips = 0;
  for (const BuildInfo& b : all) {
    BuildInfo back;
    if (parse_build(describe(b).c_str(), &back) && same(b, back)) ++round_trips;
  }
  std::cout << "  " << round_trips << " of " << sizeof(all) / sizeof(all[0]) << " builds written and read back unchanged\n";
  RC_CHECK_EQ(round_trips, 5);
}

RC_TEST("a line that is not exactly that format is refused, and leaves the output alone") {
  const char* refused[] = {"",
                           "gcc-11 c++17 release none 64bit",
                           "gcc-11 c++17 release none 64bit little extra",
                           "gcc-11  c++17 release none 64bit little",
                           "gcc c++17 release none 64bit little",
                           "icc-19 c++17 release none 64bit little",
                           "gcc-11 c17 release none 64bit little",
                           "gcc-11 c++17 fast none 64bit little",
                           "gcc-11 c++17 release memory 64bit little",
                           "gcc-11 c++17 release none 64 little",
                           "gcc-11 c++17 release none 64bit middle",
                           " gcc-11 c++17 release none 64bit little"};
  // A good line is accepted, so that refusing everything would not pass this test.
  BuildInfo good;
  RC_REQUIRE(parse_build("gcc-11 c++17 release none 64bit little", &good));
  RC_CHECK_EQ(good.version, 11);

  BuildInfo kept = make("sentinel", 1, 1, false, "none", 1, true);
  int count = 0;
  for (const char* text : refused) {
    if (!parse_build(text, &kept)) ++count;
  }
  std::cout << "  " << count << " of " << sizeof(refused) / sizeof(refused[0]) << " malformed lines refused\n\n";
  RC_CHECK_EQ(count, static_cast<int>(sizeof(refused) / sizeof(refused[0])));
  RC_CHECK_EQ(kept.compiler, std::string("sentinel"));
}

RC_TEST("speeds may be compared only between builds that are alike and neither carries a sanitizer") {
  const BuildInfo release = make("gcc", 11, 17, true, "none", 64, true);
  const BuildInfo release2 = make("gcc", 11, 17, true, "none", 64, true);
  const BuildInfo debug = make("gcc", 11, 17, false, "none", 64, true);
  const BuildInfo other_compiler = make("clang", 14, 17, true, "none", 64, true);
  const BuildInfo other_version = make("gcc", 13, 17, true, "none", 64, true);
  const BuildInfo asan = make("gcc", 11, 17, true, "address", 64, true);
  const BuildInfo small = make("gcc", 11, 17, true, "none", 32, true);

  RC_CHECK(comparable_for_speed(release, release2));
  RC_CHECK(!comparable_for_speed(release, debug));
  RC_CHECK(!comparable_for_speed(release, other_compiler));
  RC_CHECK(!comparable_for_speed(release, other_version));
  RC_CHECK(!comparable_for_speed(release, small));
  RC_CHECK(!comparable_for_speed(release, asan));
  RC_CHECK(!comparable_for_speed(asan, asan));         // the same sanitizer build is still not a speed to trust
}

RC_TEST("a build that is not fit to be timed says why, and a release build says nothing") {
  const std::vector<std::string> none = speed_warnings(make("gcc", 11, 17, true, "none", 64, true));
  const std::vector<std::string> debug = speed_warnings(make("gcc", 11, 17, false, "none", 64, true));
  const std::vector<std::string> both = speed_warnings(make("clang", 14, 17, false, "address", 64, true));
  std::cout << "  release: " << none.size() << " warnings; debug: " << debug.size() << "; debug with a sanitizer: "
            << both.size() << "\n";
  RC_CHECK_EQ(none.size(), static_cast<std::size_t>(0));
  RC_REQUIRE(debug.size() == 1);
  RC_CHECK(debug[0].find("optimised") != std::string::npos);
  RC_REQUIRE(both.size() == 2);
  RC_CHECK(both[1].find("address") != std::string::npos);
}

RC_TEST("this very build describes itself consistently") {
  const BuildInfo here = current_build();
  const std::string line = describe(here);
  std::cout << "  this binary: " << line << "\n";
  const std::vector<std::string> warnings = speed_warnings(here);
  for (const std::string& w : warnings) std::cout << "    warning: " << w << "\n";
  std::cout << "\n";

  // The pointer width is worked out from sizeof, the standard from a macro, and both are checked.
  RC_CHECK_EQ(here.pointer_bits, static_cast<int>(sizeof(void*) * 8));
  RC_CHECK(here.standard >= 17);
  RC_CHECK(here.compiler != "unknown");
  RC_CHECK(here.little_endian);          // every toolchain this curriculum claims runs on a little endian machine

  BuildInfo back;
  RC_REQUIRE(parse_build(line.c_str(), &back));
  RC_CHECK(same(here, back));
  RC_CHECK(comparable_for_speed(here, here) == (here.sanitizer == "none"));
}
