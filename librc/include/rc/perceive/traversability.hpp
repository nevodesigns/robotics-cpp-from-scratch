// rc/perceive/traversability.hpp
//
// Traversability, from lesson 24-05.
//
// A HeightGrid is the height of the ground in each cell (not a number where unknown). slope_and_roughness fits a plane to the 3 by 3 cells round a cell and gives the slope of that plane, the spread of the heights about it and the steepest rise between the cell and any neighbour (a plane smooths a step away; the rise between two cells does not). VehicleLimits is the steepest slope the vehicle can take (the smallest of what its motors can climb, what its tyres grip on the surface and the slope at which it tips over sideways) and the roughest it can cross (a share of its wheel radius). traversability_layer gives each cell a cost from the largest of the shares of the limits (the plane's slope and the steepest rise to a neighbour against the slope limit, the roughness against the roughness limit): 0 for flat smooth ground, rising with the square of the share to 252, and lethal at or beyond the limit; a cell with an unknown neighbour is unknown.
//
// The reference vehicle on gravel could take 22.3 degrees (its motors could climb 25.6 and the gravel held 22.3) and a roughness of 0.025 m; a ramp of 10 degrees was read as 10.0; noise of 3 cm on a flat patch was read as a roughness of 0.028 m; a step of 5 cm read as a slope of 14.0 degrees and a roughness of 0.0144 m by the plane, inside both limits, and as a rise of 26.6 degrees between neighbours, outside the slope limit, which made 2 cells a row lethal; and a slope of 24 degrees was 198 on tarmac, where the grip holds it, and 254 on gravel, where it does not.

#ifndef RC_RC_PERCEIVE_TRAVERSABILITY_HPP
#define RC_RC_PERCEIVE_TRAVERSABILITY_HPP

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <rc/field/terrain.hpp>
#include <rc/perceive/costmap.hpp>
#include <rc/vehicle/drive_train.hpp>
#include <vector>

namespace rc {
namespace perceive {

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
  const double res = g.spec.resolution;
  double sum = 0.0, sx = 0.0, sy = 0.0;
  for (int dj = -1; dj <= 1; ++dj)
    for (int di = -1; di <= 1; ++di) {
      sum += h[dj + 1][di + 1];
      sx += di * h[dj + 1][di + 1];
      sy += dj * h[dj + 1][di + 1];
    }
  const double a = sx / (6.0 * res);  // dz/dx
  const double b = sy / (6.0 * res);  // dz/dy
  const double c = sum / 9.0;
  double ss = 0.0;
  for (int dj = -1; dj <= 1; ++dj)
    for (int di = -1; di <= 1; ++di) {
      const double e = h[dj + 1][di + 1] - (c + a * di * res + b * dj * res);
      ss += e * e;
    }
  r.known = true;
  r.slope = std::atan(std::hypot(a, b));
  r.roughness = std::sqrt(ss / 6.0);
  for (int dj = -1; dj <= 1; ++dj)
    for (int di = -1; di <= 1; ++di) {
      if (di == 0 && dj == 0) continue;
      const double run = res * std::hypot(static_cast<double>(di), static_cast<double>(dj));
      r.max_gradient = std::max(r.max_gradient, std::atan(std::fabs(h[dj + 1][di + 1] - h[1][1]) / run));
    }
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


}  // namespace perceive
}  // namespace rc

#endif
