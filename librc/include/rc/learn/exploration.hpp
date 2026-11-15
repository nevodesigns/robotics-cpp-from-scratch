// rc/learn/exploration.hpp
//
// Exploring enough, from lesson 39-04.
//
// q_learning_visits is the Q-learning of 39-03 that also counts, for every cell, how many times a run was in it. cells_reached counts the cells visited at least once. greedy_return follows the greedy policy of a table from the start (the grid has no slip here, so one run says it all) for at most 200 steps and returns the undiscounted sum and where it ended.
//
// On a grid with a goal worth 3 two steps from the start and one worth 10 at the end of a six-step corridor, after 2000 episodes: with no random moves the learner reached the large goal 1 times, its greedy policy returned 1 and it was in the cell next to the large goal 2 times; with a tenth of the moves random, 2 times, 1 and 3 visits next to it; with half, 27 times, 1 and 44 visits. With the table started at 20 and no random moves at all, it reached the large goal 1970 times and its greedy policy returned 4, the best there is.

#ifndef RC_RC_LEARN_EXPLORATION_HPP
#define RC_RC_LEARN_EXPLORATION_HPP

#include <cmath>
#include <rc/learn/dataset.hpp>
#include <rc/learn/grid_task.hpp>
#include <rc/learn/q_learning.hpp>
#include <vector>

namespace rc {
namespace learn {

// Q-learning (39-03) that also counts, in `visits`, how many times a run was in each cell.
inline std::vector<double> q_learning_visits(const GridTask& g, int episodes, double alpha, double gamma, double epsilon, double initial,
                                             unsigned long long seed, std::vector<int>& visits, int* big_goals) {
  std::vector<double> Q = make_q(g, initial);
  visits.assign(g.cells.size(), 0);
  Rng r = make_rng(seed);
  int big = 0;
  for (int e = 0; e < episodes; ++e) {
    int s = start_of(g);
    ++visits[static_cast<std::size_t>(s)];
    bool ended = false;
    for (int t = 0; t < 200 && !ended; ++t) {
      const int m = choose_move(Q, s, epsilon, r);
      const int next = take_step(g, s, m, r);
      ended = is_terminal(g, next);
      q_update(Q, s, m, g.step + reward_of(g, next), next, ended, alpha, gamma);
      s = next;
      ++visits[static_cast<std::size_t>(s)];
    }
    if (g.cells[static_cast<std::size_t>(s)] == 'G') ++big;
  }
  if (big_goals) *big_goals = big;
  return Q;
}

inline int cells_reached(const std::vector<int>& visits) {
  int n = 0;
  for (int v : visits) n += v > 0 ? 1 : 0;
  return n;
}

// Follows the greedy policy of Q from the start, with no slip; returns the undiscounted sum.
inline double greedy_return(const GridTask& g, const std::vector<double>& Q, char* ended) {
  int s = start_of(g);
  double sum = 0.0;
  for (int t = 0; t < 200; ++t) {
    s = moved(g, s, best_move(Q, s));
    sum += g.step + reward_of(g, s);
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
