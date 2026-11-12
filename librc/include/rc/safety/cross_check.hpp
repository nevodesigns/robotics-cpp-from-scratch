// rc/safety/cross_check.hpp
//
// A two channel cross-check, from lesson 19-07, graduated.
//
// CrossCheck compares two sensors that measure the same thing. A sample is bad if either reading is invalid (not a number or outside min_valid..max_valid; invalid_a named before invalid_b) or if they differ by more than the tolerance (disagree). `debounce` bad samples in a row latch a fault; a good sample before that clears the count. Until it latches the state is suspect and the value is the last agreed one; once it latches the value is NaN and the state stays faulted until reset(), even if the two agree again.
//
// With noise 0.02 on each channel, a tolerance of 0.04 and no debounce gave 15797 false trips in 100000 samples; a debounce of 3 gave 303; a tolerance of 0.12 with a debounce of 3 gave none. A drift of 0.005 per sample was seen after 3 samples at (0.04, 1) and 29 at (0.12, 3). Two channels both reading 1.5 for a truth of 3.0 agreed.

#ifndef RC_SAFETY_CROSS_CHECK_HPP
#define RC_SAFETY_CROSS_CHECK_HPP

#include <cmath>

namespace rc {
namespace safety {

enum class CheckState { ok, suspect, faulted };
enum class CheckReason { none, disagree, invalid_a, invalid_b };

struct CrossConfig {
  double tolerance = 0.1;    // the two may differ by this much
  int debounce = 3;          // this many bad samples in a row make a fault
  double min_valid = 0.0;    // a reading outside min_valid..max_valid, or not a number, is invalid
  double max_valid = 100.0;
};

struct Verdict {
  CheckState state = CheckState::ok;
  CheckReason reason = CheckReason::none;
  double value = 0.0;        // the agreed value; the last agreed one while suspect; NaN once faulted
};

// Two sensors that measure the same thing, checked against each other. A sample is bad if either reading is invalid (reason invalid_a or invalid_b, A first) or if they differ by more than the tolerance (disagree).
// `debounce` bad samples in a row latch a fault, and a good sample before that clears the count, so one noisy sample does not stop a machine. Until it latches the state is suspect and the value is the last agreed one;
// once it latches the state is faulted, the value is NaN (there is nothing to believe), and it stays so until reset().
class CrossCheck {
 public:
  explicit CrossCheck(const CrossConfig& c) : cfg_(c) {}

  Verdict update(double a, double b) {
    Verdict v;
    if (faulted_) {
      v.state = CheckState::faulted;
      v.reason = reason_;
      v.value = NAN;
      return v;
    }
    CheckReason bad = CheckReason::none;
    if (!valid(a)) bad = CheckReason::invalid_a;
    else if (!valid(b)) bad = CheckReason::invalid_b;
    else if (std::fabs(a - b) > cfg_.tolerance) bad = CheckReason::disagree;
    if (bad == CheckReason::none) {
      bad_run_ = 0;
      last_good_ = 0.5 * (a + b);
      v.value = last_good_;
      return v;
    }
    ++bad_run_;
    v.reason = bad;
    if (bad_run_ >= cfg_.debounce) {
      faulted_ = true;
      reason_ = bad;
      v.state = CheckState::faulted;
      v.value = NAN;
    } else {
      v.state = CheckState::suspect;
      v.value = last_good_;
    }
    return v;
  }

  bool faulted() const { return faulted_; }
  void reset() {
    faulted_ = false;
    bad_run_ = 0;
    reason_ = CheckReason::none;
  }

 private:
  bool valid(double x) const { return !std::isnan(x) && x >= cfg_.min_valid && x <= cfg_.max_valid; }

  CrossConfig cfg_;
  int bad_run_ = 0;
  bool faulted_ = false;
  CheckReason reason_ = CheckReason::none;
  double last_good_ = 0.0;
};

}  // namespace safety
}  // namespace rc

#endif  // RC_SAFETY_CROSS_CHECK_HPP
