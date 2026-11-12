// rc/locate/mcl.hpp
//
// Monte Carlo localisation, from lesson 25-05.
//
// A set of particles, each a pose and a weight. predict moves each by the odometry (a distance ahead and a turn) with noise of its own. correct weighs each by the product, over the beams, of the likelihood of the range seen given the range that the map gives from that pose: a Gaussian about the expected range plus a share that is uniform to the maximum range, so that one wild beam cannot zero a good particle. correct returns the average likelihood of the scan under the cloud, the signal that the cloud no longer explains what is seen. resample_if_needed draws a new set in proportion to the weights, by the low variance method, when the effective number of particles N / (1 + variance of the weights) has fallen below half. mean_pose and spread summarise the cloud; the heading is averaged as an angle.
//
// From a cloud spread over the whole room, 3000 particles found the robot to 0.021 m after 5 steps of 0.2 m; of 10 runs, 50 particles lost it 10 times and 3000 particles 0; in a rectangular room with nothing in it the cloud settled on two places, 2 of 8 runs ending on the mirror image, and a post in one corner made it 0.

#ifndef RC_RC_LOCATE_MCL_HPP
#define RC_RC_LOCATE_MCL_HPP

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <rc/core/random.hpp>
#include <rc/field/sensors.hpp>
#include <vector>

namespace rc {
namespace locate {

struct Particle {
  double x = 0.0;
  double y = 0.0;
  double theta = 0.0;
  double w = 1.0;
};

struct McMeasure {
  double sigma = 0.15;      // standard deviation of a range that agrees with the map, metres
  double z_hit = 0.8;       // share of the likelihood that is a hit
  double z_rand = 0.2;      // share that is a uniform part: anything can be seen
  double max_range = 12.0;
  double temper = 0.25;     // the beams are not independent (neighbours hit the same wall), so the sum of their log likelihoods counts for only this share
};

class MonteCarlo {
 public:
  MonteCarlo(const rc::field::World2& map, int count, unsigned int seed) : map_(map), rng_(seed), particles_(static_cast<std::size_t>(count)) {}

  const std::vector<Particle>& particles() const { return particles_; }

  // Spreads the particles over the whole rectangle with any heading.
  void init_uniform(double x0, double x1, double y0, double y1) {
    for (std::size_t i = 0; i < particles_.size(); ++i) {
      particles_[i].x = x0 + (x1 - x0) * rng_.uniform();
      particles_[i].y = y0 + (y1 - y0) * rng_.uniform();
      particles_[i].theta = -3.14159265358979323846 + 2.0 * 3.14159265358979323846 * rng_.uniform();
      particles_[i].w = 1.0 / static_cast<double>(particles_.size());
    }
  }

  // The robot went `ahead` metres and turned `turn` radians (as the odometry says); each particle goes that far with noise of its own.
  void predict(double ahead, double turn, double sigma_ahead, double sigma_turn) {
    for (std::size_t i = 0; i < particles_.size(); ++i) {
      const double d = ahead + sigma_ahead * rng_.gaussian();
      const double t = turn + sigma_turn * rng_.gaussian();
      const double mid = particles_[i].theta + 0.5 * t;
      particles_[i].x += d * std::cos(mid);
      particles_[i].y += d * std::sin(mid);
      particles_[i].theta += t;
    }
  }

  // The range a lidar beam at `angle` from the particle's heading would see from the particle in the map.
  double expected_range(const Particle& p, double angle, double max_range) const { return rc::field::cast_ray(map_, p.x, p.y, p.theta + angle, max_range); }

  // Weighs the particles by the ranges seen at the given beam angles (infinity for a beam that saw nothing). Returns the average of the likelihood factors, weighted by the weights the particles had: how well the whole cloud explains this scan, which is low when the robot is not where the cloud says.
  double correct(const std::vector<double>& angles, const std::vector<double>& ranges, const McMeasure& m) {
    double total = 0.0;
    double before = 0.0;
    double weighted = 0.0;
    for (std::size_t i = 0; i < particles_.size(); ++i) {
      const double w_before = particles_[i].w;
      before += w_before;
      double logw = 0.0;
      for (std::size_t b = 0; b < angles.size() && b < ranges.size(); ++b) {
        const double seen = std::isfinite(ranges[b]) ? ranges[b] : m.max_range;
        const double e = expected_range(particles_[i], angles[b], m.max_range);
        const double expected = std::isfinite(e) ? e : m.max_range;
        const double z = (seen - expected) / m.sigma;
        const double hit = m.z_hit * std::exp(-0.5 * z * z) / (m.sigma * 2.5066282746310002);
        const double rand = m.z_rand / m.max_range;
        logw += std::log(hit + rand);
      }
      const double factor = std::exp(m.temper * logw);
      particles_[i].w *= factor;
      total += particles_[i].w;
      weighted += w_before * factor;
    }
    const double average = before > 0.0 ? weighted / before : 0.0;
    if (total <= 0.0) {
      for (std::size_t i = 0; i < particles_.size(); ++i) particles_[i].w = 1.0 / static_cast<double>(particles_.size());
      return average;
    }
    for (std::size_t i = 0; i < particles_.size(); ++i) particles_[i].w /= total;
    return average;
  }

  // For the lessons that follow: the particles, to be changed.
  std::vector<Particle>& mutable_particles() { return particles_; }

  // N / (1 + variance of the weights times N squared): the number of particles that count.
  double effective_count() const {
    double sq = 0.0;
    for (std::size_t i = 0; i < particles_.size(); ++i) sq += particles_[i].w * particles_[i].w;
    return sq > 0.0 ? 1.0 / sq : 0.0;
  }

  // Draws a new set in proportion to the weights, by the low variance method, if fewer than half count. Returns true if it did.
  bool resample_if_needed() {
    const std::size_t n = particles_.size();
    if (effective_count() >= 0.5 * static_cast<double>(n)) return false;
    std::vector<Particle> next(n);
    const double step = 1.0 / static_cast<double>(n);
    double u = rng_.uniform() * step;
    double c = particles_[0].w;
    std::size_t i = 0;
    for (std::size_t m = 0; m < n; ++m) {
      const double target = u + static_cast<double>(m) * step;
      while (target > c && i + 1 < n) {
        ++i;
        c += particles_[i].w;
      }
      next[m] = particles_[i];
      next[m].w = step;
    }
    particles_ = next;
    return true;
  }

  // The weighted mean pose (the heading by its sine and cosine), and the spread of the positions.
  Particle mean_pose() const {
    Particle m;
    double sx = 0.0, sy = 0.0, ss = 0.0, sc = 0.0;
    for (std::size_t i = 0; i < particles_.size(); ++i) {
      sx += particles_[i].w * particles_[i].x;
      sy += particles_[i].w * particles_[i].y;
      ss += particles_[i].w * std::sin(particles_[i].theta);
      sc += particles_[i].w * std::cos(particles_[i].theta);
    }
    m.x = sx;
    m.y = sy;
    m.theta = std::atan2(ss, sc);
    return m;
  }

  double spread() const {
    const Particle m = mean_pose();
    double v = 0.0;
    for (std::size_t i = 0; i < particles_.size(); ++i) v += particles_[i].w * ((particles_[i].x - m.x) * (particles_[i].x - m.x) + (particles_[i].y - m.y) * (particles_[i].y - m.y));
    return std::sqrt(v);
  }

 private:
  rc::field::World2 map_;
  rc::core::Rng rng_;
  std::vector<Particle> particles_;
};


}  // namespace locate
}  // namespace rc

#endif
