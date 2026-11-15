#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <rc/learn/dataset.hpp>
#include <rc/learn/grid_task.hpp>
#include <vector>

using rc::learn::Rng;
using rc::learn::make_rng;
using rc::learn::GridTask;
using rc::learn::make_grid;
using rc::learn::cell_count;
using rc::learn::start_of;
using rc::learn::is_wall;
using rc::learn::is_terminal;
using rc::learn::move_value;
using rc::learn::evaluate_policy;
using rc::learn::evaluate_random;
using rc::learn::run_policy;
using rc::learn::kNorth;
using rc::learn::kEast;

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


#endif
