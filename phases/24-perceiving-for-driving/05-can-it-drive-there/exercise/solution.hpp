#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <rc/field/terrain.hpp>
#include <rc/perceive/costmap.hpp>
#include <rc/vehicle/drive_train.hpp>
#include <vector>

using namespace rc::perceive;

// The height of the ground in each cell of a grid. NaN where it is not known.
struct HeightGrid {
  GridSpec spec;
  std::vector<double> z;

  explicit HeightGrid(const GridSpec& s) : spec(s), z(static_cast<std::size_t>(s.width) * static_cast<std::size_t>(s.height), std::numeric_limits<double>::quiet_NaN()) {}
  bool in_bounds(int i, int j) const { return i >= 0 && j >= 0 && i < spec.width && j < spec.height; }
  double at(int i, int j) const { return in_bounds(i, j) ? z[static_cast<std::size_t>(j) * static_cast<std::size_t>(spec.width) + static_cast<std::size_t>(i)] : std::numeric_limits<double>::quiet_NaN(); }
  void set(int i, int j, double h) {
    if (in_bounds(i, j)) z[static_cast<std::size_t>(j) * static_cast<std::size_t>(spec.width) + static_cast<std::size_t>(i)] = h;
  }
};

struct SlopeRoughness {
  bool known = false;
  double slope = 0.0;      // radians
  double roughness = 0.0;  // metres: the spread of the heights about the plane
  double max_gradient = 0.0;  // radians: the steepest rise between the cell and any of its eight neighbours
};

// The plane through the 3 by 3 cells round (i, j) by least squares, and how far the cells are from it.
inline SlopeRoughness slope_and_roughness(const HeightGrid& g, int i, int j) {
  SlopeRoughness r;
  double h[3][3];
  for (int dj = -1; dj <= 1; ++dj)
    for (int di = -1; di <= 1; ++di) {
      h[dj + 1][di + 1] = g.at(i + di, j + dj);
      if (!(h[dj + 1][di + 1] == h[dj + 1][di + 1])) return r;
    }
  (void)h;  // TODO: the slope of x is the sum of dx times the height over 6 times the cell, likewise for y; the plane's height at the middle is the mean; the roughness is the root of the sum of squared differences from the plane over 6 (9 cells less 3 for the plane); the largest gradient is the largest height difference to a neighbour over the distance to it, as an angle
  r.known = true;
  return r;
}

// What a vehicle can drive over.
struct VehicleLimits {
  double max_slope = 0.0;      // radians
  double max_roughness = 0.0;  // metres
};

// The smallest of three slopes: what the motors can start on, what the tyres hold on this surface, and where the vehicle tips over sideways (its half track over the height of its centre of mass); and a roughness of half a wheel radius.
inline VehicleLimits vehicle_limits(const rc::vehicle::DriveTrain& drive, const rc::field::Surface& surface, double track, double cg_height) {
  VehicleLimits v;
  const double motors = rc::vehicle::max_climb(drive, surface.crr);
  const double grip = rc::field::max_climb_angle(surface);
  const double tip = std::atan(0.5 * track / cg_height);
  v.max_slope = std::min(motors, std::min(grip, tip));
  v.max_roughness = 0.5 * drive.wheel_radius;
  return v;
}

// The cost of a cell: the larger of the shares of the two limits, squared and scaled to 252; lethal at or beyond a limit; unknown if the cell is.
inline unsigned char traversability_cost(const SlopeRoughness& s, const VehicleLimits& v) {
  if (!s.known) return kCostUnknown;
  const double share = std::max(std::max(s.slope, s.max_gradient) / v.max_slope, s.roughness / v.max_roughness);
  if (share >= 1.0) return kCostLethal;
  return static_cast<unsigned char>(252.0 * share * share);
}

inline CostGrid traversability_layer(const HeightGrid& g, const VehicleLimits& v) {
  CostGrid layer(g.spec, kCostUnknown);
  for (int j = 0; j < g.spec.height; ++j)
    for (int i = 0; i < g.spec.width; ++i) layer.set(i, j, traversability_cost(slope_and_roughness(g, i, j), v));
  return layer;
}


#endif
