#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <rc/core/random.hpp>
#include <rc/locate/mcl.hpp>

using namespace rc::locate;

struct RecoverySettings {
  double alpha_slow = 0.05;   // the slow average moves this share of the way to each new likelihood
  double alpha_fast = 0.5;    // and the fast one this share
  double max_share = 0.2;     // the most of the cloud that may be put in at random in one step
};

class RecoveryMonitor {
 public:
  explicit RecoveryMonitor(const RecoverySettings& s) : s_(s) {}

  double slow() const { return slow_; }
  double fast() const { return fast_; }

  // One step: the average likelihood of the scan. Returns the share of the cloud to replace by random poses.
  double update(double average_likelihood) {
    if (!started_) {
      slow_ = fast_ = average_likelihood;
      started_ = true;
      return 0.0;
    }
    slow_ += s_.alpha_slow * (average_likelihood - slow_);
    fast_ += s_.alpha_fast * (average_likelihood - fast_);
    if (slow_ <= 0.0) return 0.0;
    const double share = 1.0 - fast_ / slow_;
    return std::max(0.0, std::min(s_.max_share, share));
  }

 private:
  RecoverySettings s_;
  double slow_ = 0.0;
  double fast_ = 0.0;
  bool started_ = false;
};

// Replaces a fraction of the cloud, chosen at random, by poses spread over the rectangle with any heading, and gives them all an equal weight (the cloud's weights are then normalised).
inline void scatter(MonteCarlo& mc, double fraction, double x0, double x1, double y0, double y1, rc::core::Rng& rng) {
  std::vector<Particle>& ps = mc.mutable_particles();
  const double pi = 3.14159265358979323846;
  for (std::size_t i = 0; i < ps.size(); ++i) {
    if (rng.uniform() >= fraction) continue;
    ps[i].x = x0 + (x1 - x0) * rng.uniform();
    ps[i].y = y0 + (y1 - y0) * rng.uniform();
    ps[i].theta = -pi + 2.0 * pi * rng.uniform();
    ps[i].w = 1.0 / static_cast<double>(ps.size());
  }
  double total = 0.0;
  for (std::size_t i = 0; i < ps.size(); ++i) total += ps[i].w;
  if (total > 0.0)
    for (std::size_t i = 0; i < ps.size(); ++i) ps[i].w /= total;
}

enum class Health { good, doubtful, lost };

// The state of the localisation from how the scan is explained now against how it usually is, and how tight the cloud is.
inline Health verdict(double fast_likelihood, double slow_likelihood, double spread) {
  if (slow_likelihood <= 0.0) return Health::lost;
  const double ratio = fast_likelihood / slow_likelihood;
  if (ratio < 0.4) return Health::lost;
  if (ratio < 0.8 || spread > 1.0) return Health::doubtful;
  return Health::good;
}


#endif
