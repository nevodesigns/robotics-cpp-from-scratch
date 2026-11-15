// rc/learn/grid_task.hpp
//
// A task as states, actions and rewards, from lesson 39-01.
//
// GridTask is a grid of cells read from rows of text: '#' a wall, '.' a free cell, 'S' the start, 'G' the goal (worth +10, and the run ends there), 'P' a pit (worth -10, and the run ends), 'g' a small goal (worth +3, and the run ends). Every step costs `step` (usually -1). A move (north, east, south, west) goes where it is meant with chance 1 - 2 slip and to each side with chance slip; a move into a wall or off the grid stays put. outcomes gives the three results of a move with their chances; reward_of the reward for arriving in a cell. A policy is a move for every cell. evaluate_policy finds its value V(s), the expected sum of rewards discounted by gamma, by sweeping V(s) = sum over outcomes of chance times (reward + gamma V(next)) over every cell until no value changes by more than a tolerance; a policy that mixes the four moves equally (the random policy) has its own evaluate_random. run_policy runs a policy once from the start and returns the discounted sum it got.
//
// On the grid of this lesson (a corridor along a row of pits, a longer way round above, slip 0.1, step -1, gamma 0.95), the random policy's value at the start was -13.357, settled in 242 sweeps. A policy that always moves east along the pits had a value of -5.972; one that goes round above, -5.745. Running the east policy 20000 times gave a mean discounted return of -5.924, and it fell into a pit on 48.5 percent of the runs.

#ifndef RC_RC_LEARN_GRID_TASK_HPP
#define RC_RC_LEARN_GRID_TASK_HPP

#include <cmath>
#include <rc/learn/dataset.hpp>
#include <string>
#include <vector>

namespace rc {
namespace learn {

enum Move { kNorth = 0, kEast = 1, kSouth = 2, kWest = 3 };

struct GridTask {
  int width = 0, height = 0;
  std::string cells;      // row by row
  double step = -1.0;     // the reward for every step
  double slip = 0.1;      // the chance of going to each side instead
};

inline GridTask make_grid(const std::vector<std::string>& rows, double step, double slip) {
  GridTask g;
  g.height = static_cast<int>(rows.size());
  g.width = rows.empty() ? 0 : static_cast<int>(rows[0].size());
  for (const std::string& r : rows) g.cells += r;
  g.step = step;
  g.slip = slip;
  return g;
}

inline int cell_count(const GridTask& g) { return static_cast<int>(g.cells.size()); }
inline int start_of(const GridTask& g) { return static_cast<int>(g.cells.find('S')); }
inline bool is_wall(const GridTask& g, int s) { return g.cells[static_cast<std::size_t>(s)] == '#'; }
inline bool is_terminal(const GridTask& g, int s) {
  const char c = g.cells[static_cast<std::size_t>(s)];
  return c == 'G' || c == 'P' || c == 'g';
}

// The reward for arriving in s, beyond the step.
inline double reward_of(const GridTask& g, int s) {
  const char c = g.cells[static_cast<std::size_t>(s)];
  return c == 'G' ? 10.0 : (c == 'P' ? -10.0 : (c == 'g' ? 3.0 : 0.0));
}

// Where a move from s goes if it does not slip: the next cell, or s itself at a wall or the edge.
inline int moved(const GridTask& g, int s, int m) {
  const int dx[4] = {0, 1, 0, -1}, dy[4] = {-1, 0, 1, 0};
  const int x = s % g.width + dx[m], y = s / g.width + dy[m];
  if (x < 0 || y < 0 || x >= g.width || y >= g.height) return s;
  const int n = y * g.width + x;
  return is_wall(g, n) ? s : n;
}

// The three results of move m from s: as meant, and slipped to either side, with their chances.
inline void outcomes(const GridTask& g, int s, int m, double chance[3], int next[3]) {
  chance[0] = 1.0 - 2.0 * g.slip;
  next[0] = moved(g, s, m);
  chance[1] = g.slip;
  next[1] = moved(g, s, (m + 1) % 4);
  chance[2] = g.slip;
  next[2] = moved(g, s, (m + 3) % 4);
}

// The value of taking move m in s, given the values V of the next cells.
inline double move_value(const GridTask& g, const std::vector<double>& V, int s, int m, double gamma) {
  double c[3];
  int n[3];
  outcomes(g, s, m, c, n);
  double q = 0.0;
  for (int k = 0; k < 3; ++k) {
    const double future = is_terminal(g, n[k]) ? 0.0 : gamma * V[static_cast<std::size_t>(n[k])];
    q += c[k] * (g.step + reward_of(g, n[k]) + future);
  }
  return q;
}

// The value of following `policy` (a move for every cell), by sweeping until no value changes by more than tol.
inline std::vector<double> evaluate_policy(const GridTask& g, const std::vector<int>& policy, double gamma, double tol, int* sweeps) {
  std::vector<double> V(g.cells.size(), 0.0);
  int n = 0;
  for (n = 1; n <= 100000; ++n) {
    double change = 0.0;
    for (int s = 0; s < cell_count(g); ++s) {
      if (is_wall(g, s) || is_terminal(g, s)) continue;
      const double v = move_value(g, V, s, policy[static_cast<std::size_t>(s)], gamma);
      change = std::fabs(v - V[static_cast<std::size_t>(s)]) > change ? std::fabs(v - V[static_cast<std::size_t>(s)]) : change;
      V[static_cast<std::size_t>(s)] = v;
    }
    if (change < tol) break;
  }
  if (sweeps) *sweeps = n;
  return V;
}

// The value of the policy that takes each of the four moves with chance one quarter.
inline std::vector<double> evaluate_random(const GridTask& g, double gamma, double tol, int* sweeps) {
  std::vector<double> V(g.cells.size(), 0.0);
  int n = 0;
  for (n = 1; n <= 100000; ++n) {
    double change = 0.0;
    for (int s = 0; s < cell_count(g); ++s) {
      if (is_wall(g, s) || is_terminal(g, s)) continue;
      double v = 0.0;
      for (int m = 0; m < 4; ++m) v += 0.25 * move_value(g, V, s, m, gamma);
      change = std::fabs(v - V[static_cast<std::size_t>(s)]) > change ? std::fabs(v - V[static_cast<std::size_t>(s)]) : change;
      V[static_cast<std::size_t>(s)] = v;
    }
    if (change < tol) break;
  }
  if (sweeps) *sweeps = n;
  return V;
}

// One run of a policy from the start, at most max_steps; returns the discounted sum and where it ended ('-' if it did not).
inline double run_policy(const GridTask& g, const std::vector<int>& policy, double gamma, int max_steps, Rng& r, char* ended) {
  int s = start_of(g);
  double sum = 0.0, discount = 1.0;
  for (int t = 0; t < max_steps; ++t) {
    double c[3];
    int n[3];
    outcomes(g, s, policy[static_cast<std::size_t>(s)], c, n);
    const double u = uniform(r);
    s = u < c[0] ? n[0] : (u < c[0] + c[1] ? n[1] : n[2]);
    sum += discount * (g.step + reward_of(g, s));
    discount *= gamma;
    if (is_terminal(g, s)) {
      if (ended) *ended = g.cells[static_cast<std::size_t>(s)];
      return sum;
    }
  }
  if (ended) *ended = '-';
  return sum;
}


}  // namespace learn
}  // namespace rc

#endif
