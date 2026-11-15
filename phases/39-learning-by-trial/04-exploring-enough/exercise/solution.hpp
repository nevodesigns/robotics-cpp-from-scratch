#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <rc/learn/dataset.hpp>
#include <rc/learn/grid_task.hpp>
#include <rc/learn/q_learning.hpp>
#include <vector>

using rc::learn::Rng;
using rc::learn::make_rng;
using rc::learn::GridTask;
using rc::learn::make_grid;
using rc::learn::cell_count;
using rc::learn::start_of;
using rc::learn::is_wall;
using rc::learn::is_terminal;
using rc::learn::reward_of;
using rc::learn::moved;
using rc::learn::make_q;
using rc::learn::choose_move;
using rc::learn::take_step;
using rc::learn::q_update;
using rc::learn::best_move;

// Q-learning (39-03) that also counts, in `visits`, how many times a run was in each cell.
inline std::vector<double> q_learning_visits(const GridTask& g, int episodes, double alpha, double gamma, double epsilon, double initial,
                                             unsigned long long seed, std::vector<int>& visits, int* big_goals) {
  std::vector<double> Q = make_q(g, initial);
  visits.assign(g.cells.size(), 0);
  Rng r = make_rng(seed);
  int big = 0;
  (void)episodes;  // TODO: the loop of q_learning in 39-03 (each episode from the start, at most 200 steps, choose_move, take_step, q_update), counting the start and every cell arrived in into visits, and counting in `big` the episodes that ended on 'G'
  (void)alpha;
  (void)gamma;
  (void)epsilon;
  (void)r;
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


#endif
