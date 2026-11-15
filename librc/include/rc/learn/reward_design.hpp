// rc/learn/reward_design.hpp
//
// The reward is the specification, from lesson 39-05.
//
// goal_distance is the number of steps from a cell to the goal along rows and columns, ignoring walls. shaped_reward gives the reward a learner is paid for a move from s to next: kPlain is the task's own (the step and the arrival); kPotential adds gamma times the potential of next less the potential of s, with the potential minus goal_distance (and zero at a cell that ends the run); kCloser adds 1 whenever next is closer to the goal than s. q_learning_shaped is the Q-learning of 39-03 paid that reward. steps_to_goal follows the learned greedy policy (no slip) and gives how many steps it took to arrive, or the limit if it never did.
//
// On an open grid with no step cost and +10 at the goal, 4 steps from the start, after 20 episodes: the plain reward's policy 200, the potential's 4, the bonus's 200 (a limit of 200 means it never arrived). After 200 episodes: 8, 4 and 200. The bonus learner, run for 200 steps, collected 101 in bonus and never reached the goal.

#ifndef RC_RC_LEARN_REWARD_DESIGN_HPP
#define RC_RC_LEARN_REWARD_DESIGN_HPP

#include <cmath>
#include <cstdlib>
#include <rc/learn/dataset.hpp>
#include <rc/learn/grid_task.hpp>
#include <rc/learn/q_learning.hpp>
#include <vector>

namespace rc {
namespace learn {

enum Shaping { kPlain = 0, kPotential = 1, kCloser = 2 };

inline int goal_distance(const GridTask& g, int s) {
  const int goal = static_cast<int>(g.cells.find('G'));
  return std::abs(s % g.width - goal % g.width) + std::abs(s / g.width - goal / g.width);
}

// What the learner is paid for a move from s to next.
inline double shaped_reward(const GridTask& g, int s, int next, int shaping, double gamma) {
  double r = g.step + reward_of(g, next);
  if (shaping == kPotential) {
    const double after = is_terminal(g, next) ? 0.0 : -static_cast<double>(goal_distance(g, next));
    const double before = -static_cast<double>(goal_distance(g, s));
    r += gamma * after - before;
  } else if (shaping == kCloser) {
    if (goal_distance(g, next) < goal_distance(g, s)) r += 1.0;
  }
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


}  // namespace learn
}  // namespace rc

#endif
