// rc/learn/cartpole_search.hpp
//
// Searching for a controller by trying it, from lesson 39-06.
//
// CartModel is the pole on a cart of Barto, Sutton and Anderson (1983): a cart of 1 kg on a track, a pole of 0.1 kg and half length 0.5 m hinged on it, pushed by a force limited to 10 N, stepped every 0.02 s. A run fails when the cart leaves the track (2.4 m from the centre) or the pole leans past 0.21 rad (12 degrees). cart_step applies a force for one step. The controller is four gains: the force is the gains times (position, speed, angle, rate). balance_steps runs it from a start until it fails or reaches a limit; balance_score is the mean over starts drawn from a seed, each value within 0.1 of zero. random_search starts from gains of zero and, each try, adds to every gain a random amount within plus or minus `step`, keeping the new gains if their score is higher.
//
// With every gain zero the pole balanced for 36.9 steps on average (it falls). Random search, scoring each try on 20 starts with a step of 2.0, reached 64.0 after 10 tries, 207.7 after 50 and 500.0 after 300, out of a limit of 500 steps; the gains it found were -0.83, 4.97, 9.06 and 19.94. On 100 starts it had never seen, they balanced for 497.9 steps on average. The search ran 6000 trial runs.

#ifndef RC_RC_LEARN_CARTPOLE_SEARCH_HPP
#define RC_RC_LEARN_CARTPOLE_SEARCH_HPP

#include <cmath>
#include <rc/learn/dataset.hpp>
#include <vector>

namespace rc {
namespace learn {

struct CartModel {
  double cart_mass = 1.0;     // kg
  double pole_mass = 0.1;     // kg
  double half_length = 0.5;   // m, from the hinge to the pole's centre
  double max_force = 10.0;    // N
  double dt = 0.02;           // s
  double track = 2.4;         // m, either side of the centre
  double max_angle = 0.21;    // rad
};

struct CartState {
  double x = 0.0, v = 0.0, angle = 0.0, rate = 0.0;
};

// One step of the pole on a cart with a force (clipped to the limit), by Euler's method as in the original.
inline void cart_step(const CartModel& m, CartState& c, double force) {
  if (force > m.max_force) force = m.max_force;
  if (force < -m.max_force) force = -m.max_force;
  const double g = 9.8, ct = std::cos(c.angle), st = std::sin(c.angle);
  const double total = m.cart_mass + m.pole_mass, pml = m.pole_mass * m.half_length;
  const double tmp = (force + pml * c.rate * c.rate * st) / total;
  const double alpha = (g * st - ct * tmp) / (m.half_length * (4.0 / 3.0 - m.pole_mass * ct * ct / total));
  const double acc = tmp - pml * alpha * ct / total;
  c.x += m.dt * c.v;
  c.v += m.dt * acc;
  c.angle += m.dt * c.rate;
  c.rate += m.dt * alpha;
}

inline double control_force(const double K[4], const CartState& c) { return K[0] * c.x + K[1] * c.v + K[2] * c.angle + K[3] * c.rate; }

inline int balance_steps(const CartModel& m, const double K[4], CartState c, int limit) {
  for (int i = 0; i < limit; ++i) {
    cart_step(m, c, control_force(K, c));
    if (!(std::fabs(c.x) <= m.track) || !(std::fabs(c.angle) <= m.max_angle)) return i + 1;
  }
  return limit;
}

inline CartState random_start(Rng& r) {
  CartState c;
  c.x = 0.1 * (2.0 * uniform(r) - 1.0);
  c.v = 0.1 * (2.0 * uniform(r) - 1.0);
  c.angle = 0.1 * (2.0 * uniform(r) - 1.0);
  c.rate = 0.1 * (2.0 * uniform(r) - 1.0);
  return c;
}

// The mean number of steps balanced from n starts drawn from seed.
inline double balance_score(const CartModel& m, const double K[4], int n, unsigned long long seed, int limit) {
  Rng r = make_rng(seed);
  double s = 0.0;
  for (int i = 0; i < n; ++i) s += balance_steps(m, K, random_start(r), limit);
  return n > 0 ? s / n : 0.0;
}

// From zero gains: each try adds a random amount within plus or minus step to every gain, and keeps the change if the
// score (n starts from score_seed) is higher. history, if not null, gets the best score after each try.
inline void random_search(const CartModel& m, int tries, double step, int n, unsigned long long score_seed, unsigned long long seed,
                          int limit, double K[4], std::vector<double>* history) {
  for (int j = 0; j < 4; ++j) K[j] = 0.0;
  Rng r = make_rng(seed);
  double best = balance_score(m, K, n, score_seed, limit);
  if (history) history->clear();
  for (int t = 0; t < tries; ++t) {
    double T[4];
    for (int j = 0; j < 4; ++j) T[j] = K[j] + step * (2.0 * uniform(r) - 1.0);
    const double s = balance_score(m, T, n, score_seed, limit);
    if (s > best) {
      best = s;
      for (int j = 0; j < 4; ++j) K[j] = T[j];
    }
    if (history) history->push_back(best);
  }
}


}  // namespace learn
}  // namespace rc

#endif
