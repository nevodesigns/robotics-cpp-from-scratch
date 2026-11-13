#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <cstddef>
#include <rc/arm/motion_check.hpp>
#include <vector>

using namespace rc::arm;

struct PlanRng {
  unsigned long long s;
  explicit PlanRng(unsigned long long seed) : s(seed * 2862933555777941757ULL + 3037000493ULL) {}
  double uniform() {
    s = s * 6364136223846793005ULL + 1442695040888963407ULL;
    return static_cast<double>((s >> 33) & 0xFFFFFFULL) / 16777216.0;
  }
};

struct PlanProblem {
  ArmCheck check;
  std::vector<double> bounds;  // the sweep bounds of 30-02
  double eta = 0.5;            // rad, the longest step
  double goal_bias = 0.1;      // the share of samples that are the goal
  double margin = 0.02;        // m
  int max_iter = 3000;
  unsigned long long seed = 1;
  mutable int checks = 0;      // the poses looked at so far, over every plan made with this problem
};

struct PlanResult {
  bool found = false;
  std::vector<std::vector<double> > path;
  int iterations = 0;
  int nodes = 0;
  int checks = 0;  // poses looked at by this plan
};

inline double joint_distance(const std::vector<double>& a, const std::vector<double>& b) {
  double s = 0.0;
  for (std::size_t i = 0; i < a.size(); ++i) s += (a[i] - b[i]) * (a[i] - b[i]);
  return std::sqrt(s);
}

// From a configuration towards another, by at most eta (it reaches it if it is nearer than that).
inline std::vector<double> steer(const std::vector<double>& from, const std::vector<double>& to, double eta) {
  const double d = joint_distance(from, to);
  (void)d;  // TODO: if the target is within eta return it; otherwise the point eta along the straight line from `from` to it
  return from;
}

struct Tree {
  std::vector<std::vector<double> > q;
  std::vector<int> parent;
  void add(const std::vector<double>& c, int p) {
    q.push_back(c);
    parent.push_back(p);
  }
  int nearest(const std::vector<double>& c) const {
    int best = 0;
    double bd = 1e18;
    for (std::size_t i = 0; i < q.size(); ++i) {
      const double d = joint_distance(q[i], c);
      if (d < bd) {
        bd = d;
        best = static_cast<int>(i);
      }
    }
    return best;
  }
  std::vector<std::vector<double> > path_to(int node) const {
    std::vector<std::vector<double> > p;
    for (int k = node; k >= 0; k = parent[static_cast<std::size_t>(k)]) p.push_back(q[static_cast<std::size_t>(k)]);
    std::reverse(p.begin(), p.end());
    return p;
  }
};

inline std::vector<double> sample_config(const ArmTable& t, PlanRng& rng) {
  std::vector<double> q(t.rows.size());
  for (std::size_t i = 0; i < q.size(); ++i) q[i] = t.rows[i].lo + (t.rows[i].hi - t.rows[i].lo) * rng.uniform();
  return q;
}

enum ExtendStatus { kTrapped = 0, kAdvanced = 1, kReached = 2 };

// Grows the tree one step towards a target; the new node is the last in the tree if it advanced or reached.
inline ExtendStatus extend(const PlanProblem& p, Tree& tree, const std::vector<double>& target) {
  const int near = tree.nearest(target);
  const std::vector<double> nq = steer(tree.q[static_cast<std::size_t>(near)], target, p.eta);
  if (joint_distance(nq, tree.q[static_cast<std::size_t>(near)]) < 1e-12) return kTrapped;  // a step that goes nowhere is a blocked one (and keeps a loop that joins two trees from running for ever)
  const EdgeResult er = edge_conservative(p.check, tree.q[static_cast<std::size_t>(near)], nq, p.bounds, p.margin);
  p.checks += er.checks;
  if (!er.free) return kTrapped;
  tree.add(nq, near);
  return joint_distance(nq, target) < 1e-9 ? kReached : kAdvanced;
}

// One tree from the start; each new node is tried against the goal.
inline PlanResult rrt(const PlanProblem& p, const std::vector<double>& start, const std::vector<double>& goal) {
  PlanResult out;
  PlanRng rng(p.seed);
  const int c0 = p.checks;
  if (!free_pose(p.check, start, p.margin) || !free_pose(p.check, goal, p.margin)) return out;
  Tree tree;
  tree.add(start, -1);
  for (int it = 1; it <= p.max_iter; ++it) {
    out.iterations = it;
    const std::vector<double> target = rng.uniform() < p.goal_bias ? goal : sample_config(*p.check.table, rng);
    if (extend(p, tree, target) == kTrapped) continue;
    const int last = static_cast<int>(tree.q.size()) - 1;
    if (joint_distance(tree.q[static_cast<std::size_t>(last)], goal) <= p.eta) {
      const EdgeResult join = edge_conservative(p.check, tree.q[static_cast<std::size_t>(last)], goal, p.bounds, p.margin);
      p.checks += join.checks;
      if (join.free) {
        out.path = tree.path_to(last);
        out.path.push_back(goal);
        out.found = true;
        break;
      }
    }
  }
  out.nodes = static_cast<int>(tree.q.size());
  out.checks = p.checks - c0;
  return out;
}

// A tree from each end; the other is extended towards each new node until it joins or is blocked, and the trees swap.
inline PlanResult rrt_connect(const PlanProblem& p, const std::vector<double>& start, const std::vector<double>& goal) {
  PlanResult out;
  PlanRng rng(p.seed);
  const int c0 = p.checks;
  if (!free_pose(p.check, start, p.margin) || !free_pose(p.check, goal, p.margin)) return out;
  Tree ta, tb;
  ta.add(start, -1);
  tb.add(goal, -1);
  bool a_is_start = true;
  for (int it = 1; it <= p.max_iter; ++it) {
    out.iterations = it;
    const std::vector<double> target = sample_config(*p.check.table, rng);
    if (extend(p, ta, target) != kTrapped) {
      const std::vector<double> newq = ta.q.back();
      ExtendStatus st = kAdvanced;
      while (st == kAdvanced) st = extend(p, tb, newq);
      if (st == kReached) {
        std::vector<std::vector<double> > pa = ta.path_to(static_cast<int>(ta.q.size()) - 1);
        std::vector<std::vector<double> > pb = tb.path_to(static_cast<int>(tb.q.size()) - 1);
        std::reverse(pb.begin(), pb.end());
        pa.insert(pa.end(), pb.begin() + 1, pb.end());
        if (!a_is_start) std::reverse(pa.begin(), pa.end());
        out.path = pa;
        out.found = true;
        out.nodes = static_cast<int>(ta.q.size() + tb.q.size());
        out.checks = p.checks - c0;
        return out;
      }
    }
    std::swap(ta, tb);
    a_is_start = !a_is_start;
  }
  out.nodes = static_cast<int>(ta.q.size() + tb.q.size());
  out.checks = p.checks - c0;
  return out;
}

// The length of a path in joint space.
inline double path_length(const std::vector<std::vector<double> >& path) {
  double s = 0.0;
  for (std::size_t i = 1; i < path.size(); ++i) s += joint_distance(path[i - 1], path[i]);
  return s;
}


#endif
