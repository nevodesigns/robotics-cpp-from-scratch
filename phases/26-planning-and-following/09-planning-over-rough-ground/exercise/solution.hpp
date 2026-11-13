#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <queue>
#include <rc/drive/hybrid_astar.hpp>
#include <rc/field/terrain.hpp>
#include <vector>

using namespace rc::drive;

struct TerrainGrid {
  rc::perceive::GridSpec spec;
  std::vector<rc::field::Cell> cells;

  explicit TerrainGrid(const rc::perceive::GridSpec& s) : spec(s), cells(static_cast<std::size_t>(s.width) * static_cast<std::size_t>(s.height)) {}
  bool in_bounds(int i, int j) const { return i >= 0 && j >= 0 && i < spec.width && j < spec.height; }
  const rc::field::Cell& at(int i, int j) const { return cells[static_cast<std::size_t>(j) * static_cast<std::size_t>(spec.width) + static_cast<std::size_t>(i)]; }
  int cell_i(double x) const { return static_cast<int>(std::floor((x - spec.origin_x) / spec.resolution)); }
  int cell_j(double y) const { return static_cast<int>(std::floor((y - spec.origin_y) / spec.resolution)); }
  double centre_x(int i) const { return spec.origin_x + (i + 0.5) * spec.resolution; }
  double centre_y(int j) const { return spec.origin_y + (j + 0.5) * spec.resolution; }
  // Gives every cell whose centre is in the rectangle this surface and slope.
  void paint(double x0, double y0, double x1, double y1, const rc::field::Cell& cell) {
    for (int j = 0; j < spec.height; ++j)
      for (int i = 0; i < spec.width; ++i) {
        const double x = centre_x(i), y = centre_y(j);
        if (x >= x0 && x <= x1 && y >= y0 && y <= y1) cells[static_cast<std::size_t>(j) * static_cast<std::size_t>(spec.width) + static_cast<std::size_t>(i)] = cell;
      }
  }
};

struct OffroadWeights {
  double mass = 10.0;   // kg
  double energy = 0.0;  // per joule per metre
  double risk = 0.0;    // per unit of risk (the cell's cost over 255)
};

// The work done for each metre over a cell, heading a given way, J/m. Going downhill gives nothing back.
inline double energy_per_metre(const rc::field::Cell& c, double heading, double mass) {
  const rc::field::Traction t = rc::field::traction(c, heading);
  const double e = mass * 9.81 * (c.surface.crr * std::cos(t.angle) + std::sin(t.angle));
  return e > 0.0 ? e : 0.0;
}

// The price of a step from cell (i0, j0) to a neighbour (i1, j1), or a negative number if the step is not allowed.
inline double step_cost(const TerrainGrid& terrain, const rc::perceive::CostGrid* risk, int i0, int j0, int i1, int j1, const OffroadWeights& w) {
  if (!terrain.in_bounds(i1, j1)) return -1.0;
  if (risk != nullptr && risk->at(i1, j1) >= rc::perceive::kCostInscribed) return -1.0;
  const double dx = (i1 - i0) * terrain.spec.resolution, dy = (j1 - j0) * terrain.spec.resolution;
  const double dist = std::hypot(dx, dy);
  const double heading = std::atan2(dy, dx);
  double price = dist;
  double e = 0.0, r = 0.0;
  (void)r;  // TODO: the energy per metre is the mean of the two cells' (going the way of the step); the risk is the target cell's cost over 255; the price is dist * (1 + energy weight * energy + risk weight * risk)
  (void)e;
  return price;
}

struct OffroadPath {
  bool found = false;
  std::vector<CarPose> points;
  double length = 0.0;
  int expansions = 0;
};

// The cheapest path over the eight neighbours, with the straight distance as the guide (it never over-estimates, for weights of zero or more).
inline OffroadPath plan_offroad(const TerrainGrid& terrain, const rc::perceive::CostGrid* risk, double sx, double sy, double gx, double gy, const OffroadWeights& w) {
  OffroadPath out;
  const int W = terrain.spec.width, H = terrain.spec.height;
  const int si = terrain.cell_i(sx), sj = terrain.cell_j(sy), gi = terrain.cell_i(gx), gj = terrain.cell_j(gy);
  if (!terrain.in_bounds(si, sj) || !terrain.in_bounds(gi, gj)) return out;
  std::vector<double> g(static_cast<std::size_t>(W) * static_cast<std::size_t>(H), 1e18);
  std::vector<int> parent(g.size(), -1);
  using Entry = std::pair<double, int>;
  std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry> > open;
  const int start = sj * W + si, goal = gj * W + gi;
  g[static_cast<std::size_t>(start)] = 0.0;
  open.push(Entry(0.0, start));
  while (!open.empty()) {
    const Entry top = open.top();
    open.pop();
    const int cur = top.second;
    const int ci = cur % W, cj = cur / W;
    if (top.first > g[static_cast<std::size_t>(cur)] + std::hypot((gi - ci) * terrain.spec.resolution, (gj - cj) * terrain.spec.resolution) + 1e-9) continue;
    ++out.expansions;
    if (cur == goal) {
      out.found = true;
      for (int k = cur; k >= 0; k = parent[static_cast<std::size_t>(k)]) {
        CarPose p;
        p.x = terrain.centre_x(k % W);
        p.y = terrain.centre_y(k / W);
        out.points.push_back(p);
      }
      std::reverse(out.points.begin(), out.points.end());
      for (std::size_t k = 1; k < out.points.size(); ++k) out.length += std::hypot(out.points[k].x - out.points[k - 1].x, out.points[k].y - out.points[k - 1].y);
      return out;
    }
    for (int dj = -1; dj <= 1; ++dj)
      for (int di = -1; di <= 1; ++di) {
        if (di == 0 && dj == 0) continue;
        const int ni = ci + di, nj = cj + dj;
        const double price = step_cost(terrain, risk, ci, cj, ni, nj, w);
        if (price < 0.0) continue;
        const int nid = nj * W + ni;
        const double ng = g[static_cast<std::size_t>(cur)] + price;
        if (ng < g[static_cast<std::size_t>(nid)]) {
          g[static_cast<std::size_t>(nid)] = ng;
          parent[static_cast<std::size_t>(nid)] = cur;
          open.push(Entry(ng + std::hypot((gi - ni) * terrain.spec.resolution, (gj - nj) * terrain.spec.resolution), nid));
        }
      }
  }
  return out;
}

// The work a path costs a vehicle of this mass, in joules.
inline double path_energy(const TerrainGrid& terrain, const std::vector<CarPose>& path, double mass) {
  double total = 0.0;
  for (std::size_t k = 1; k < path.size(); ++k) {
    const double dx = path[k].x - path[k - 1].x, dy = path[k].y - path[k - 1].y;
    const double mx = 0.5 * (path[k].x + path[k - 1].x), my = 0.5 * (path[k].y + path[k - 1].y);
    total += std::hypot(dx, dy) * energy_per_metre(terrain.at(terrain.cell_i(mx), terrain.cell_j(my)), std::atan2(dy, dx), mass);
  }
  return total;
}

// The worst cell cost along a path.
inline double path_max_risk(const rc::perceive::CostGrid& risk, const std::vector<CarPose>& path) {
  double worst = 0.0;
  for (std::size_t k = 0; k < path.size(); ++k) worst = std::max(worst, static_cast<double>(risk.at(risk.cell_i(path[k].x), risk.cell_j(path[k].y))));
  return worst;
}


#endif
