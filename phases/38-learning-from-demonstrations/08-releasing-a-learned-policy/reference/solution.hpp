#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <limits>
#include <rc/learn/cloning.hpp>
#include <rc/learn/dagger.hpp>
#include <rc/learn/dataset.hpp>
#include <rc/learn/guard.hpp>
#include <rc/learn/linear_policy.hpp>
#include <rc/learn/train.hpp>
#include <vector>

using rc::learn::Rng;
using rc::learn::make_rng;
using rc::learn::uniform;
using rc::learn::Sample;
using rc::learn::LaneTask;
using rc::learn::lane_step;
using rc::learn::demonstrations;
using rc::learn::split_by_episode;
using rc::learn::fit_linear;
using rc::learn::linear_mse;
using rc::learn::mlp_mse;
using rc::learn::Learner;
using rc::learn::clone;
using rc::learn::dagger;
using rc::learn::Guard;
using rc::learn::GuardedAction;
using rc::learn::guarded_act;
using rc::learn::guarded_success;
using rc::learn::kBadInput;

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


#endif
