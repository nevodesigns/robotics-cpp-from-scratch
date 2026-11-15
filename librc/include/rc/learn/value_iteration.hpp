// rc/learn/value_iteration.hpp
//
// The best policy when the model is known, from lesson 39-02.
//
// value_iteration sweeps V(s) = max over the four moves of move_value (39-01) over every cell until no value changes by more than a tolerance; the result is the optimal value. greedy_move picks, in a cell, the move with the highest value under V (the first of equals), and greedy_policy does it for every cell. outcome_stats runs a policy many times from the start and gives the share of runs that reached the goal and the mean undiscounted return.
//
// On the grid of 39-01 with gamma 0.95, value iteration settled in 36 sweeps with an optimal value at the start of -5.245, against -5.745 for going round and -13.357 for the random policy; the greedy policy's own value equalled it. With gamma 0.80 the greedy move at the start was north (round above): 95 percent of runs reached the goal, with a mean undiscounted return of -5.13. With gamma 0.95 it was east (the corridor beside the pits): 54 percent reached the goal, mean return -5.16.

#ifndef RC_RC_LEARN_VALUE_ITERATION_HPP
#define RC_RC_LEARN_VALUE_ITERATION_HPP

#include <cmath>
#include <rc/learn/dataset.hpp>
#include <rc/learn/grid_task.hpp>
#include <vector>

namespace rc {
namespace learn {

// The optimal value: sweep V(s) = max over moves of move_value until no value changes by more than tol.
inline std::vector<double> value_iteration(const GridTask& g, double gamma, double tol, int* sweeps) {
  std::vector<double> V(g.cells.size(), 0.0);
  int n = 0;
  for (n = 1; n <= 100000; ++n) {
    double change = 0.0;
    for (int s = 0; s < cell_count(g); ++s) {
      if (is_wall(g, s) || is_terminal(g, s)) continue;
      double best = move_value(g, V, s, 0, gamma);
      for (int m = 1; m < 4; ++m) {
        const double q = move_value(g, V, s, m, gamma);
        if (q > best) best = q;
      }
      const double d = std::fabs(best - V[static_cast<std::size_t>(s)]);
      if (d > change) change = d;
      V[static_cast<std::size_t>(s)] = best;
    }
    if (change < tol) break;
  }
  if (sweeps) *sweeps = n;
  return V;
}

// The move with the highest value in s under V (the first of equals).
inline int greedy_move(const GridTask& g, const std::vector<double>& V, int s, double gamma) {
  int best = 0;
  double bv = move_value(g, V, s, 0, gamma);
  for (int m = 1; m < 4; ++m) {
    const double q = move_value(g, V, s, m, gamma);
    if (q > bv + 1e-12) {
      bv = q;
      best = m;
    }
  }
  return best;
}

inline std::vector<int> greedy_policy(const GridTask& g, const std::vector<double>& V, double gamma) {
  std::vector<int> p(g.cells.size(), 0);
  for (int s = 0; s < cell_count(g); ++s)
    if (!is_wall(g, s) && !is_terminal(g, s)) p[static_cast<std::size_t>(s)] = greedy_move(g, V, s, gamma);
  return p;
}

struct OutcomeStats {
  double goal_share = 0.0;   // runs that ended at the goal
  double mean_return = 0.0;  // undiscounted
};

inline OutcomeStats outcome_stats(const GridTask& g, const std::vector<int>& policy, int runs, unsigned long long seed) {
  Rng r = make_rng(seed);
  OutcomeStats o;
  int goals = 0;
  double sum = 0.0;
  for (int i = 0; i < runs; ++i) {
    char ended = '-';
    sum += run_policy(g, policy, 1.0, 200, r, &ended);
    if (ended == 'G') ++goals;
  }
  o.goal_share = runs > 0 ? static_cast<double>(goals) / runs : 0.0;
  o.mean_return = runs > 0 ? sum / runs : 0.0;
  return o;
}


}  // namespace learn
}  // namespace rc

#endif
