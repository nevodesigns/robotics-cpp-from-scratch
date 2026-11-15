#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <rc/learn/cloning.hpp>
#include <rc/learn/dataset.hpp>
#include <vector>

using rc::learn::Rng;
using rc::learn::make_rng;
using rc::learn::uniform;
using rc::learn::Sample;
using rc::learn::Normaliser;
using rc::learn::normalise;
using rc::learn::fit_normaliser;
using rc::learn::LaneTask;
using rc::learn::clip;
using rc::learn::lane_step;
using rc::learn::demonstrations;
using rc::learn::Learner;
using rc::learn::learner_act;
using rc::learn::clone;
using rc::learn::success_rate;

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


#endif
