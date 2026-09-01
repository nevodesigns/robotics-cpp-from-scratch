#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <cstddef>
#include <vector>

#include <rc/core/compat.hpp>
#include <rc/kin/chain.hpp>
#include <rc/math/vector.hpp>

// How the tool moves when each joint moves a little: one column per joint.
//
// Measured rather than derived. Nudge a joint, see where the tool went, divide
// by the nudge. That works for any chain, including one whose geometry came out
// of a file this morning, and it is why a numerical solver does not care what
// shape the arm is.
inline std::vector<rc::math::Vec3> jacobian(const rc::kin::Chain& arm,
                                            rc::span<const double> angles, double step) {
  std::vector<rc::math::Vec3> columns;
  columns.reserve(arm.size());

  std::vector<double> nudged(angles.data(), angles.data() + angles.size());

  for (std::size_t joint = 0; joint < arm.size() && joint < nudged.size(); ++joint) {
    const double held = nudged[joint];

    // Central difference: one nudge each way. It costs a second evaluation and
    // its error falls as the square of the step rather than linearly, which is
    // worth far more than the call it saves.
    nudged[joint] = held + step;
    const rc::math::Vec3 forward =
        arm.tool(rc::span<const double>(nudged.data(), nudged.size())).translation;

    nudged[joint] = held - step;
    const rc::math::Vec3 backward =
        arm.tool(rc::span<const double>(nudged.data(), nudged.size())).translation;

    nudged[joint] = held;

    columns.push_back(rc::math::Vec3{(forward.x - backward.x) / (2.0 * step),
                                     (forward.y - backward.y) / (2.0 * step),
                                     (forward.z - backward.z) / (2.0 * step)});
  }
  return columns;
}

struct ReachOptions {
  // Measured in this lesson: the error of a central difference falls as the
  // square of the step until about 1e-6, and then rises again as subtracting
  // two nearly equal numbers loses the difference. A step of 1e-14 is worse
  // than a step of 1e-1.
  double step = 1e-6;

  double gain = 1.0;
  double tolerance = 1e-9;

  // A cap, because an unreachable target is not an error the solver can detect
  // and is a loop it will never leave.
  int max_iterations = 20000;
};

struct ReachResult {
  std::vector<double> angles;
  int iterations = 0;
  double error = 0.0;

  // Reported, never assumed. A solver that stops after its last iteration and
  // hands back the angles without saying it gave up has produced a pose that
  // does not reach the target and looks exactly like one that does.
  bool converged = false;
};

// Move the joints in the direction that reduces the distance to the target.
//
// The step is the transpose of the Jacobian applied to the error, which needs
// no matrix inverse at all: each joint moves by how much moving it would help,
// which is the dot product of its column with the error.
inline ReachResult reach(const rc::kin::Chain& arm, rc::span<const double> start,
                         const rc::math::Vec3& target, const ReachOptions& options) {
  ReachResult result;
  result.angles.assign(start.data(), start.data() + start.size());
  result.angles.resize(arm.size(), 0.0);

  for (int iteration = 0; iteration <= options.max_iterations; ++iteration) {
    const rc::math::Vec3 at =
        arm.tool(rc::span<const double>(result.angles.data(), result.angles.size())).translation;

    const rc::math::Vec3 error{target.x - at.x, target.y - at.y, target.z - at.z};
    result.error = rc::math::length(error);
    result.iterations = iteration;

    if (result.error < options.tolerance) {
      result.converged = true;
      return result;
    }

    const std::vector<rc::math::Vec3> columns =
        jacobian(arm, rc::span<const double>(result.angles.data(), result.angles.size()),
                 options.step);

    for (std::size_t joint = 0; joint < columns.size(); ++joint) {
      result.angles[joint] += options.gain * rc::math::dot(columns[joint], error);
    }
  }
  return result;
}

#endif  // LESSON_SOLUTION_HPP
