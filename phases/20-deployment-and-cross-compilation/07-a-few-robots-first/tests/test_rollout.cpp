#include <rc/test/rc_test.hpp>

#include <cstdint>
#include <iomanip>
#include <iostream>

#include "solution.hpp"

namespace {

// A fleet of n robots with `bad` of them that the release fails on and `quiet` that do not report, spread through the fleet in a fixed pseudo-random order (the same on every machine, since it is integers only).
std::vector<FleetRobot> make_fleet(int n, int bad, int quiet, std::uint64_t seed) {
  std::vector<FleetRobot> fleet(static_cast<std::size_t>(n));
  for (int i = 0; i < bad; ++i) fleet[static_cast<std::size_t>(i)].fails = true;
  for (int i = 0; i < quiet; ++i) fleet[static_cast<std::size_t>(n - 1 - i)].silent = true;
  std::uint64_t s = seed;
  for (std::size_t i = fleet.size() - 1; i > 0; --i) {
    s = s * 6364136223846793005ULL + 1442695040888963407ULL;
    const std::size_t j = static_cast<std::size_t>((s >> 33) % (i + 1));
    std::swap(fleet[i], fleet[j]);
  }
  return fleet;
}

RolloutPolicy staged(double max_failure, double min_reported) {
  RolloutPolicy p;
  p.cumulative = {0.01, 0.1, 0.5, 1.0};
  p.max_failure_ratio = max_failure;
  p.min_reported_ratio = min_reported;
  return p;
}

RolloutPolicy all_at_once() {
  RolloutPolicy p;
  p.cumulative = {1.0};
  p.max_failure_ratio = 1.0;
  p.min_reported_ratio = 0.0;
  return p;
}

void line(const char* what, const RolloutResult& r) {
  std::cout << "  " << what << std::string(44 - std::string(what).size(), ' ') << std::setw(4) << r.attempted << " robots given it, " << std::setw(3) << r.failed << " failed, " << std::setw(3) << r.silent << " silent" << (r.halted ? ", halted at wave " + std::to_string(r.halted_at_wave) : "") << "\n";
}

}  // namespace

RC_TEST("the waves take the next robots in order, an empty wave is skipped, and a clean fleet goes all the way") {
  const std::vector<FleetRobot> fleet = make_fleet(1000, 0, 0, 1);
  const RolloutResult r = rollout(fleet, staged(0.05, 0.8));
  RC_CHECK(r.attempted == 1000 && r.failed == 0 && r.silent == 0);
  RC_CHECK(!r.halted && r.waves_done == 4 && r.halted_at_wave == -1);
  RolloutPolicy tiny;
  tiny.cumulative = {0.001, 0.002, 1.0};                  // the first two waves of a fleet of 10 are the same single robot
  const RolloutResult t = rollout(make_fleet(10, 0, 0, 1), tiny);
  RC_CHECK(t.attempted == 10 && t.waves_done == 2);        // 0.002 of 10 rounds up to 1, the same as the first wave: skipped
  RolloutPolicy never;
  never.cumulative = {0.5};
  const RolloutResult half = rollout(make_fleet(10, 0, 0, 1), never);
  RC_CHECK(half.attempted == 5 && !half.halted);
  RC_CHECK(rollout({}, staged(0.05, 0.8)).attempted == 0);
  RC_CHECK(rollout(make_fleet(10, 0, 0, 1), RolloutPolicy()).attempted == 0);
  std::cout << "  a clean fleet of 1000 in four waves of 10, 90, 400 and 500 robots: all given the release\n\n";
}

RC_TEST("a fault on three robots in ten is stopped by the first wave, where giving it to everyone fails three hundred") {
  const std::vector<FleetRobot> fleet = make_fleet(1000, 300, 0, 7);
  const RolloutResult all = rollout(fleet, all_at_once());
  const RolloutResult st = rollout(fleet, staged(0.05, 0.8));
  std::cout << "  a release that fails on 300 of 1000 robots:\n";
  line("all at once", all);
  line("four waves, halting above 5 percent", st);
  std::cout << "\n";
  RC_CHECK(all.attempted == 1000 && all.failed == 300 && !all.halted);
  RC_CHECK(st.halted && st.halted_at_wave == 0);
  RC_CHECK(st.attempted == 10);
  RC_CHECK(st.failed < 10 && st.failed >= 1);
}

RC_TEST("a fault on one robot in two hundred is not seen by a canary, and a threshold of five percent never halts for it") {
  const std::vector<FleetRobot> fleet = make_fleet(1000, 5, 0, 7);
  const RolloutResult lenient = rollout(fleet, staged(0.05, 0.8));
  const RolloutResult strict = rollout(fleet, staged(0.0, 0.8));
  std::cout << "  a release that fails on 5 of 1000 robots:\n";
  line("halting above 5 percent", lenient);
  line("halting on any failure", strict);
  std::cout << "\n  the chance that a wave includes at least one of them: ";
  const int sizes[] = {10, 100, 500, 598, 1000};
  for (int n : sizes) std::cout << n << " robots " << std::fixed << std::setprecision(1) << 100.0 * detection_probability(0.005, n) << "%  ";
  std::cout << "\n  robots needed for a 95 percent chance: " << robots_needed(0.005, 0.95) << "; for 99 percent: " << robots_needed(0.005, 0.99) << "\n\n";
  RC_CHECK(!lenient.halted && lenient.attempted == 1000 && lenient.failed == 5);
  RC_CHECK(strict.halted);
  RC_CHECK(strict.failed >= 1 && strict.failed <= 5);
  RC_CHECK(strict.attempted < 1000);
  RC_CHECK_NEAR(detection_probability(0.005, 10), 0.0489, 1e-3);
  RC_CHECK_NEAR(detection_probability(0.005, 100), 0.3942, 1e-3);
  RC_CHECK(robots_needed(0.005, 0.95) == 598);
  RC_CHECK(robots_needed(0.005, 0.99) == 919);
  RC_CHECK(detection_probability(0.005, 598) >= 0.95 && detection_probability(0.005, 597) < 0.95);
  RC_CHECK(robots_needed(0.3, 0.95) == 9);
}

RC_TEST("when too few robots report, there is no news and the rollout stops, and one that goes on regardless sends the release blind") {
  const std::vector<FleetRobot> fleet = make_fleet(1000, 0, 450, 11);          // 45 percent of the fleet is off or out of range
  const RolloutResult careful = rollout(fleet, staged(0.05, 0.8));
  const RolloutResult blind = rollout(fleet, staged(0.05, 0.0));
  std::cout << "  45 percent of the robots not reporting:\n";
  line("halting below 80 percent reporting", careful);
  line("not checking who reports", blind);
  std::cout << "\n";
  RC_CHECK(careful.halted);
  RC_CHECK(careful.attempted < 1000);
  RC_CHECK(!blind.halted && blind.attempted == 1000);
  RC_CHECK(blind.silent == 450);
}

RC_TEST("the arithmetic of a wave, and what cannot happen") {
  RC_CHECK(detection_probability(0.0, 100) == 0.0);
  RC_CHECK(detection_probability(0.5, 0) == 0.0);
  RC_CHECK(detection_probability(1.0, 1) == 1.0);
  RC_CHECK(detection_probability(2.0, 1) == 1.0);
  RC_CHECK(robots_needed(0.0, 0.95) == -1);
  RC_CHECK(robots_needed(0.1, 1.0) == -1);
  RC_CHECK(robots_needed(0.1, 0.0) == 0);
  RC_CHECK(robots_needed(1.0, 0.99) == 1);
  RC_CHECK(robots_needed(0.5, 0.5) == 1);
}
