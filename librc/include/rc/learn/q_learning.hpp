// rc/learn/q_learning.hpp
//
// Learning from trying, from lesson 39-03.
//
// The Q table holds, for every cell, the value of each of the four moves. choose_move takes the move with the highest value (the first of equals) and, with chance epsilon, one of the four at random instead. take_step makes a move in the task, drawing its outcome from the chances the learner does not see, and returns where it led. q_update nudges the value of the move made toward the target, the reward plus gamma times the best value in the cell it led to (just the reward if that cell ends the run), by a share alpha of the difference. q_learning runs episodes from the start, each until the run ends or 200 steps, and counts how they ended. q_policy is the greedy policy of the table.
//
// On the grid of 39-02 with gamma 0.95, alpha 0.1 and epsilon 0.1, the learned policy agreed with the optimal one in 12, 15 and 17 of the 17 cells after 100, 500 and 2000 episodes, and reached the goal on 48, 50 and 54 percent of test runs (the optimal policy: 54 percent). While learning for 2000 episodes it fell into a pit 759 times.

#ifndef RC_RC_LEARN_Q_LEARNING_HPP
#define RC_RC_LEARN_Q_LEARNING_HPP

#include <cmath>
#include <rc/learn/dataset.hpp>
#include <rc/learn/grid_task.hpp>
#include <rc/learn/value_iteration.hpp>
#include <vector>

namespace rc {
namespace learn {

struct QStats {
  int goals = 0;      // episodes that ended at the goal
  int pits = 0;       // in a pit
  int timeouts = 0;   // that ran out of steps
};

inline std::vector<double> make_q(const GridTask& g, double initial) {
  std::vector<double> Q(g.cells.size() * 4, initial);
  for (int s = 0; s < cell_count(g); ++s)
    if (is_terminal(g, s))
      for (int m = 0; m < 4; ++m) Q[static_cast<std::size_t>(s) * 4 + static_cast<std::size_t>(m)] = 0.0;
  return Q;
}

inline double best_value(const std::vector<double>& Q, int s) {
  const std::size_t b = static_cast<std::size_t>(s) * 4;
  double v = Q[b];
  for (std::size_t m = 1; m < 4; ++m) v = Q[b + m] > v ? Q[b + m] : v;
  return v;
}

inline int best_move(const std::vector<double>& Q, int s) {
  const std::size_t b = static_cast<std::size_t>(s) * 4;
  int best = 0;
  for (int m = 1; m < 4; ++m)
    if (Q[b + static_cast<std::size_t>(m)] > Q[b + static_cast<std::size_t>(best)]) best = m;
  return best;
}

// The greedy move, or with chance epsilon one of the four at random.
inline int choose_move(const std::vector<double>& Q, int s, double epsilon, Rng& r) {
  if (uniform(r) < epsilon) {
    int m = static_cast<int>(uniform(r) * 4.0);
    return m > 3 ? 3 : m;
  }
  return best_move(Q, s);
}

// One move in the task, its outcome drawn from chances the learner never sees.
inline int take_step(const GridTask& g, int s, int m, Rng& r) {
  double c[3];
  int n[3];
  outcomes(g, s, m, c, n);
  const double u = uniform(r);
  return u < c[0] ? n[0] : (u < c[0] + c[1] ? n[1] : n[2]);
}

// Nudges Q(s, m) toward reward + gamma max Q(next) (just the reward if next ends the run).
inline void q_update(std::vector<double>& Q, int s, int m, double reward, int next, bool ended, double alpha, double gamma) {
  const double target = reward + (ended ? 0.0 : gamma * best_value(Q, next));
  double& q = Q[static_cast<std::size_t>(s) * 4 + static_cast<std::size_t>(m)];
  q += alpha * (target - q);
}

inline std::vector<double> q_learning(const GridTask& g, int episodes, double alpha, double gamma, double epsilon, double initial,
                                      unsigned long long seed, QStats* stats) {
  std::vector<double> Q = make_q(g, initial);
  Rng r = make_rng(seed);
  QStats st;
  for (int e = 0; e < episodes; ++e) {
    int s = start_of(g);
    bool ended = false;
    for (int t = 0; t < 200 && !ended; ++t) {
      const int m = choose_move(Q, s, epsilon, r);
      const int next = take_step(g, s, m, r);
      ended = is_terminal(g, next);
      q_update(Q, s, m, g.step + reward_of(g, next), next, ended, alpha, gamma);
      s = next;
    }
    const char c = g.cells[static_cast<std::size_t>(s)];
    if (c == 'G') ++st.goals;
    else if (c == 'P') ++st.pits;
    else if (!ended) ++st.timeouts;
  }
  if (stats) *stats = st;
  return Q;
}

inline std::vector<int> q_policy(const GridTask& g, const std::vector<double>& Q) {
  std::vector<int> p(g.cells.size(), 0);
  for (int s = 0; s < cell_count(g); ++s) p[static_cast<std::size_t>(s)] = best_move(Q, s);
  return p;
}


}  // namespace learn
}  // namespace rc

#endif
