// rc/arm/demo_record.hpp
//
// Keeping what the hand showed, from lesson 31-09.
//
// A Sample is one moment of a demonstration: the time, the leader's six joints (the action the person gave), the follower's six (what was observed) and the hand's opening. An episode is a vector of them, in time order. resample gives the episode on a steady grid by linear interpolation (messages never arrive at a steady rate). max_gap is the longest time between two samples of the original. trim_idle cuts the stretches at the start and the end in which the leader was not moving (its speed under a threshold), keeping a few samples of padding. estimate_lag finds the delay, in seconds, of the follower behind the leader by the shift that makes them agree best. check_episode summarises an episode and says whether it is worth keeping: its longest gap short enough, it long enough, and not mostly idle.
//
// An episode of 5.0 s had 151 samples at a nominal 30 Hz with jitter; its longest gap was 0.039 s. Resampled it had 151 samples. Trimming the 1 s of idling at each end left 2.8 s. The follower was found 100 ms behind the leader (it was 100 ms). An episode that lost 20 frames in a row had a longest gap of 0.70 s and 0 (0: refused), against 1 for the good one.

#ifndef RC_RC_ARM_DEMO_RECORD_HPP
#define RC_RC_ARM_DEMO_RECORD_HPP

#include <cmath>
#include <cstddef>
#include <vector>

namespace rc {
namespace arm {

struct Sample {
  double t = 0.0;
  double q[6] = {0, 0, 0, 0, 0, 0};    // the leader's joints: the action
  double qf[6] = {0, 0, 0, 0, 0, 0};   // the follower's joints: what was observed
  double grip = 0.0;
};

using Episode = std::vector<Sample>;

// The longest time between two successive samples.
inline double max_gap(const Episode& e) {
  double g = 0.0;
  for (std::size_t i = 1; i < e.size(); ++i)
    if (e[i].t - e[i - 1].t > g) g = e[i].t - e[i - 1].t;
  return g;
}

// The episode on a steady grid at `rate` Hz, by linear interpolation.
inline Episode resample(const Episode& in, double rate) {
  Episode out;
  if (in.size() < 2 || rate <= 0.0) return out;
  std::size_t j = 0;
  for (double t = in.front().t; t <= in.back().t + 1e-12; t += 1.0 / rate) {
    while (j + 2 < in.size() && in[j + 1].t < t) ++j;
    const Sample& a = in[j];
    const Sample& b = in[j + 1];
    const double w = b.t > a.t ? (t - a.t) / (b.t - a.t) : 0.0;
    Sample s;
    s.t = t;
    for (int k = 0; k < 6; ++k) {
      s.q[k] = a.q[k] + w * (b.q[k] - a.q[k]);
      s.qf[k] = a.qf[k] + w * (b.qf[k] - a.qf[k]);
    }
    s.grip = a.grip + w * (b.grip - a.grip);
    out.push_back(s);
  }
  return out;
}

// The speed of the leader (rad/s, over all joints) between sample i - 1 and i.
inline double leader_speed(const Episode& e, std::size_t i) {
  if (i == 0 || i >= e.size() || e[i].t <= e[i - 1].t) return 0.0;
  double s = 0.0;
  for (int k = 0; k < 6; ++k) s += (e[i].q[k] - e[i - 1].q[k]) * (e[i].q[k] - e[i - 1].q[k]);
  return std::sqrt(s) / (e[i].t - e[i - 1].t);
}

// Cuts the idle ends: from `pad` samples before the first moving one to `pad` after the last (leader speed over `thr`).
inline Episode trim_idle(const Episode& e, double thr, std::size_t pad) {
  std::size_t first = e.size(), last = 0;
  for (std::size_t i = 1; i < e.size(); ++i) {
    if (leader_speed(e, i) > thr) {
      if (first == e.size()) first = i;
      last = i;
    }
  }
  if (first == e.size()) return Episode();
  const std::size_t lo = first > pad ? first - pad : 0;
  const std::size_t hi = last + pad < e.size() - 1 ? last + pad : e.size() - 1;
  return Episode(e.begin() + static_cast<std::ptrdiff_t>(lo), e.begin() + static_cast<std::ptrdiff_t>(hi) + 1);
}

// The delay, in seconds, of the follower behind the leader, by the shift (up to max_lag seconds) that makes joint 0 agree best. The episode should be on a steady grid.
inline double estimate_lag(const Episode& e, double rate, double max_lag) {
  const int max_shift = static_cast<int>(max_lag * rate);
  int best = 0;
  double best_err = 1e18;
  for (int lag = 0; lag <= max_shift; ++lag) {
    double err = 0.0;
    int n = 0;
    for (std::size_t i = 0; i + static_cast<std::size_t>(lag) < e.size(); ++i) {
      const double d = e[i + static_cast<std::size_t>(lag)].qf[0] - e[i].q[0];
      err += d * d;
      ++n;
    }
    if (n > 0 && err / n < best_err) {
      best_err = err / n;
      best = lag;
    }
  }
  return best / rate;
}

struct EpisodeCheck {
  std::size_t n = 0;
  double duration = 0.0;
  double longest_gap = 0.0;
  double idle_fraction = 0.0;
  bool ok = false;
};

// A summary of an episode, and whether it is worth keeping.
inline EpisodeCheck check_episode(const Episode& e, double idle_thr, double max_allowed_gap, double min_duration, double max_idle) {
  EpisodeCheck c;
  c.n = e.size();
  if (e.size() < 2) return c;
  c.duration = e.back().t - e.front().t;
  c.longest_gap = max_gap(e);
  std::size_t idle = 0;
  for (std::size_t i = 1; i < e.size(); ++i)
    if (leader_speed(e, i) <= idle_thr) ++idle;
  c.idle_fraction = static_cast<double>(idle) / static_cast<double>(e.size() - 1);
  c.ok = c.longest_gap <= max_allowed_gap && c.duration >= min_duration && c.idle_fraction <= max_idle;
  return c;
}


}  // namespace arm
}  // namespace rc

#endif
