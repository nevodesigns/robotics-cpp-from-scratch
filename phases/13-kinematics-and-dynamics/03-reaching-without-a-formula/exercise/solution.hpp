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
  // TODO: one column per joint, measured rather than derived.
  //
  // Nudge a joint, see where the tool went, divide by the nudge. Use a central
  // difference, one nudge each way: it costs a second evaluation of the arm and
  // its error falls as the square of the step rather than linearly, which is
  // worth far more than the call it saves.
  //
  // Put the joint back before moving on to the next one.
  (void)arm;
  (void)angles;
  (void)step;
  return std::vector<rc::math::Vec3>();
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

  // TODO: move the joints in the direction that reduces the distance to the
  // target, until it is close enough or the iterations run out.
  //
  // The step is the transpose of the Jacobian applied to the error, which needs
  // no matrix inverse at all: each joint moves by how much moving it would
  // help, which is the dot product of its column with the error.
  //
  // Two things about stopping. The cap is not optional: an unreachable target
  // is not something the arithmetic notices, the error simply stops going down,
  // and without a limit the loop never leaves.
  //
  // And whether it converged is reported rather than assumed. A solver that
  // runs out of iterations and hands back the angles without saying so has
  // produced a pose that does not reach the target and looks exactly like one
  // that does.
  (void)target;
  (void)options;
  return result;
}

#endif  // LESSON_SOLUTION_HPP
