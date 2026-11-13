// rc/arm/roadmap.hpp
//
// A map of where the arm can go, from lesson 30-04.
//
// build_roadmap draws random configurations inside the joint limits, keeps the free ones, and joins each to its nearest few by an edge if the edge is free by conservative advancement (30-02); it counts the poses it looked at. components says how many separate pieces the graph has. roadmap_query joins the start and the goal to their nearest nodes (by free edges), searches the graph for the shortest path with Dijkstra's method, and gives the path, its length in joint space and the number of poses it looked at; if the start or the goal cannot be joined, or they are in different pieces, it says there is no path. edge_free_fraction checks every edge of a roadmap against a (possibly changed) scene and says what share is still free.
//
// A roadmap of 250 nodes and 8 neighbours looked at 56406 poses to build and had 5 piece1. Five queries took 858 poses each on average against 2432 for RRT-Connect, and found paths 0.74 times the length of the RRT-Connect ones. With 40, 120 and 300 nodes the roadmap answered 7, 7 and 7 of the 8 queries (one pair had an end that was not free), and its graph had 6 pieces at the smallest size and 5 at the largest. After a box was put in the scene 34 percent of the roadmap's edges were not free, and a query on the old roadmap returned paths with 12 bad edges.

#ifndef RC_RC_ARM_ROADMAP_HPP
#define RC_RC_ARM_ROADMAP_HPP

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <queue>
#include <rc/arm/rrt.hpp>
#include <utility>
#include <vector>

namespace rc {
namespace arm {

struct Roadmap {
  std::vector<std::vector<double> > q;
  std::vector<std::vector<std::pair<int, double> > > adj;  // neighbour and the length of the edge
  int edges = 0;
  int checks = 0;  // poses looked at while building
};

// A roadmap: n random free configurations, each joined to its k nearest by a free edge.
inline Roadmap build_roadmap(const PlanProblem& p, int n, int k, unsigned long long seed) {
  Roadmap r;
  PlanRng rng(seed);
  while (static_cast<int>(r.q.size()) < n) {
    const std::vector<double> c = sample_config(*p.check.table, rng);
    ++r.checks;
    if (free_pose(p.check, c, p.margin)) r.q.push_back(c);
  }
  r.adj.assign(r.q.size(), std::vector<std::pair<int, double> >());
  for (std::size_t i = 0; i < r.q.size(); ++i) {
    std::vector<std::pair<double, int> > near;
    for (std::size_t j = 0; j < r.q.size(); ++j)
      if (j != i) near.push_back(std::make_pair(joint_distance(r.q[i], r.q[j]), static_cast<int>(j)));
    std::sort(near.begin(), near.end());
    for (int m = 0; m < k && m < static_cast<int>(near.size()); ++m) {
      const int j = near[static_cast<std::size_t>(m)].second;
      bool have = false;
      for (std::size_t e = 0; e < r.adj[i].size(); ++e)
        if (r.adj[i][e].first == j) have = true;
      if (have) continue;
      const EdgeResult er = edge_conservative(p.check, r.q[i], r.q[static_cast<std::size_t>(j)], p.bounds, p.margin);
      r.checks += er.checks;
      if (er.free) {
        const double d = near[static_cast<std::size_t>(m)].first;
        r.adj[i].push_back(std::make_pair(j, d));
        r.adj[static_cast<std::size_t>(j)].push_back(std::make_pair(static_cast<int>(i), d));
        ++r.edges;
      }
    }
  }
  return r;
}

// The number of separate pieces of the graph.
inline int components(const Roadmap& r) {
  std::vector<int> seen(r.q.size(), 0);
  int count = 0;
  for (std::size_t s = 0; s < r.q.size(); ++s) {
    if (seen[s]) continue;
    ++count;
    std::vector<int> stack(1, static_cast<int>(s));
    seen[s] = 1;
    while (!stack.empty()) {
      const int u = stack.back();
      stack.pop_back();
      for (std::size_t e = 0; e < r.adj[static_cast<std::size_t>(u)].size(); ++e) {
        const int v = r.adj[static_cast<std::size_t>(u)][e].first;
        if (!seen[static_cast<std::size_t>(v)]) {
          seen[static_cast<std::size_t>(v)] = 1;
          stack.push_back(v);
        }
      }
    }
  }
  return count;
}

struct QueryResult {
  bool found = false;
  std::vector<std::vector<double> > path;
  double length = 0.0;
  int checks = 0;
};

// Joins the start and the goal to their nearest nodes (up to k of each, by free edges) and finds the shortest path through the roadmap.
inline QueryResult roadmap_query(const PlanProblem& p, const Roadmap& r, const std::vector<double>& start, const std::vector<double>& goal, int k) {
  QueryResult out;
  if (!free_pose(p.check, start, p.margin) || !free_pose(p.check, goal, p.margin)) return out;
  const int n = static_cast<int>(r.q.size());
  const int S = n, G = n + 1;
  std::vector<std::vector<std::pair<int, double> > > adj = r.adj;
  adj.resize(static_cast<std::size_t>(n) + 2);
  for (int which = 0; which < 2; ++which) {
    const std::vector<double>& end = which == 0 ? start : goal;
    const int id = which == 0 ? S : G;
    std::vector<std::pair<double, int> > near;
    for (int j = 0; j < n; ++j) near.push_back(std::make_pair(joint_distance(end, r.q[static_cast<std::size_t>(j)]), j));
    std::sort(near.begin(), near.end());
    int joined = 0;
    for (std::size_t m = 0; m < near.size() && joined < k; ++m) {
      const int j = near[m].second;
      const EdgeResult er = edge_conservative(p.check, end, r.q[static_cast<std::size_t>(j)], p.bounds, p.margin);
      out.checks += er.checks;
      if (er.free) {
        adj[static_cast<std::size_t>(id)].push_back(std::make_pair(j, near[m].first));
        adj[static_cast<std::size_t>(j)].push_back(std::make_pair(id, near[m].first));
        ++joined;
      }
    }
  }
  std::vector<double> dist(static_cast<std::size_t>(n) + 2, 1e18);
  std::vector<int> prev(static_cast<std::size_t>(n) + 2, -1);
  using Item = std::pair<double, int>;
  std::priority_queue<Item, std::vector<Item>, std::greater<Item> > open;
  dist[static_cast<std::size_t>(S)] = 0.0;
  open.push(Item(0.0, S));
  while (!open.empty()) {
    const Item top = open.top();
    open.pop();
    const int u = top.second;
    if (top.first > dist[static_cast<std::size_t>(u)]) continue;
    if (u == G) break;
    for (std::size_t e = 0; e < adj[static_cast<std::size_t>(u)].size(); ++e) {
      const int v = adj[static_cast<std::size_t>(u)][e].first;
      const double nd = dist[static_cast<std::size_t>(u)] + adj[static_cast<std::size_t>(u)][e].second;
      if (nd < dist[static_cast<std::size_t>(v)]) {
        dist[static_cast<std::size_t>(v)] = nd;
        prev[static_cast<std::size_t>(v)] = u;
        open.push(Item(nd, v));
      }
    }
  }
  if (dist[static_cast<std::size_t>(G)] >= 1e17) return out;
  out.found = true;
  out.length = dist[static_cast<std::size_t>(G)];
  std::vector<int> ids;
  for (int u = G; u >= 0; u = prev[static_cast<std::size_t>(u)]) ids.push_back(u);
  std::reverse(ids.begin(), ids.end());
  for (std::size_t m = 0; m < ids.size(); ++m) {
    const int u = ids[m];
    out.path.push_back(u == S ? start : (u == G ? goal : r.q[static_cast<std::size_t>(u)]));
  }
  return out;
}

// The share of the roadmap's edges that are still free in a scene (the one in `p`, which may have changed since the roadmap was built).
inline double edge_free_fraction(const PlanProblem& p, const Roadmap& r) {
  int total = 0, ok = 0;
  for (std::size_t i = 0; i < r.q.size(); ++i)
    for (std::size_t e = 0; e < r.adj[i].size(); ++e) {
      const int j = r.adj[i][e].first;
      if (j < static_cast<int>(i)) continue;
      ++total;
      if (edge_conservative(p.check, r.q[i], r.q[static_cast<std::size_t>(j)], p.bounds, 0.0).free) ++ok;
    }
  return total ? static_cast<double>(ok) / total : 1.0;
}


}  // namespace arm
}  // namespace rc

#endif
