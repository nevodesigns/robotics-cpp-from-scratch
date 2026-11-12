#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>

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
    // TODO: Once faulted: state faulted, the latched reason, value NaN.
    // A sample is bad (in this order of reason) if a is invalid (invalid_a), else b is invalid (invalid_b), else |a - b| > tolerance (disagree). Invalid: NaN or outside min_valid..max_valid.
    // A good sample: clear the bad run, remember the mean as last_good_, state ok, value that mean.
    // A bad one: count it; at debounce in a row latch the fault (state faulted, value NaN, reason kept); before that state suspect with the last good value and the reason.
    (void)a;
    (void)b;
    (void)bad_run_;
    (void)faulted_;
    (void)reason_;
    (void)last_good_;
    return Verdict();
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

#endif  // LESSON_SOLUTION_HPP
