#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <cstddef>
#include <vector>

struct Stamped {
  double t = 0.0;
  double x = 0.0;
  double y = 0.0;
  double theta = 0.0;
};

// The recent states of an estimator, in time order, from which the state at a past time can be found.
class StateHistory {
 public:
  void add(const Stamped& s) {
    if (!states_.empty() && s.t < states_.back().t) return;
    states_.push_back(s);
    if (states_.size() > 4000) states_.erase(states_.begin(), states_.begin() + 1000);
  }

  std::size_t size() const { return states_.size(); }

  // The state at time t, interpolated between the states either side of it. False if t is before the first or after the last.
  bool at(double t, Stamped& out) const {
    if (states_.empty() || t < states_.front().t || t > states_.back().t) return false;
    std::size_t hi = 1;
    while (hi < states_.size() && states_[hi].t < t) ++hi;
    if (hi >= states_.size()) {
      out = states_.back();
      return true;
    }
    const Stamped& a = states_[hi - 1];
    const Stamped& b = states_[hi];
    const double span = b.t - a.t;
    const double u = span > 0.0 ? (t - a.t) / span : 0.0;
    out.t = t;
    out.x = a.x + u * (b.x - a.x);
    out.y = a.y + u * (b.y - a.y);
    out.theta = a.theta + u * (b.theta - a.theta);
    return true;
  }

 private:
  std::vector<Stamped> states_;
};

struct Correction {
  double dx = 0.0;
  double dy = 0.0;
  bool valid = false;
};

// A position measurement taken at time `stamp` is compared with the state the estimator had then, and the difference, times a gain from 0 to 1, is the correction to add to the state now. False (not valid) if the history does not reach back to the stamp.
inline Correction delayed_correction(const StateHistory& history, double stamp, double zx, double zy, double gain) {
  Correction c;
  Stamped then;
  if (!history.at(stamp, then)) return c;
  c.dx = gain * (zx - then.x);
  c.dy = gain * (zy - then.y);
  c.valid = true;
  return c;
}

// The shift of b against a, in samples (positive: b is late), over -max_lag to max_lag, that makes the two signals most alike; the sum of products is the measure, with the means taken off.
inline int estimate_lag(const std::vector<double>& a, const std::vector<double>& b, int max_lag) {
  double ma = 0.0, mb = 0.0;
  const std::size_t n = a.size() < b.size() ? a.size() : b.size();
  for (std::size_t i = 0; i < n; ++i) {
    ma += a[i];
    mb += b[i];
  }
  if (n == 0) return 0;
  ma /= static_cast<double>(n);
  mb /= static_cast<double>(n);
  int best = 0;
  double best_score = -1e300;
  for (int lag = -max_lag; lag <= max_lag; ++lag) {
    double sum = 0.0;
    for (std::size_t i = 0; i < n; ++i) {
      const long j = static_cast<long>(i) + lag;
      if (j < 0 || j >= static_cast<long>(n)) continue;
      sum += (a[i] - ma) * (b[static_cast<std::size_t>(j)] - mb);
    }
    if (sum > best_score) {
      best_score = sum;
      best = lag;
    }
  }
  return best;
}


#endif
