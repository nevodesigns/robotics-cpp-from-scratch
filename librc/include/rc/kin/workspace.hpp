// rc/kin/workspace.hpp
//
// Where an arm can work, and where it can work well, from lesson 13-10, graduated.
//
// Reachable is a ring: the points between |l1 - l2| and l1 + l2 from the shoulder. Usable is a smaller
// ring, because near both edges the arm is close to a singularity and the tool can hardly move in some
// direction (lesson 13-04). On a 0.5 and 0.4 metre arm the reachable area measured 2.5129 m^2 against
// the ring's 2.5133, and demanding a manipulability of half the best left 2.1763 m^2, which is 86.6
// percent, inside radii of 0.252 and 0.870 m. Demanding 0.9 of the best left 43.6 percent.
//
// Joint limits cut it unevenly. A shoulder limited to the top half kept 70.0 percent of the area and not 50, because
// the arm folds back over its own base. An elbow that bends only one way lost none of either area: the
// two elbow poses are mirror images, so the other one always holds.

#ifndef RC_KIN_WORKSPACE_HPP
#define RC_KIN_WORKSPACE_HPP

#include <algorithm>
#include <cmath>
#include <cstddef>

#include <rc/kin/limits.hpp>
#include <rc/kin/planar_arm.hpp>

namespace rc {
namespace kin {

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
inline double best_manipulability(double l1, double l2, const ArmLimits& limits, double x,
                                  double y) {
  const auto legal = legal_solutions(x, y, l1, l2, limits);
  if (!legal) return -1.0;
  const LegalSolutions& both = legal.value();
  if (!both.any()) return -1.0;

  double best = 0.0;
  if (both.up_holds) best = std::max(best, std::fabs(l1 * l2 * std::sin(both.elbow_up.q2)));
  if (both.down_holds) best = std::max(best, std::fabs(l1 * l2 * std::sin(both.elbow_down.q2)));
  return best;
}

// Walk a grid over the square the arm could possibly reach, and add up the area of the cells
// that are reachable and the cells that are reachable well. A cell counts as dexterous when its
// best manipulability is at least `min_fraction` of the most the arm could ever have, l1 * l2.
inline WorkspaceMap map_workspace(double l1, double l2, const ArmLimits& limits,
                                  double min_fraction, double cell) {
  WorkspaceMap map;
  if (cell <= 0.0) return map;

  const double half = l1 + l2;
  const std::size_t n = static_cast<std::size_t>(std::ceil(2.0 * half / cell));
  const double area = cell * cell;
  const double needed = min_fraction * l1 * l2;

  for (std::size_t row = 0; row < n; ++row) {
    for (std::size_t col = 0; col < n; ++col) {
      const double x = -half + (static_cast<double>(col) + 0.5) * cell;
      const double y = -half + (static_cast<double>(row) + 0.5) * cell;
      ++map.cells;
      const double m = best_manipulability(l1, l2, limits, x, y);
      if (m < 0.0) continue;
      map.reachable += area;
      if (m >= needed) map.dexterous += area;
    }
  }
  return map;
}

}  // namespace kin
}  // namespace rc

#endif  // RC_KIN_WORKSPACE_HPP
