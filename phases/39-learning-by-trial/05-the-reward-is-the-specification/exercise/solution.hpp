#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <cstdlib>
#include <rc/learn/dataset.hpp>
#include <rc/learn/grid_task.hpp>
#include <rc/learn/q_learning.hpp>
#include <vector>

using rc::learn::Rng;
using rc::learn::make_rng;
using rc::learn::GridTask;
using rc::learn::make_grid;
using rc::learn::start_of;
using rc::learn::is_terminal;
using rc::learn::reward_of;
using rc::learn::moved;
using rc::learn::make_q;
using rc::learn::choose_move;
using rc::learn::take_step;
using rc::learn::q_update;
using rc::learn::best_move;

enum Shaping { kPlain = 0, kPotential = 1, kCloser = 2 };

inline int goal_distance(const GridTask& g, int s) {
  const int goal = static_cast<int>(g.cells.find('G'));
  return std::abs(s % g.width - goal % g.width) + std::abs(s / g.width - goal / g.width);
}

// What the learner is paid for a move from s to next.
inline double shaped_reward(const GridTask& g, int s, int next, int shaping, double gamma) {
  double r = g.step + reward_of(g, next);
  (void)s;  // TODO: kPlain pays r as it is; kPotential adds gamma times the potential of next (minus its goal_distance, or 0 if next ends the run) less the potential of s (minus its goal_distance); kCloser adds 1 when next is closer to the goal than s
  (void)next;
  (void)shaping;
  (void)gamma;
  return r;
}

inline std::vector<double> q_learning_shaped(const GridTask& g, int episodes, double alpha, double gamma, double epsilon, int shaping,
                                             unsigned long long seed) {
  std::vector<double> Q = make_q(g, 0.0);
  Rng r = make_rng(seed);
  for (int e = 0; e < episodes; ++e) {
    int s = start_of(g);
    bool ended = false;
    for (int t = 0; t < 200 && !ended; ++t) {
      const int m = choose_move(Q, s, epsilon, r);
      const int next = take_step(g, s, m, r);
      ended = is_terminal(g, next);
      q_update(Q, s, m, shaped_reward(g, s, next, shaping, gamma), next, ended, alpha, gamma);
      s = next;
    }
  }
  return Q;
}

// Steps the greedy policy takes from the start to a cell that ends the run (no slip), or `limit` if it never does.
inline int steps_to_goal(const GridTask& g, const std::vector<double>& Q, int limit) {
  int s = start_of(g);
  for (int t = 1; t <= limit; ++t) {
    s = moved(g, s, best_move(Q, s));
    if (is_terminal(g, s)) return t;
  }
  return limit;
}


#endif
