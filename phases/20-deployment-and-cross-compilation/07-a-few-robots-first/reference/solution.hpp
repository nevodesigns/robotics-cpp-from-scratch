#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <cstddef>
#include <vector>

// One robot of the fleet, as the release will meet it: whether this release fails on it (a hardware variant, a sensor, a firmware version) and whether it reports back at all (it may be off, out of range, or its link may be down).
struct FleetRobot {
  bool fails = false;
  bool silent = false;
};

// When to go on. `cumulative` is the share of the fleet that has the release after each wave (for example 0.01, 0.1, 0.5, 1.0) and must end at 1.0 to reach everyone.
struct RolloutPolicy {
  std::vector<double> cumulative;
  double max_failure_ratio = 0.05;     // halt if more than this share of the robots that reported in a wave failed
  double min_reported_ratio = 0.8;     // halt if fewer than this share of the wave reported at all
};

struct RolloutResult {
  int attempted = 0;                   // robots that were given the release
  int failed = 0;                      // of those, the ones that failed and reported it
  int silent = 0;                      // of those, the ones that said nothing
  bool halted = false;
  int waves_done = 0;                  // waves that completed and passed the checks
  int halted_at_wave = -1;             // the wave (from 0) that stopped it, or -1
};

// Give the release to the fleet in waves. Wave w takes the robots from where the last stopped up to ceil(cumulative[w] * fleet size) (not past the end, and an empty wave is skipped). Robots are taken in the order of `fleet`.
// A robot that is `silent` reports nothing; otherwise it reports failure if `fails`, and success if not. After each wave, with n robots given the release and r of them reported: if r < min_reported_ratio * n the rollout halts (there is not enough news to say it is safe);
// otherwise if the failed share of r is more than max_failure_ratio it halts. A halted rollout gives no more robots the release. The wave that halted it counts in attempted, failed and silent but not in waves_done.
inline RolloutResult rollout(const std::vector<FleetRobot>& fleet, const RolloutPolicy& policy) {
  RolloutResult out;
  std::size_t next = 0;
  for (std::size_t w = 0; w < policy.cumulative.size(); ++w) {
    double want = std::ceil(policy.cumulative[w] * static_cast<double>(fleet.size()));
    if (want > static_cast<double>(fleet.size())) want = static_cast<double>(fleet.size());
    const std::size_t end = want < 0.0 ? 0 : static_cast<std::size_t>(want);
    if (end <= next) continue;
    int n = 0;
    int failed = 0;
    int silent = 0;
    for (; next < end; ++next) {
      ++n;
      if (fleet[next].silent) ++silent;
      else if (fleet[next].fails) ++failed;
    }
    out.attempted += n;
    out.failed += failed;
    out.silent += silent;
    const int reported = n - silent;
    const bool too_quiet = static_cast<double>(reported) < policy.min_reported_ratio * static_cast<double>(n);
    const bool too_many = reported > 0 && static_cast<double>(failed) > policy.max_failure_ratio * static_cast<double>(reported);
    if (too_quiet || too_many) {
      out.halted = true;
      out.halted_at_wave = static_cast<int>(w);
      return out;
    }
    ++out.waves_done;
  }
  return out;
}

// The chance that a wave of n robots includes at least one that the release fails on, if a fraction p of the fleet is like that: 1 - (1 - p)^n. Zero for p or n at or below zero, and one for p of 1 or more.
inline double detection_probability(double p, int n) {
  if (p <= 0.0 || n <= 0) return 0.0;
  if (p >= 1.0) return 1.0;
  return 1.0 - std::pow(1.0 - p, static_cast<double>(n));
}

// How many robots must have the release for the chance of at least one failure to reach `confidence`: the smallest n with 1 - (1 - p)^n >= confidence, which is ceil(ln(1 - confidence) / ln(1 - p)).
// 0 for a confidence of zero or less; 1 for p of 1 or more; and -1 where it can never happen: p at or below zero, or a confidence of 1 or more.
inline int robots_needed(double p, double confidence) {
  if (confidence <= 0.0) return 0;
  if (p >= 1.0) return 1;
  if (p <= 0.0 || confidence >= 1.0) return -1;
  return static_cast<int>(std::ceil(std::log(1.0 - confidence) / std::log(1.0 - p)));
}

#endif  // LESSON_SOLUTION_HPP
