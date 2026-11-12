#include <rc/test/rc_test.hpp>

#include <iomanip>
#include <iostream>

#include "solution.hpp"

namespace {

Board pi() {
  Board b;
  b.name = "small board";
  b.cores = 4.0;
  b.ram_mb = 4096.0;
  b.flash_mb = 16384.0;
  b.throttle_factor = 0.6;
  return b;
}

Proc proc(const char* name, double avg, double peak, double ram) {
  Proc p;
  p.name = name;
  p.cpu_avg = avg;
  p.cpu_peak = peak;
  p.ram_mb = ram;
  return p;
}

std::vector<Proc> robot() {
  return {proc("perception", 1.2, 2.0, 900), proc("planner", 0.6, 1.5, 500), proc("drivers", 0.3, 0.5, 150), proc("bridge", 0.2, 0.4, 100), proc("logger", 0.1, 0.2, 80), proc("system", 0.2, 0.3, 300)};
}

std::string names(const Fit& f) {
  std::string s;
  for (const Finding& x : f.findings) s += (s.empty() ? "" : ", ") + x.resource;
  return s.empty() ? "none" : s;
}

}  // namespace

RC_TEST("a percentile is the nearest rank, and the profile keeps the average, the peak and the largest memory") {
  const std::vector<double> v = {5, 1, 4, 2, 3};
  RC_CHECK(percentile(v, 1.0) == 5.0);
  RC_CHECK(percentile(v, 0.5) == 3.0);
  RC_CHECK(percentile(v, 0.2) == 1.0);
  RC_CHECK(percentile(v, 0.0) == 1.0);
  RC_CHECK(percentile(v, 7.0) == 5.0);
  RC_CHECK(percentile({}, 0.5) == 0.0);
  const Proc p = profile_from_samples("x", {1.0, 2.0, 3.0, 6.0}, {100, 300, 200}, 1.0);
  RC_CHECK(p.name == "x");
  RC_CHECK_NEAR(p.cpu_avg, 3.0, 1e-12);
  RC_CHECK(p.cpu_peak == 6.0);
  RC_CHECK(p.ram_mb == 300.0);
  const Proc none = profile_from_samples("y", {}, {}, 1.0);
  RC_CHECK(none.cpu_avg == 0.0 && none.cpu_peak == 0.0 && none.ram_mb == 0.0);
}

RC_TEST("a minute of measuring on the bench misses the peak that an hour in the field has, and the planner's peak looks a fifth of its size") {
  // The planner uses 0.4 cores while it waits, and 2.0 for a quarter of a second when it replans, which happens once a minute or so.
  std::vector<double> hour;
  for (int i = 0; i < 3600; ++i) hour.push_back((i % 70) < 1 ? 2.0 : 0.4);        // one second sample, one in 70 is a replan
  std::vector<double> minute(hour.begin() + 1, hour.begin() + 61);                  // sixty seconds that happen to have none
  const Proc bench = profile_from_samples("planner (bench, 60 s)", minute, {500}, 1.0);
  const Proc field = profile_from_samples("planner (field, 1 h)", hour, {500}, 1.0);
  std::cout << "  the planner: " << bench.name << " average " << std::fixed << std::setprecision(2) << bench.cpu_avg << " peak " << bench.cpu_peak << "; " << field.name << " average " << field.cpu_avg << " peak " << field.cpu_peak << "\n";
  std::cout << "  the 99th percentile in the field: " << profile_from_samples("p", hour, {500}, 0.99).cpu_peak << "; the largest: " << field.cpu_peak << "\n\n";
  RC_CHECK_NEAR(bench.cpu_peak, 0.4, 1e-12);
  RC_CHECK_NEAR(field.cpu_peak, 2.0, 1e-12);
  RC_CHECK(field.cpu_avg > bench.cpu_avg);
  RC_CHECK(field.cpu_peak >= 5.0 * bench.cpu_peak);
}

RC_TEST("six programs fit a four core board with headroom, and the numbers are what is left") {
  const Fit f = check_fit(pi(), robot(), 3000.0, 2, 2000.0, 200.0, Budget(), false);
  std::cout << "  six programs on a board of 4 cores, 4096 MB of memory and 16 GB of flash:\n";
  std::cout << "  CPU average " << std::fixed << std::setprecision(2) << f.cpu_avg << " of 2.80 allowed; peaks together " << f.cpu_peak << " of 5.00; memory " << std::setprecision(0) << f.ram_mb << " MB of 3277; flash " << f.flash_mb << " MB of 14746\n";
  std::cout << "  fits: " << (f.fits ? "yes" : "no") << ", over: " << names(f) << "\n\n";
  RC_CHECK_NEAR(f.cpu_avg, 2.6, 1e-9);
  RC_CHECK_NEAR(f.cpu_peak, 4.9, 1e-9);
  RC_CHECK_NEAR(f.ram_mb, 2030.0, 1e-9);
  RC_CHECK_NEAR(f.flash_mb, 8200.0, 1e-9);
  RC_CHECK(f.fits && f.findings.empty());
}

RC_TEST("in a hot enclosure the board runs at 60 percent, and the same robot no longer fits") {
  const Fit cool = check_fit(pi(), robot(), 3000.0, 2, 2000.0, 200.0, Budget(), false);
  const Fit hot = check_fit(pi(), robot(), 3000.0, 2, 2000.0, 200.0, Budget(), true);
  std::cout << "  cool: fits " << (cool.fits ? "yes" : "no") << "; throttled to 60 percent (2.4 cores): over " << names(hot) << " (average " << std::fixed << std::setprecision(2) << hot.findings[0].used << " against " << hot.findings[0].allowed << ")\n\n";
  RC_CHECK(cool.fits);
  RC_CHECK(!hot.fits);
  RC_CHECK(hot.findings.size() == 2);
  if (hot.findings.size() != 2) return;
  RC_CHECK(hot.findings[0].resource == "cpu_avg" && hot.findings[1].resource == "cpu_peak");
  RC_CHECK_NEAR(hot.findings[0].allowed, 0.7 * 2.4, 1e-9);
  RC_CHECK_NEAR(hot.findings[1].allowed, 1.25 * 2.4, 1e-9);
}

RC_TEST("adding a map server of 1500 MB pushes memory over, and each over-budget resource is named with what was used and what was allowed") {
  std::vector<Proc> big = robot();
  big.push_back(proc("map server", 0.05, 0.05, 1500));
  const Fit f = check_fit(pi(), big, 3000.0, 2, 2000.0, 200.0, Budget(), false);
  std::cout << "  with a map server: over " << names(f);
  RC_CHECK(!f.fits);
  RC_CHECK(f.findings.size() == 1);
  if (f.findings.size() != 1) return;
  RC_CHECK(f.findings[0].resource == "ram");
  RC_CHECK_NEAR(f.findings[0].used, 3530.0, 1e-9);
  RC_CHECK_NEAR(f.findings[0].allowed, 0.8 * 4096.0, 1e-9);
  std::cout << " (" << std::setprecision(0) << f.findings[0].used << " MB used, " << f.findings[0].allowed << " allowed)\n\n";
}

RC_TEST("two slots halve the release that fits, and a release that is too big for them is named before it is built") {
  const Board b = pi();
  const double two = max_release_mb(b, 2, 2000.0, 200.0, Budget());
  const double one = max_release_mb(b, 1, 2000.0, 200.0, Budget());
  std::cout << "  16 GB of flash, 2 GB of data, 200 MB of logs: the largest release is " << std::fixed << std::setprecision(0) << two << " MB with two slots and " << one << " MB with one\n";
  RC_CHECK_NEAR(two, (0.9 * 16384.0 - 2200.0) / 2.0, 1e-9);
  RC_CHECK_NEAR(one, 0.9 * 16384.0 - 2200.0, 1e-9);
  const Fit over = check_fit(b, robot(), two + 100.0, 2, 2000.0, 200.0, Budget(), false);
  RC_CHECK(!over.fits && over.findings.back().resource == "flash");
  RC_CHECK(check_fit(b, robot(), two - 1.0, 2, 2000.0, 200.0, Budget(), false).fits);
  RC_CHECK(max_release_mb(b, 0, 2000.0, 200.0, Budget()) == 0.0);
  RC_CHECK(max_release_mb(b, 2, 20000.0, 200.0, Budget()) == 0.0);
  std::cout << "  a release 100 MB past that: flash is over; 1 MB under it: fits\n\n";
}
