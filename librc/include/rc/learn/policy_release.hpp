// rc/learn/policy_release.hpp
//
// Releasing a learned policy, from lesson 38-08.
//
// PolicyJob is what the policy is for: the spread of starts (offset0, heading0), the share it must succeed on, and the largest share of steps its guard may hand over. Candidate is a learned policy as it would be released: the learner, its guard, and the data and settings it was trained with (so it can be trained again). check_policy runs six requirements in order and names the first that fails: 1 it beats the straight line of 38-02 on the test episodes, 2 it succeeds from the job's starts behind its guard, 3 the guard hands over on no more than the job allows, 4 an input that is not a number gives a finite turn, 5 no turn it commands over the job's runs is past the car's limit, 6 training it again from the same seed gives the same weights. PolicyVerdict keeps every figure.
//
// A policy trained by two rounds of DAgger was released for a job of starts within 0.6 m and 0.8 rad: test error 0.00022 against the line's 0.01240, success 98 percent (needed 95), handover 3.2 percent (allowed 5). The plain clone of 38-05 failed at requirement 1: on episodes from the job's starts its test error was 0.01618, worse than the line's (and its guard handed over on 12 percent of the steps). The DAgger policy behind a guard with a threshold of 1 spread failed at requirement 3, handing over on 10 percent. A network of one unit trained for 2 epochs passed (first failure 0, none), with a test error of 0.00056: one tanh is close to the expert's clipped line. A job of starts within 0.95 m and 1.2 rad failed at requirement 2 (success 73 percent).

#ifndef RC_RC_LEARN_POLICY_RELEASE_HPP
#define RC_RC_LEARN_POLICY_RELEASE_HPP

#include <cmath>
#include <limits>
#include <rc/learn/cloning.hpp>
#include <rc/learn/dagger.hpp>
#include <rc/learn/dataset.hpp>
#include <rc/learn/guard.hpp>
#include <rc/learn/linear_policy.hpp>
#include <rc/learn/train.hpp>
#include <vector>

namespace rc {
namespace learn {

struct PolicyJob {
  double offset0 = 0.6;          // m, starts within this offset
  double heading0 = 0.8;         // rad, and this heading error
  double need_success = 0.95;    // the share of starts it must succeed on
  double max_handover = 0.05;    // the largest share of steps the guard may hand over
};

struct Candidate {
  Learner learner;
  Guard guard;
  std::vector<Sample> data;      // what it was trained on, so it can be trained again
  int hidden = 16;
  int epochs = 100;
  double lr = 0.02;
  unsigned long long seed = 4;
};

struct PolicyVerdict {
  double test_error = 0.0, line_error = 0.0;
  bool beats_line = false;
  double success = 0.0;
  bool success_ok = false;
  double handover = 0.0;
  bool handover_ok = false;
  bool bad_input_ok = false;
  double largest_turn = 0.0;
  bool limit_ok = false;
  bool repeatable = false;
  bool released = false;
  int first_fail = 0;            // 0 none, 1 beats the line, 2 success, 3 handover, 4 bad input, 5 limit, 6 repeatable
};

inline bool same_weights(const Learner& a, const Learner& b) {
  return a.net.w1 == b.net.w1 && a.net.b1 == b.net.b1 && a.net.w2 == b.net.w2 && a.net.b2 == b.net.b2;
}

// The largest |turn| the guarded policy commands over n runs from the job's starts.
inline double largest_turn(const LaneTask& t, const PolicyJob& j, const Candidate& c, int n, unsigned long long seed, int steps) {
  Rng r = make_rng(seed);
  double worst = 0.0;
  for (int k = 0; k < n; ++k) {
    double e = j.offset0 * (2.0 * uniform(r) - 1.0);
    double h = j.heading0 * (2.0 * uniform(r) - 1.0);
    for (int i = 0; i < steps; ++i) {
      const GuardedAction a = guarded_act(t, c.guard, c.learner, e, h);
      if (!(std::fabs(a.turn) <= worst)) worst = std::fabs(a.turn);
      lane_step(t, e, h, a.turn);
      if (!(std::fabs(e) <= t.edge)) break;
    }
  }
  return worst;
}

inline PolicyVerdict check_policy(const LaneTask& t, const PolicyJob& j, const Candidate& c, const std::vector<Sample>& train,
                                  const std::vector<Sample>& test) {
  PolicyVerdict v;
  v.test_error = mlp_mse(c.learner.net, c.learner.scale, test);
  v.line_error = linear_mse(fit_linear(train, 0.0), test);
  v.beats_line = v.test_error < v.line_error;
  v.success = guarded_success(t, c.guard, c.learner, j.offset0, j.heading0, 100, 77, 300, &v.handover);
  v.success_ok = v.success >= j.need_success;
  v.handover_ok = v.handover <= j.max_handover;
  const GuardedAction bad = guarded_act(t, c.guard, c.learner, std::numeric_limits<double>::quiet_NaN(), 0.1);
  v.bad_input_ok = bad.reason == kBadInput && std::isfinite(bad.turn);
  v.largest_turn = largest_turn(t, j, c, 50, 91, 300);
  v.limit_ok = v.largest_turn <= t.max_turn;
  v.repeatable = same_weights(clone(c.data, c.hidden, c.epochs, c.lr, c.seed), clone(c.data, c.hidden, c.epochs, c.lr, c.seed));
  const bool ok[6] = {v.beats_line, v.success_ok, v.handover_ok, v.bad_input_ok, v.limit_ok, v.repeatable};
  for (int i = 0; i < 6; ++i) {
    if (!ok[i]) {
      v.first_fail = i + 1;
      break;
    }
  }
  v.released = v.first_fail == 0;
  return v;
}


}  // namespace learn
}  // namespace rc

#endif
