#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <cstddef>
#include <limits>
#include <rc/core/random.hpp>
#include <vector>

inline double no_return() { return std::numeric_limits<double>::infinity(); }

// A wall, and a post.
struct Segment {
  double x0 = 0.0;
  double y0 = 0.0;
  double x1 = 0.0;
  double y1 = 0.0;
};

struct Circle {
  double x = 0.0;
  double y = 0.0;
  double r = 0.0;
};

struct World2 {
  std::vector<Segment> walls;
  std::vector<Circle> posts;
};

// The distance along a ray from (ox, oy) at an angle to the nearest wall or post, or no_return() if there is none within max_range.
inline double cast_ray(const World2& world, double ox, double oy, double angle, double max_range) {
  double best = no_return();
  const double dx = std::cos(angle);
  const double dy = std::sin(angle);
  for (std::size_t i = 0; i < world.walls.size(); ++i) {
    const Segment& s = world.walls[i];
    const double ex = s.x1 - s.x0;
    const double ey = s.y1 - s.y0;
    const double denom = dx * ey - dy * ex;
    if (std::fabs(denom) < 1e-12) continue;
    const double px = s.x0 - ox;
    const double py = s.y0 - oy;
    const double t = (px * ey - py * ex) / denom;
    const double u = (px * dy - py * dx) / denom;
    if (t >= 0.0 && u >= 0.0 && u <= 1.0 && t < best) best = t;
  }
  for (std::size_t i = 0; i < world.posts.size(); ++i) {
    const Circle& c = world.posts[i];
    const double fx = ox - c.x;
    const double fy = oy - c.y;
    const double b = dx * fx + dy * fy;
    const double q = fx * fx + fy * fy - c.r * c.r;
    const double disc = b * b - q;
    if (disc < 0.0) continue;
    double t = -b - std::sqrt(disc);
    if (t < 0.0) t = -b + std::sqrt(disc);
    if (t >= 0.0 && t < best) best = t;
  }
  if (best > max_range) best = no_return();
  return best;
}

struct LidarSpec {
  int beams = 360;
  double fov = 6.283185307179586;
  double min_range = 0.15;
  double max_range = 12.0;
  double sigma = 0.02;
  double dropout = 0.01;
};

// One scan from a pose: a range per beam, no_return() where the beam saw nothing, was too near or too far, or was lost.
inline std::vector<double> scan(const LidarSpec& spec, const World2& world, double x, double y, double theta, rc::core::Rng& rng) {
  std::vector<double> ranges(static_cast<std::size_t>(spec.beams), no_return());
  for (int i = 0; i < spec.beams; ++i) {
    const double angle = theta - 0.5 * spec.fov + spec.fov * (static_cast<double>(i) + 0.5) / static_cast<double>(spec.beams);
    double r = cast_ray(world, x, y, angle, spec.max_range);
    if (r < spec.min_range) r = no_return();
    if (r != no_return()) {
      r += spec.sigma * rng.gaussian();
      if (rng.uniform() < spec.dropout) r = no_return();
    }
    ranges[static_cast<std::size_t>(i)] = r;
  }
  return ranges;
}

// A gyro about z and two accelerometers, with white noise, a constant bias and a bias that wanders.
struct ImuSpec {
  double gyro_sigma = 0.005;   // rad/s (typical of a MEMS part)
  double gyro_bias = 0.01;     // rad/s (typical)
  double bias_walk = 1.0e-4;   // rad/s per root second (typical)
  double accel_sigma = 0.05;   // m/s2 (typical)
  double accel_bias = 0.02;    // m/s2 (typical)
};

struct ImuSample {
  double gyro_z = 0.0;
  double ax = 0.0;
  double ay = 0.0;
};

class Imu {
 public:
  Imu(const ImuSpec& spec, unsigned int seed) : s_(spec), rng_(seed), bias_(spec.gyro_bias) {}

  ImuSample measure(double true_w, double true_ax, double true_ay, double dt) {
    bias_ += s_.bias_walk * std::sqrt(dt) * rng_.gaussian();
    ImuSample m;
    m.gyro_z = true_w + bias_ + s_.gyro_sigma * rng_.gaussian();
    m.ax = true_ax + s_.accel_bias + s_.accel_sigma * rng_.gaussian();
    m.ay = true_ay + s_.accel_bias + s_.accel_sigma * rng_.gaussian();
    return m;
  }

 private:
  ImuSpec s_;
  rc::core::Rng rng_;
  double bias_;
};

enum class Fix { none, single, floating, fixed };

struct GnssSpec {
  double sigma_single = 1.5;    // metres per axis (typical of a single frequency receiver)
  double sigma_float = 0.25;    // metres (typical of an RTK float solution)
  double sigma_fixed = 0.02;    // metres (typical of an RTK fixed solution)
  double outlier_prob = 0.01;   // chance per fix of a jump (multipath, a reflection)
  double outlier_size = 8.0;    // metres
};

struct GnssFix {
  bool valid = false;
  Fix fix = Fix::none;
  double x = 0.0;
  double y = 0.0;
};

class Gnss {
 public:
  Gnss(const GnssSpec& spec, unsigned int seed) : s_(spec), rng_(seed) {}

  GnssFix measure(double true_x, double true_y, Fix level) {
    GnssFix out;
    out.fix = level;
    if (level == Fix::none) return out;
    const double sigma = level == Fix::single ? s_.sigma_single : (level == Fix::floating ? s_.sigma_float : s_.sigma_fixed);
    out.valid = true;
    out.x = true_x + sigma * rng_.gaussian();
    out.y = true_y + sigma * rng_.gaussian();
    if (rng_.uniform() < s_.outlier_prob) {
      const double a = 6.283185307179586 * rng_.uniform();
      out.x += s_.outlier_size * std::cos(a);
      out.y += s_.outlier_size * std::sin(a);
    }
    return out;
  }

 private:
  GnssSpec s_;
  rc::core::Rng rng_;
};

// A wheel encoder: it counts whole ticks. A wrong wheel radius scales every reading, and a wheel that slips turns more than the vehicle moves.
struct EncoderSpec {
  double ticks_per_m = 1000.0;
  double scale_error = 0.0;  // fraction: 0.02 reads 2 percent long
  double slip = 0.0;         // fraction of extra wheel travel
};

class Encoder {
 public:
  explicit Encoder(const EncoderSpec& spec) : s_(spec) {}

  void move(double true_distance) { wheel_ += true_distance * (1.0 + s_.slip) * (1.0 + s_.scale_error); }

  long ticks() const { return static_cast<long>(std::floor(wheel_ * s_.ticks_per_m)); }

  // The distance as the vehicle would compute it from the ticks.
  double distance() const { return static_cast<double>(ticks()) / s_.ticks_per_m; }

 private:
  EncoderSpec s_;
  double wheel_ = 0.0;
};


#endif
