#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <algorithm>
#include <cmath>
#include <cstddef>

#include <rc/kin/limits.hpp>
#include <rc/kin/planar_arm.hpp>

// How much of the plane an arm can work in, and how much of that it can work in well.
struct WorkspaceMap {
  double reachable = 0.0;   // square metres where some legal pose puts the tool
  double dexterous = 0.0;   // square metres where the best legal pose is not near a singularity
  std::size_t cells = 0;    // how many grid cells were looked at
};

// How good the best legal pose at (x, y) is, as the determinant of the Jacobian, which for this
// arm is l1 * l2 * |sin q2|. Minus one if no legal pose reaches the point.
//
// The best of the two elbows and not the first one found: a point one elbow cannot hold is
// often held perfectly by the other.
inline double best_manipulability(double l1, double l2, const rc::kin::ArmLimits& limits, double x,
                                  double y) {
  const auto legal = rc::kin::legal_solutions(x, y, l1, l2, limits);
  if (!legal) return -1.0;
  const rc::kin::LegalSolutions& both = legal.value();
  if (!both.any()) return -1.0;

  // TODO: the best of the legal elbows. The determinant for one pose is l1 * l2 * |sin q2|.
  // Use elbow_up only if up_holds, and elbow_down only if down_holds, and keep the larger.
  (void)both;
  (void)limits;
  return -1.0;
}

// Walk a grid over the square the arm could possibly reach, and add up the area of the cells
// that are reachable and the cells that are reachable well. A cell counts as dexterous when its
// best manipulability is at least `min_fraction` of the most the arm could ever have, l1 * l2.
inline WorkspaceMap map_workspace(double l1, double l2, const rc::kin::ArmLimits& limits,
                                  double min_fraction, double cell) {
  WorkspaceMap map;
  if (cell <= 0.0) return map;

  const double half = l1 + l2;
  const std::size_t n = static_cast<std::size_t>(std::ceil(2.0 * half / cell));
  const double area = cell * cell;
  const double needed = min_fraction * l1 * l2;

  // TODO: visit the centre of every cell. For each one, count it in map.cells, ask
  // best_manipulability, skip it when that is negative, add `area` to map.reachable
  // otherwise, and add `area` to map.dexterous as well when the value is at least `needed`.
  //
  // The centre of the cell in column col and row row is
  //   x = -half + (col + 0.5) * cell,  y = -half + (row + 0.5) * cell.
  (void)limits;
  (void)n;
  (void)area;
  (void)needed;
  return map;
}

#endif  // LESSON_SOLUTION_HPP
