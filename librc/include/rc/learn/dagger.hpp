// rc/learn/dagger.hpp
//
// Asking the expert where the learner goes, from lesson 38-06.
//
// collect_on_learner drives the car with the learner from n starts drawn from a seed (offset within offset0, heading within heading0) for up to `steps` steps, and at every state it reaches records a sample whose label is the expert's action there, while the car goes on with the learner's own action; a run ends when the car leaves the lane. dagger repeats it for a number of rounds: collect, add to the data, train a new learner on everything, and measure the new learner's success from a spread of starts. visited_error is the mean squared difference between a learner's actions and the expert's over the states the learner visits from a spread of starts.
//
// The clone of 38-05, trained on 3000 samples, succeeded from the wide starts on 87 percent. After round 1 (4570 samples) it succeeded on 98 percent, after round 2 (6570 samples) on 98 percent; the expert's own rate from the same starts was 98 percent. On the states the learner itself visits, its actions differed from the expert's by 0.01082 before and 0.00009 after.

#ifndef RC_RC_LEARN_DAGGER_HPP
#define RC_RC_LEARN_DAGGER_HPP

#include <cmath>
#include <rc/learn/cloning.hpp>
#include <rc/learn/dataset.hpp>
#include <vector>

namespace rc {
namespace learn {

// The learner drives from n starts; every state it reaches is labelled with the expert's action. Runs end on leaving the lane.
inline std::vector<Sample> collect_on_learner(const LaneTask& t, const Learner& l, double offset0, double heading0, int n, int steps,
                                              int episode_base, unsigned long long seed) {
  std::vector<Sample> out;
  Rng r = make_rng(seed);
  for (int k = 0; k < n; ++k) {
    double e = offset0 * (2.0 * uniform(r) - 1.0);
    double h = heading0 * (2.0 * uniform(r) - 1.0);
    for (int i = 0; i < steps; ++i) {
      Sample x;
      x.s[0] = e;
      x.s[1] = h;
      x.a = lane_expert(t, e, h);
      x.episode = episode_base + k;
      out.push_back(x);
      lane_step(t, e, h, learner_act(l, e, h));
      if (!(std::fabs(e) <= t.edge)) break;
    }
  }
  return out;
}

struct DaggerRound {
  int samples = 0;       // the size of the data after the round
  double success = 0.0;  // the new learner's success rate from the test starts
};

// `rounds` rounds of collect, aggregate and retrain, starting from `data`. The final learner is left in `learner`.
inline std::vector<DaggerRound> dagger(const LaneTask& t, std::vector<Sample> data, Learner& learner, int rounds, int rollouts, int steps,
                                       double offset0, double heading0, int hidden, int epochs, double lr, unsigned long long seed) {
  std::vector<DaggerRound> out;
  for (int k = 1; k <= rounds; ++k) {
    const std::vector<Sample> extra = collect_on_learner(t, learner, offset0, heading0, rollouts, steps, 1000 * k, seed + static_cast<unsigned long long>(k));
    data.insert(data.end(), extra.begin(), extra.end());
    learner = clone(data, hidden, epochs, lr, 4);
    DaggerRound d;
    d.samples = static_cast<int>(data.size());
    d.success = success_rate(t, &learner, learner.scale, offset0, heading0, 100, 77, 300);
    out.push_back(d);
  }
  return out;
}

// The mean squared difference between the learner's actions and the expert's on the states the learner visits.
inline double visited_error(const LaneTask& t, const Learner& l, double offset0, double heading0, int n, int steps, unsigned long long seed) {
  const std::vector<Sample> v = collect_on_learner(t, l, offset0, heading0, n, steps, 0, seed);
  double s = 0.0;
  for (const Sample& x : v) {
    const double e = learner_act(l, x.s[0], x.s[1]) - x.a;
    s += e * e;
  }
  return v.empty() ? 0.0 : s / static_cast<double>(v.size());
}


}  // namespace learn
}  // namespace rc

#endif
