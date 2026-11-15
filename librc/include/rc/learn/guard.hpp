// rc/learn/guard.hpp
//
// A learned policy behind a guard, from lesson 38-07.
//
// Guard holds the scale of the training data, the number of spreads from its mean beyond which a state counts as unseen, and the gains of the fall-back controller (a plain proportional law, clipped). guarded_act gives the turn and why: kLearned (the network's own output), kClipped (its output was past the limit and was clipped), kBadInput (an input was not a number: the fall-back with the input taken as zero), kUnseen (the state was too far from the training data: the fall-back), kBadOutput (the network gave a number that is not one: the fall-back). guarded_success drives from a spread of starts with the guard, gives the share that stayed in the lane and ended on the centre line, and the share of steps on which the fall-back acted.
//
// The clone of 38-05 (trained on starts near the centre) succeeded from the wide starts on 87 percent without the guard and 98 percent with it (a threshold of 3 spreads), handing over on 12 percent of the steps; the fall-back alone succeeded on 98 percent. From the narrow starts the guard handed over on 2.4 percent of the steps. With a threshold of 1 it handed over on 20 percent and succeeded on 98 percent; with 1000, a guard that never acts, on 0 percent and 87 percent, the clone's own rate.

#ifndef RC_RC_LEARN_GUARD_HPP
#define RC_RC_LEARN_GUARD_HPP

#include <cmath>
#include <rc/learn/cloning.hpp>
#include <rc/learn/dataset.hpp>
#include <vector>

namespace rc {
namespace learn {

enum GuardReason { kLearned = 0, kClipped = 1, kBadInput = 2, kUnseen = 3, kBadOutput = 4 };

struct Guard {
  Normaliser seen;          // the scale of the data the policy was trained on
  double z_max = 3.0;       // spreads from the mean beyond which a state is unseen
  double k_offset = 1.5;    // the fall-back's gains: a plain proportional law, written by hand
  double k_heading = 2.0;
};

struct GuardedAction {
  double turn = 0.0;
  int reason = kLearned;
};

inline double fallback_turn(const LaneTask& t, const Guard& g, double offset, double heading) {
  return clip(-g.k_offset * offset - g.k_heading * heading, t.max_turn);
}

inline GuardedAction guarded_act(const LaneTask& t, const Guard& g, const Learner& l, double offset, double heading) {
  GuardedAction a;
  if (!std::isfinite(offset) || !std::isfinite(heading)) {
    a.turn = fallback_turn(t, g, std::isfinite(offset) ? offset : 0.0, std::isfinite(heading) ? heading : 0.0);
    a.reason = kBadInput;
    return a;
  }
  const double s[2] = {offset, heading};
  double z[2];
  normalise(g.seen, s, z);
  if (std::fabs(z[0]) > g.z_max || std::fabs(z[1]) > g.z_max) {
    a.turn = fallback_turn(t, g, offset, heading);
    a.reason = kUnseen;
    return a;
  }
  const double out = learner_act(l, offset, heading);
  if (!std::isfinite(out)) {
    a.turn = fallback_turn(t, g, offset, heading);
    a.reason = kBadOutput;
    return a;
  }
  if (std::fabs(out) > t.max_turn) {
    a.turn = clip(out, t.max_turn);
    a.reason = kClipped;
    return a;
  }
  a.turn = out;
  a.reason = kLearned;
  return a;
}

// The share of n starts from which the guarded policy stays and settles; fallback_share gets the share of steps handed over.
inline double guarded_success(const LaneTask& t, const Guard& g, const Learner& l, double offset0, double heading0, int n, unsigned long long seed,
                              int steps, double* fallback_share) {
  Rng r = make_rng(seed);
  int ok = 0;
  long total = 0, handed = 0;
  for (int k = 0; k < n; ++k) {
    double e = offset0 * (2.0 * uniform(r) - 1.0);
    double h = heading0 * (2.0 * uniform(r) - 1.0);
    bool stayed = true;
    for (int i = 0; i < steps; ++i) {
      const GuardedAction a = guarded_act(t, g, l, e, h);
      ++total;
      if (a.reason == kBadInput || a.reason == kUnseen || a.reason == kBadOutput) ++handed;
      lane_step(t, e, h, a.turn);
      if (!(std::fabs(e) <= t.edge)) {
        stayed = false;
        break;
      }
    }
    if (stayed && std::fabs(e) < 0.05 && std::fabs(h) < 0.05) ++ok;
  }
  if (fallback_share) *fallback_share = total > 0 ? static_cast<double>(handed) / static_cast<double>(total) : 0.0;
  return n > 0 ? static_cast<double>(ok) / n : 0.0;
}


}  // namespace learn
}  // namespace rc

#endif
