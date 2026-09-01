#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

#include "solution.hpp"

namespace {

using rc::math::Quat;
using rc::math::Transform;
using rc::math::Vec3;

constexpr double kPi = 3.14159265358979323846;
const Vec3 kZ{0.0, 0.0, 1.0};
const Vec3 kY{0.0, 1.0, 0.0};

rc::span<const double> view(const std::vector<double>& v) {
  return rc::span<const double>(v.data(), v.size());
}

// The planar arm every textbook starts with, because it has a closed form:
// two links in a plane, both turning about z.
Chain planar_arm(double l1, double l2) {
  Chain arm;
  arm.add(revolute_along_x(0.0, kZ));   // shoulder, at the origin
  arm.add(revolute_along_x(l1, kZ));    // elbow, one link out
  arm.add(revolute_along_x(l2, kZ));    // the tool, which does not turn
  return arm;
}

}  // namespace

RC_TEST("an arm with no joints puts the tool at the base") {
  const Chain empty;
  const std::vector<double> none;
  const Transform base_tool = empty.tool(view(none));

  RC_CHECK_NEAR(base_tool.translation.x, 0.0, 1e-12);
  RC_CHECK_NEAR(base_tool.translation.y, 0.0, 1e-12);
  RC_CHECK_NEAR(base_tool.rotation.w, 1.0, 1e-12);
}

RC_TEST("an arm folded straight out reaches the sum of its links") {
  const Chain arm = planar_arm(0.5, 0.3);
  const std::vector<double> home{0.0, 0.0, 0.0};
  const Transform base_tool = arm.tool(view(home));

  RC_CHECK_NEAR(base_tool.translation.x, 0.8, 1e-12);
  RC_CHECK_NEAR(base_tool.translation.y, 0.0, 1e-12);
}

RC_TEST("turning the first joint a quarter turn swings the whole arm") {
  const Chain arm = planar_arm(0.5, 0.3);
  const std::vector<double> q{kPi / 2.0, 0.0, 0.0};
  const Transform base_tool = arm.tool(view(q));

  // Straight up the y axis, because everything past the shoulder came with it.
  RC_CHECK_NEAR(base_tool.translation.x, 0.0, 1e-12);
  RC_CHECK_NEAR(base_tool.translation.y, 0.8, 1e-12);
}

RC_TEST("turning the second joint moves only what is beyond it") {
  // The check that catches a chain composed the wrong way round. With the
  // elbow bent and the shoulder still, the tool must be at the elbow plus the
  // second link turned, which is not the same as turning the whole arm.
  const Chain arm = planar_arm(0.5, 0.3);
  const std::vector<double> q{0.0, kPi / 2.0, 0.0};
  const Transform base_tool = arm.tool(view(q));

  RC_CHECK_NEAR(base_tool.translation.x, 0.5, 1e-12);
  RC_CHECK_NEAR(base_tool.translation.y, 0.3, 1e-12);
}

RC_TEST("the chain agrees with the closed form, everywhere") {
  // The test worth more than all the others. A planar two link arm has an
  // answer that can be written down without any of this machinery, so the
  // machinery can be checked against something derived independently rather
  // than against itself.
  const double l1 = 0.5;
  const double l2 = 0.3;
  const Chain arm = planar_arm(l1, l2);

  double worst = 0.0;
  int checked = 0;
  for (int i = -30; i <= 30; ++i) {
    for (int j = -30; j <= 30; ++j) {
      const double q1 = static_cast<double>(i) * 0.1;
      const double q2 = static_cast<double>(j) * 0.1;

      const std::vector<double> q{q1, q2, 0.0};
      const Vec3 chain = arm.tool(view(q)).translation;

      const double x = l1 * std::cos(q1) + l2 * std::cos(q1 + q2);
      const double y = l1 * std::sin(q1) + l2 * std::sin(q1 + q2);

      const double error = std::hypot(chain.x - x, chain.y - y);
      if (error > worst) worst = error;
      ++checked;
    }
  }

  std::cout << "\n  compared against the closed form at " << checked
            << " configurations, worst disagreement " << std::scientific
            << std::setprecision(2) << worst << " metres\n";
  RC_CHECK(worst < 1e-12);
}

RC_TEST("a frame partway down the arm is where that joint is") {
  const Chain arm = planar_arm(0.5, 0.3);
  const std::vector<double> q{kPi / 2.0, 0.0, 0.0};

  // The elbow is one link out along the shoulder's direction, which is now y.
  const Transform base_elbow = arm.frame_at(1, view(q));
  RC_CHECK_NEAR(base_elbow.translation.x, 0.0, 1e-12);
  RC_CHECK_NEAR(base_elbow.translation.y, 0.5, 1e-12);
}

RC_TEST("too few angles is reported rather than read past the end") {
  // The check that catches a chain indexing whatever follows the array. Holding
  // the missing joints at zero is a defensible answer; reading past the end is
  // not an answer at all.
  const Chain arm = planar_arm(0.5, 0.3);
  const std::vector<double> too_few{kPi / 2.0};

  RC_CHECK(!arm.accepts(view(too_few)));

  const Transform base_tool = arm.tool(view(too_few));
  RC_CHECK(std::isfinite(base_tool.translation.x));
  RC_CHECK_NEAR(base_tool.translation.x, 0.0, 1e-12);
  RC_CHECK_NEAR(base_tool.translation.y, 0.8, 1e-12);
}

RC_TEST("the right number of angles is accepted") {
  const Chain arm = planar_arm(0.5, 0.3);
  const std::vector<double> right{0.0, 0.0, 0.0};
  RC_CHECK(arm.accepts(view(right)));
}

RC_TEST("a joint turning about a different axis lifts the arm out of the plane") {
  // Nothing about the chain is planar. Give one joint a different axis and the
  // tool leaves the plane, which is the whole reason this is built from
  // transforms rather than from two dimensional trigonometry.
  Chain arm;
  arm.add(revolute_along_x(0.0, kZ));
  arm.add(revolute_along_x(0.5, kY));   // this one pitches
  arm.add(revolute_along_x(0.3, kZ));

  const std::vector<double> q{0.0, -kPi / 2.0, 0.0};
  const Transform base_tool = arm.tool(view(q));

  RC_CHECK_NEAR(base_tool.translation.x, 0.5, 1e-12);
  RC_CHECK_NEAR(base_tool.translation.z, 0.3, 1e-12);
}

RC_TEST("a six axis arm composes without the rotation drifting") {
  // Six joints is five compositions of quaternions, and lesson 06-02 measured
  // what repeated composition does to a rotation matrix. This reports what it
  // does here rather than assuming it is fine.
  Chain arm;
  const Vec3 axes[] = {kZ, kY, kY, kZ, kY, kZ};
  for (int i = 0; i < 6; ++i) arm.add(revolute_along_x(i == 0 ? 0.0 : 0.2, axes[i]));

  std::vector<double> q{0.3, -0.6, 0.9, -1.2, 0.5, 2.0};
  const Transform base_tool = arm.tool(view(q));

  const double norm = rc::math::norm(base_tool.rotation);
  std::cout << "  a six joint chain leaves the rotation with norm "
            << std::fixed << std::setprecision(15) << norm << "\n";

  RC_CHECK_NEAR(norm, 1.0, 1e-12);
  RC_CHECK(std::isfinite(base_tool.translation.x));
}

RC_TEST("turning a joint and then turning it back leaves the tool where it was") {
  const Chain arm = planar_arm(0.5, 0.3);
  const std::vector<double> home{0.0, 0.0, 0.0};
  const std::vector<double> away{1.1, -0.4, 0.0};

  const Vec3 first = arm.tool(view(home)).translation;
  arm.tool(view(away));
  const Vec3 again = arm.tool(view(home)).translation;

  // The chain holds no state between calls, which is what makes it safe to ask
  // about a configuration the arm is not currently in.
  RC_CHECK_NEAR(first.x, again.x, 1e-15);
  RC_CHECK_NEAR(first.y, again.y, 1e-15);
}
