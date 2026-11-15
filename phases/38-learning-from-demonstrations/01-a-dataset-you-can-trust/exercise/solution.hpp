#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <vector>

// A random stream that is the same on every compiler: no library generator, no library distribution.
struct Rng {
  unsigned long long state = 1;
};

inline Rng make_rng(unsigned long long seed) {
  Rng r;
  r.state = seed * 2862933555777941757ULL + 3037000493ULL;
  return r;
}

inline double uniform(Rng& r) {
  r.state = r.state * 6364136223846793005ULL + 1442695040888963407ULL;
  return static_cast<double>(r.state >> 11) / 9007199254740992.0;
}

// Close to a standard normal: the sum of twelve uniforms has mean 6 and variance 1.
inline double normal(Rng& r) {
  double a = 0.0;
  for (int i = 0; i < 12; ++i) a += uniform(r);
  return a - 6.0;
}

struct LaneTask {
  double speed = 2.0;       // m/s
  double k_offset = 2.0;    // rad/s per m, the expert's gain on the offset
  double k_heading = 3.0;   // 1/s, its gain on the heading error
  double max_turn = 1.0;    // rad/s, the largest turn the car can make
  double dt = 0.05;         // s
  double edge = 1.0;        // m, the lane's half width: past it the car has left the lane
};

inline double clip(double x, double limit) { return x > limit ? limit : (x < -limit ? -limit : x); }

inline double lane_expert(const LaneTask& t, double offset, double heading) {
  return clip(-t.k_offset * offset - t.k_heading * heading, t.max_turn);
}

inline void lane_step(const LaneTask& t, double& offset, double& heading, double turn) {
  const double w = clip(turn, t.max_turn);
  offset += t.speed * std::sin(heading) * t.dt;
  heading += w * t.dt;
}

struct Sample {
  double s[2] = {0.0, 0.0};   // the state: offset (m), heading error (rad)
  double a = 0.0;             // the action the expert chose: a turn rate (rad/s)
  int episode = 0;
};

// The expert from `episodes` random starts (offset within plus or minus offset0, heading within heading0), `steps` each.
// Each episode's demonstrator steers with the expert's gains times a factor drawn within plus or minus `style` (a person
// steers a little harder or softer each time); the label is that demonstrator's action, and what is executed has normal
// noise of `noise` rad/s added.
inline std::vector<Sample> demonstrations(const LaneTask& t, int episodes, int steps, double offset0, double heading0, double noise, double style, unsigned long long seed) {
  Rng r = make_rng(seed);
  std::vector<Sample> out;
  for (int k = 0; k < episodes; ++k) {
    double e = offset0 * (2.0 * uniform(r) - 1.0);
    double h = heading0 * (2.0 * uniform(r) - 1.0);
    LaneTask person = t;
    const double g = 1.0 + style * (2.0 * uniform(r) - 1.0);
    person.k_offset *= g;
    person.k_heading *= g;
    for (int i = 0; i < steps; ++i) {
      Sample x;
      x.s[0] = e;
      x.s[1] = h;
      x.a = lane_expert(person, e, h);
      x.episode = k;
      out.push_back(x);
      lane_step(t, e, h, x.a + noise * normal(r));
    }
  }
  return out;
}

struct Normaliser {
  double mean[2] = {0.0, 0.0};
  double sd[2] = {1.0, 1.0};
};

// The mean and the spread of each input over `d` (a spread of zero is taken as a tiny one, so dividing is safe).
inline Normaliser fit_normaliser(const std::vector<Sample>& d) {
  Normaliser n;
  (void)d;  // TODO: for each of the two inputs, the mean over d and the spread (the square root of the mean of the squares less the square of the mean, at least 1e-6); an empty d leaves the defaults
  return n;
}

inline void normalise(const Normaliser& n, const double s[2], double z[2]) {
  for (int j = 0; j < 2; ++j) z[j] = (s[j] - n.mean[j]) / n.sd[j];
}

// Every k-th sample to the test part.
inline void split_by_sample(const std::vector<Sample>& d, int every, std::vector<Sample>& train, std::vector<Sample>& test) {
  train.clear();
  test.clear();
  for (std::size_t i = 0; i < d.size(); ++i) {
    if (static_cast<int>(i % static_cast<std::size_t>(every)) == 0) {
      test.push_back(d[i]);
    } else {
      train.push_back(d[i]);
    }
  }
}

// Whole episodes to the test part: those whose number is a multiple of `every`.
inline void split_by_episode(const std::vector<Sample>& d, int every, std::vector<Sample>& train, std::vector<Sample>& test) {
  train.clear();
  test.clear();
  (void)d;  // TODO: a sample whose episode number is a multiple of `every` goes to test, every other sample to train, in order
  (void)every;
}

// How many episodes have samples in both parts.
inline int shared_episodes(const std::vector<Sample>& train, const std::vector<Sample>& test) {
  int top = -1;
  for (const Sample& x : train) top = x.episode > top ? x.episode : top;
  for (const Sample& x : test) top = x.episode > top ? x.episode : top;
  if (top < 0) return 0;
  std::vector<int> in_train(static_cast<std::size_t>(top) + 1, 0), in_test(static_cast<std::size_t>(top) + 1, 0);
  for (const Sample& x : train) in_train[static_cast<std::size_t>(x.episode)] = 1;
  for (const Sample& x : test) in_test[static_cast<std::size_t>(x.episode)] = 1;
  int n = 0;
  for (std::size_t k = 0; k < in_train.size(); ++k) n += in_train[k] * in_test[k];
  return n;
}

// The action of the training sample nearest to s, in normalised units.
inline double nearest_action(const std::vector<Sample>& train, const Normaliser& n, const double s[2]) {
  double best = 1e300, a = 0.0;
  for (const Sample& x : train) {
    double d = 0.0;
    for (int j = 0; j < 2; ++j) {
      const double u = (x.s[j] - s[j]) / n.sd[j];
      d += u * u;
    }
    if (d < best) {
      best = d;
      a = x.a;
    }
  }
  return a;
}

// The nearest neighbour's mean squared error on `test`, with the scale fitted on `train`.
inline double nearest_mse(const std::vector<Sample>& train, const std::vector<Sample>& test) {
  const Normaliser n = fit_normaliser(train);
  double s = 0.0;
  for (const Sample& x : test) {
    const double e = nearest_action(train, n, x.s) - x.a;
    s += e * e;
  }
  return test.empty() ? 0.0 : s / static_cast<double>(test.size());
}


#endif
