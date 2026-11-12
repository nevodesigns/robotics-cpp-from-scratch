// rc/perceive/costmap.hpp
//
// Costmap layers, from lesson 24-04.
//
// A CostGrid is cells of cost: 0 is free, 253 is inscribed (the robot's centre there is inside its own radius of an obstacle), 254 is lethal (an obstacle is there) and 255 is unknown, with 1 to 252 a gradient between. A layer writes its cells and the master takes the highest cost of the layers; inflation is applied last to the master, so that it spreads round every lethal cell from every other layer. rasterise_world draws the walls and posts of a lesson-22-06 world into a static layer; mark_and_clear takes the points a sensor saw from a position, marks the cells they fall in and clears the cells the beams passed through; inflate gives every cell within an inflation radius of a lethal one the cost 253 inside the inscribed radius and 252 exp(-k (d - r)) beyond it.
//
// A cell of 0.05 m: a wall of 4 m was 81 lethal cells and a post of 0.15 m radius 32; the inflation gave 253 inside 0.25 m, 216 at 0.30 m, 119 at 0.50 m and 30 at 1.00 m; a gap of 0.60 m between two posts left 2 free cells across it and a gap of 0.40 m left 0; and an obstacle that went away left 7 cells in a layer that only marks and 0 in one that clears.

#ifndef RC_RC_PERCEIVE_COSTMAP_HPP
#define RC_RC_PERCEIVE_COSTMAP_HPP

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <rc/field/frames.hpp>
#include <rc/field/sensors.hpp>
#include <utility>
#include <vector>

namespace rc {
namespace perceive {

// Cost values after the convention of the ROS costmaps.
const unsigned char kCostFree = 0;
const unsigned char kCostInscribed = 253;
const unsigned char kCostLethal = 254;
const unsigned char kCostUnknown = 255;

struct GridSpec {
  double origin_x = 0.0;  // the world position of the corner of cell (0, 0)
  double origin_y = 0.0;
  double resolution = 0.05;
  int width = 0;
  int height = 0;
};

class CostGrid {
 public:
  explicit CostGrid(const GridSpec& spec, unsigned char fill = kCostFree) : spec_(spec), cost_(static_cast<std::size_t>(spec.width) * static_cast<std::size_t>(spec.height), fill) {}

  const GridSpec& spec() const { return spec_; }
  bool in_bounds(int i, int j) const { return i >= 0 && j >= 0 && i < spec_.width && j < spec_.height; }
  unsigned char at(int i, int j) const { return in_bounds(i, j) ? cost_[index(i, j)] : kCostUnknown; }
  void set(int i, int j, unsigned char c) {
    if (in_bounds(i, j)) cost_[index(i, j)] = c;
  }
  int cell_i(double x) const { return static_cast<int>(std::floor((x - spec_.origin_x) / spec_.resolution)); }
  int cell_j(double y) const { return static_cast<int>(std::floor((y - spec_.origin_y) / spec_.resolution)); }
  double centre_x(int i) const { return spec_.origin_x + (static_cast<double>(i) + 0.5) * spec_.resolution; }
  double centre_y(int j) const { return spec_.origin_y + (static_cast<double>(j) + 0.5) * spec_.resolution; }
  std::size_t count(unsigned char c) const { return static_cast<std::size_t>(std::count(cost_.begin(), cost_.end(), c)); }
  void fill(unsigned char c) { std::fill(cost_.begin(), cost_.end(), c); }

  // The layer takes the higher of the two costs, cell by cell (unknown is higher than all).
  void merge_max(const CostGrid& other) {
    for (std::size_t k = 0; k < cost_.size() && k < other.cost_.size(); ++k) cost_[k] = std::max(cost_[k], other.cost_[k]);
  }

 private:
  std::size_t index(int i, int j) const { return static_cast<std::size_t>(j) * static_cast<std::size_t>(spec_.width) + static_cast<std::size_t>(i); }
  GridSpec spec_;
  std::vector<unsigned char> cost_;
};

// The static layer: the walls and the posts of a world, as lethal cells.
inline void rasterise_world(CostGrid& layer, const rc::field::World2& world) {
  const double step = 0.5 * layer.spec().resolution;
  for (std::size_t w = 0; w < world.walls.size(); ++w) {
    const rc::field::Segment& s = world.walls[w];
    const double len = std::hypot(s.x1 - s.x0, s.y1 - s.y0);
    const int n = static_cast<int>(std::ceil(len / step));
    for (int k = 0; k <= n; ++k) {
      const double u = n > 0 ? static_cast<double>(k) / n : 0.0;
      layer.set(layer.cell_i(s.x0 + u * (s.x1 - s.x0)), layer.cell_j(s.y0 + u * (s.y1 - s.y0)), kCostLethal);
    }
  }
  for (std::size_t p = 0; p < world.posts.size(); ++p) {
    const rc::field::Circle& c = world.posts[p];
    for (int j = layer.cell_j(c.y - c.r); j <= layer.cell_j(c.y + c.r); ++j)
      for (int i = layer.cell_i(c.x - c.r); i <= layer.cell_i(c.x + c.r); ++i)
        if (std::hypot(layer.centre_x(i) - c.x, layer.centre_y(j) - c.y) <= c.r) layer.set(i, j, kCostLethal);
  }
}

// Marks the cells that points fall in, and clears the cells along the beam from the sensor to each point (the point's own cell excepted), so that something that has gone is forgotten. The points are in the world frame; the sensor is at (sx, sy).
inline void mark_and_clear(CostGrid& layer, double sx, double sy, const std::vector<rc::field::Point3>& hits, bool clear) {
  for (std::size_t h = 0; h < hits.size(); ++h) {
    const int hi = layer.cell_i(hits[h].x);
    const int hj = layer.cell_j(hits[h].y);
    if (clear) {
      const double len = std::hypot(hits[h].x - sx, hits[h].y - sy);
      const int n = static_cast<int>(std::ceil(len / (0.5 * layer.spec().resolution)));
      for (int k = 0; k < n; ++k) {
        const double u = static_cast<double>(k) / n;
        const int ci = layer.cell_i(sx + u * (hits[h].x - sx));
        const int cj = layer.cell_j(sy + u * (hits[h].y - sy));
        if (ci != hi || cj != hj) layer.set(ci, cj, kCostFree);
      }
    }
    layer.set(hi, hj, kCostLethal);
  }
}

// Spreads cost round every lethal cell: inside the inscribed radius the cost is 253 (the centre of the robot cannot be there), and beyond it 252 * exp(-scale * (d - inscribed)) out to the inflation radius. A cell keeps the higher of its own cost and what it is given.
inline void inflate(CostGrid& master, double inscribed_radius, double inflation_radius, double scale) {
  const CostGrid source = master;
  const int reach = static_cast<int>(std::ceil(inflation_radius / master.spec().resolution));
  const double res = master.spec().resolution;
  for (int j = 0; j < master.spec().height; ++j) {
    for (int i = 0; i < master.spec().width; ++i) {
      if (source.at(i, j) != kCostLethal) continue;
      for (int dj = -reach; dj <= reach; ++dj) {
        for (int di = -reach; di <= reach; ++di) {
          if (!master.in_bounds(i + di, j + dj)) continue;
          const double d = res * std::hypot(static_cast<double>(di), static_cast<double>(dj));
          if (d > inflation_radius) continue;
          unsigned char c;
          if (d <= inscribed_radius) c = kCostInscribed;
          else c = static_cast<unsigned char>(252.0 * std::exp(-scale * (d - inscribed_radius)));
          if (source.at(i + di, j + dj) == kCostLethal) continue;
          if (c > master.at(i + di, j + dj)) master.set(i + di, j + dj, c);
        }
      }
    }
  }
}


}  // namespace perceive
}  // namespace rc

#endif
