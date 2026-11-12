// rc/safety/selftest.hpp
//
// A self-test, a permit and a test interval, from lesson 19-11, graduated.
//
// SelfTest runs every check in the order added and reports each as a CheckResult: a check that returns false, throws or has no function in it fails, and a failure does not hide the next. all_passed is false for an empty list, since a test that checked nothing has not passed. Permit allows driving only after a passing test, until the interval ends, and not after a failed test or if the clock has gone backwards.
// failure_probability is 1 - exp(-rate * hours) and max_interval the longest interval that keeps it at or below a target.
//
// With a failure rate of 1e-5 per hour the chance of an undetected failure was 1.00e-05 after an hour, 2.40e-04 after a day, 1.68e-03 after a week and 8.39e-02 after a year; to keep it at 1 in 1000 the test was due every 100.1 hours. A welded contactor passed a check of the software's command and failed the check that read the contactor.

#ifndef RC_SAFETY_SELFTEST_HPP
#define RC_SAFETY_SELFTEST_HPP

#include <cmath>
#include <cstddef>
#include <functional>
#include <string>
#include <vector>

namespace rc {
namespace safety {

struct CheckResult {
  std::string name;
  bool passed = false;
  std::string detail;
};

// A list of named checks that is run before the robot may drive.
class SelfTest {
 public:
  void add(const std::string& name, std::function<bool()> check) { checks_.push_back({name, std::move(check)}); }

  std::size_t size() const { return checks_.size(); }

  // Run EVERY check, in the order they were added, and report each: the first failure does not hide the rest. A check that throws has failed (detail "threw an exception"), a check that returns false has failed
  // (detail "returned false"), and a check with no function in it has failed too (detail "no check").
  std::vector<CheckResult> run() const {
    std::vector<CheckResult> out;
    for (const Entry& e : checks_) {
      CheckResult r;
      r.name = e.name;
      if (!e.fn) {
        r.detail = "no check";
      } else {
        try {
          r.passed = e.fn();
          if (!r.passed) r.detail = "returned false";
        } catch (...) {
          r.passed = false;
          r.detail = "threw an exception";
        }
      }
      out.push_back(r);
    }
    return out;
  }

 private:
  struct Entry {
    std::string name;
    std::function<bool()> fn;
  };
  std::vector<Entry> checks_;
};

// Did everything pass? A test that checked nothing has not passed: an empty list is false.
inline bool all_passed(const std::vector<CheckResult>& results) {
  if (results.empty()) return false;
  for (const CheckResult& r : results)
    if (!r.passed) return false;
  return true;
}

inline std::vector<std::string> failed_names(const std::vector<CheckResult>& results) {
  std::vector<std::string> names;
  for (const CheckResult& r : results)
    if (!r.passed) names.push_back(r.name);
  return names;
}

// Permission to drive, which lasts for an interval after a test that passed. It is not given before any test, not after one that failed (even if an earlier one passed a moment ago), not after the interval, and not if the clock has gone backwards.
class Permit {
 public:
  explicit Permit(double interval) : interval_(interval) {}

  void record(double now, bool passed) {
    tested_ = true;
    last_at_ = now;
    last_passed_ = passed;
  }

  bool allowed(double now) const {
    if (!tested_ || !last_passed_) return false;
    if (now < last_at_) return false;
    return (now - last_at_) <= interval_;
  }

 private:
  double interval_ = 3600.0;
  bool tested_ = false;
  bool last_passed_ = false;
  double last_at_ = 0.0;
};

// The chance that a safety function has failed, undetected, by `hours` after it was last known to work, if it fails at `rate_per_hour` (a constant rate: 1 - exp(-rate * hours)).
inline double failure_probability(double rate_per_hour, double hours) {
  if (rate_per_hour <= 0.0 || hours <= 0.0) return 0.0;
  return 1.0 - std::exp(-rate_per_hour * hours);
}

// The longest interval between tests for which the chance of an undetected failure stays at or below `target` (0 to 1, exclusive): hours = -ln(1 - target) / rate. Infinity for a function that never fails; zero for a target of zero or less.
inline double max_interval(double rate_per_hour, double target) {
  if (target <= 0.0) return 0.0;
  if (rate_per_hour <= 0.0 || target >= 1.0) return INFINITY;
  return -std::log(1.0 - target) / rate_per_hour;
}

}  // namespace safety
}  // namespace rc

#endif  // RC_SAFETY_SELFTEST_HPP
