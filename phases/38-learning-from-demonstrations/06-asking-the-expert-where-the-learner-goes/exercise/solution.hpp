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
using rc::learn::fit_normaliser;
using rc::learn::LaneTask;
using rc::learn::lane_expert;
using rc::learn::lane_step;
using rc::learn::demonstrations;
using rc::learn::Learner;
using rc::learn::learner_act;
using rc::learn::clone;
using rc::learn::success_rate;

// The learner drives from n starts; every state it reaches is labelled with the expert's action. Runs end on leaving the lane.
inline std::vector<Sample> collect_on_learner(const LaneTask& t, const Learner& l, double offset0, double heading0, int n, int steps,
                                              int episode_base, unsigned long long seed) {
  std::vector<Sample> out;
  Rng r = make_rng(seed);
  (void)t;  // TODO: for each of n starts (offset then heading, each drawn as offset0 (2 uniform - 1) and heading0 (2 uniform - 1)): for up to `steps` steps, record a Sample of the state with the EXPERT's action as its label and episode episode_base + k, then move the car with the LEARNER's action; stop the run when |offset| passes t.edge
  (void)l;
  (void)offset0;
  (void)heading0;
  (void)n;
  (void)steps;
  (void)episode_base;
  (void)r;
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


#endif
