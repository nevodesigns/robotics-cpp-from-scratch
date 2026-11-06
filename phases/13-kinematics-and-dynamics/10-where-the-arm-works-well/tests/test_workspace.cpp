#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iomanip>
#include <iostream>

#include "solution.hpp"

namespace {

const double kL1 = 0.5;
const double kL2 = 0.4;
const double kPi = 3.14159265358979323846;
const double kCell = 0.005;

rc::kin::ArmLimits free_joints() {
  rc::kin::ArmLimits l;
  l.shoulder = rc::kin::Limit{-10.0, 10.0};
  l.elbow = rc::kin::Limit{-10.0, 10.0};
  return l;
}

// The shoulder can only turn over the top half, as on an arm fixed to a wall.
rc::kin::ArmLimits wall_mounted() {
  rc::kin::ArmLimits l = free_joints();
  l.shoulder = rc::kin::Limit{0.0, kPi};
  return l;
}

// The elbow can bend one way only, as most real ones do.
rc::kin::ArmLimits one_way_elbow() {
  rc::kin::ArmLimits l = free_joints();
  l.elbow = rc::kin::Limit{0.0, kPi};
  return l;
}

}  // namespace

RC_TEST("a point the arm cannot reach, or no pose can hold, has no manipulability") {
  RC_CHECK(best_manipulability(kL1, kL2, free_joints(), 2.0, 0.0) < 0.0);     // beyond the links
  RC_CHECK(best_manipulability(kL1, kL2, free_joints(), 0.02, 0.0) < 0.0);    // inside the hole
  RC_CHECK(best_manipulability(kL1, kL2, free_joints(), 0.6, 0.3) > 0.0);

  // On the x axis with the elbow square the determinant is l1 * l2 exactly.
  const double r = std::sqrt(kL1 * kL1 + kL2 * kL2);
  RC_CHECK_NEAR(best_manipulability(kL1, kL2, free_joints(), r, 0.0), kL1 * kL2, 1e-9);
}

RC_TEST("walking the grid gives the area of the ring, to a fraction of a percent") {
  const WorkspaceMap m = map_workspace(kL1, kL2, free_joints(), 0.0, kCell);
  const double ring = kPi * ((kL1 + kL2) * (kL1 + kL2) - (kL1 - kL2) * (kL1 - kL2));
  std::cout << "  free joints: reachable " << std::fixed << std::setprecision(4) << m.reachable
            << " m^2 against the ring's " << ring << " m^2 (" << m.cells << " cells)\n\n";
  RC_REQUIRE(m.cells > 0);
  RC_CHECK_NEAR(m.reachable, ring, 0.005 * ring);
}

RC_TEST("excluding near-singular poses removes a band at each edge of the ring") {
  // |sin q2| >= 0.5 means the elbow between 30 and 150 degrees, which is a ring of radii
  // sqrt(l1^2 + l2^2 + 2 l1 l2 cos q2) at the two ends.
  const double lo = std::sqrt(kL1 * kL1 + kL2 * kL2 + 2.0 * kL1 * kL2 * std::cos(150.0 * kPi / 180.0));
  const double hi = std::sqrt(kL1 * kL1 + kL2 * kL2 + 2.0 * kL1 * kL2 * std::cos(30.0 * kPi / 180.0));
  const double predicted = kPi * (hi * hi - lo * lo);

  const WorkspaceMap m = map_workspace(kL1, kL2, free_joints(), 0.5, kCell);
  std::cout << "  usable at half the best: radii " << std::fixed << std::setprecision(3) << lo << " to " << hi
            << " m, area " << std::setprecision(4) << m.dexterous << " m^2 against a predicted " << predicted
            << ", " << std::setprecision(1) << 100.0 * m.dexterous / m.reachable << " percent of the reachable\n\n";
  RC_CHECK_NEAR(m.dexterous, predicted, 0.01 * predicted);
  RC_CHECK(m.dexterous < m.reachable);
}

RC_TEST("the stricter the demand the smaller the usable area, and it never grows") {
  const double fractions[] = {0.0, 0.1, 0.3, 0.5, 0.7, 0.9};
  std::cout << "  needed fraction of the best   usable area   percent of reachable\n";
  double previous = 1e9;
  bool shrinks = true;
  double full = 0.0;
  for (double f : fractions) {
    const WorkspaceMap m = map_workspace(kL1, kL2, free_joints(), f, kCell);
    if (f == 0.0) full = m.reachable;
    std::cout << "  " << std::setw(27) << std::fixed << std::setprecision(1) << f << "   " << std::setw(11)
              << std::setprecision(4) << m.dexterous << "   " << std::setw(9) << std::setprecision(1)
              << 100.0 * m.dexterous / full << "\n";
    if (m.dexterous > previous) shrinks = false;
    previous = m.dexterous;
  }
  std::cout << "\n";
  RC_CHECK(shrinks);
  RC_CHECK(previous < 0.5 * full);
}

RC_TEST("a shoulder limited to the top half keeps well over half the area, because the arm folds over") {
  const WorkspaceMap all = map_workspace(kL1, kL2, free_joints(), 0.0, kCell);
  const WorkspaceMap wall = map_workspace(kL1, kL2, wall_mounted(), 0.0, kCell);
  std::cout << "  shoulder limited to the top half: reachable " << std::fixed << std::setprecision(4) << wall.reachable
            << " m^2 of " << all.reachable << ", " << std::setprecision(1) << 100.0 * wall.reachable / all.reachable
            << " percent\n\n";
  RC_REQUIRE(all.reachable > 0.0);
  RC_CHECK(wall.reachable < all.reachable);
  RC_CHECK(wall.reachable > 0.6 * all.reachable);
}

RC_TEST("an elbow that bends one way loses no points, because the other branch is always there") {
  const WorkspaceMap all = map_workspace(kL1, kL2, free_joints(), 0.0, kCell);
  const WorkspaceMap one = map_workspace(kL1, kL2, one_way_elbow(), 0.0, kCell);
  const WorkspaceMap one_good = map_workspace(kL1, kL2, one_way_elbow(), 0.5, kCell);
  const WorkspaceMap all_good = map_workspace(kL1, kL2, free_joints(), 0.5, kCell);
  std::cout << "  elbow limited to one direction: reachable " << std::fixed << std::setprecision(4) << one.reachable
            << " of " << all.reachable << " m^2, usable " << one_good.dexterous << " of " << all_good.dexterous
            << " m^2\n\n";
  RC_REQUIRE(all.reachable > 0.0);
  RC_CHECK_NEAR(one.reachable, all.reachable, 0.005 * all.reachable);
  RC_CHECK_NEAR(one_good.dexterous, all_good.dexterous, 0.005 * all_good.dexterous);
}
