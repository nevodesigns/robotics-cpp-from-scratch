#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <rc/learn/dataset.hpp>
#include <rc/learn/mlp.hpp>
#include <rc/learn/train.hpp>
#include <vector>

using rc::learn::Rng;
using rc::learn::make_rng;
using rc::learn::uniform;
using rc::learn::Sample;
using rc::learn::Normaliser;
using rc::learn::normalise;
using rc::learn::fit_normaliser;
using rc::learn::Mlp;
using rc::learn::make_mlp;
using rc::learn::mlp_forward;
using rc::learn::mlp_mse;
using rc::learn::train_epoch;
using rc::learn::LaneTask;
using rc::learn::lane_expert;
using rc::learn::lane_step;
using rc::learn::demonstrations;

struct Learner {
  Mlp net;
  Normaliser scale;
};

inline double learner_act(const Learner& l, double offset, double heading) {
  const double s[2] = {offset, heading};
  double z[2];
  normalise(l.scale, s, z);
  return mlp_forward(l.net, z, nullptr);
}

// A network of `hidden` units trained on d for `epochs` epochs at step size lr.
inline Learner clone(const std::vector<Sample>& d, int hidden, int epochs, double lr, unsigned long long seed) {
  Learner l;
  l.scale = fit_normaliser(d);
  l.net = make_mlp(hidden, 1.0, seed);
  Rng r = make_rng(seed + 1000);
  for (int e = 0; e < epochs; ++e) train_epoch(l.net, l.scale, d, lr, r);
  return l;
}

struct Rollout {
  bool stayed = true;       // never past the lane's edge
  bool settled = false;     // ended with offset and heading within 0.05
  double worst = 0.0;       // m, the largest offset reached
  int unseen = 0;           // steps in states more than 3 spreads from the demonstrations' mean
  int steps = 0;            // steps driven
};

// Drives from (offset, heading) for `steps` steps with the expert (learner null) or the learner; `seen` is the scale of the
// demonstrations, used only to count unseen states. Stops when the car leaves the lane.
inline Rollout rollout(const LaneTask& t, const Learner* learner, const Normaliser& seen, double offset, double heading, int steps) {
  Rollout r;
  double e = offset, h = heading;
  (void)t;  // TODO: for each step: count the state as unseen if either input, scaled by `seen`, is more than 3 from zero; the turn is learner_act for a learner or lane_expert when learner is null; lane_step; count the step; keep the largest |offset| in worst; if |offset| is past t.edge (or not a number), set stayed false and return at once; after the last step, settled is |offset| < 0.05 and |heading| < 0.05
  (void)learner;
  (void)seen;
  (void)steps;
  (void)e;
  (void)h;
  return r;
}

// The share of n starts (offset within offset0, heading within heading0, drawn from seed) from which the policy stays and settles.
inline double success_rate(const LaneTask& t, const Learner* learner, const Normaliser& seen, double offset0, double heading0, int n,
                           unsigned long long seed, int steps) {
  Rng r = make_rng(seed);
  int ok = 0;
  for (int k = 0; k < n; ++k) {
    const double e = offset0 * (2.0 * uniform(r) - 1.0);
    const double h = heading0 * (2.0 * uniform(r) - 1.0);
    const Rollout ro = rollout(t, learner, seen, e, h, steps);
    if (ro.stayed && ro.settled) ++ok;
  }
  return n > 0 ? static_cast<double>(ok) / n : 0.0;
}


#endif
