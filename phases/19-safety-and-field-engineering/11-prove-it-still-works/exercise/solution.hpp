#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <cstddef>
#include <functional>
#include <string>
#include <vector>

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
    // TODO: for each check, in order: a result with its name; no function: failed, detail "no check"; otherwise call it inside try, passed is what it returns (detail "returned false" if false), and a throw (catch (...)) is failed with detail "threw an exception". Run all of them
    (void)checks_;
    return std::vector<CheckResult>();
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
  // TODO: false for an empty list; false if any failed; otherwise true
  (void)results;
  return false;
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
    // TODO: false if never tested, or the last test failed, or now is before the last test; otherwise true if now - last test <= interval
    (void)now;
    (void)interval_;
    (void)tested_;
    (void)last_passed_;
    (void)last_at_;
    return false;
  }

 private:
  double interval_ = 3600.0;
  bool tested_ = false;
  bool last_passed_ = false;
  double last_at_ = 0.0;
};

// The chance that a safety function has failed, undetected, by `hours` after it was last known to work, if it fails at `rate_per_hour` (a constant rate: 1 - exp(-rate * hours)).
inline double failure_probability(double rate_per_hour, double hours) {
  // TODO: zero if rate or hours is not above zero; otherwise 1 - exp(-rate * hours)
  (void)rate_per_hour;
  (void)hours;
  return 0.0;
}

// The longest interval between tests for which the chance of an undetected failure stays at or below `target` (0 to 1, exclusive): hours = -ln(1 - target) / rate. Infinity for a function that never fails; zero for a target of zero or less.
inline double max_interval(double rate_per_hour, double target) {
  // TODO: zero if target <= 0; infinity if rate <= 0 or target >= 1; otherwise -log(1 - target) / rate
  (void)rate_per_hour;
  (void)target;
  return 0.0;
}

#endif  // LESSON_SOLUTION_HPP
