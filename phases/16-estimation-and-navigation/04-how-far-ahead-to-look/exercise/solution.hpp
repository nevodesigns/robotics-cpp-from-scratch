#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <cstddef>
#include <vector>

#include <rc/math/vector.hpp>
#include <rc/sim/diff_drive.hpp>

// What the follower decided.
struct Steering {
  bool have_target = false;
  rc::math::Vec2 target{};
  double curvature = 0.0;   // one over the radius of the arc to drive
};

struct WheelSpeeds {
  double left = 0.0;
  double right = 0.0;
};

// A curvature and a speed, turned into what the two motors are asked for.
inline WheelSpeeds wheels_for(double speed, double curvature, double wheel_base) {
  const double turn = curvature * speed;   // radians per second
  WheelSpeeds wheels;
  wheels.left = speed - turn * wheel_base / 2.0;
  wheels.right = speed + turn * wheel_base / 2.0;
  return wheels;
}

// Follows a path by aiming at a point on it, a fixed distance ahead.
class PurePursuit {
 public:
  PurePursuit(std::vector<rc::math::Vec2> path, double lookahead)
      : path_(std::move(path)), lookahead_(lookahead > 0.0 ? lookahead : 0.01) {}

  double lookahead() const { return lookahead_; }
  const std::vector<rc::math::Vec2>& path() const { return path_; }
  void reset() { from_ = 0; }

  // TODO 2: aim at the target and say what arc gets there.
  //
  // Take the target from target_index below, put it into the robot's own frame,
  // and compute the curvature.
  //
  // The robot's frame has x forward and y to its left, so rotating the offset
  // by minus the heading gives
  //
  //     forward =  cos(theta) * dx + sin(theta) * dy
  //     left    = -sin(theta) * dx + cos(theta) * dy
  //
  // and the arc from the robot through that point has
  //
  //     curvature = 2 * left / (forward^2 + left^2)
  //
  // which is the whole of pure pursuit. Guard the division: a target exactly
  // underneath the robot has no arc to it.
  //
  // Set have_target and target as well, because the tests draw them.
  Steering follow(const rc::sim::Pose& believed) {
    (void)believed;
    return Steering{};
  }

  // TODO 3: how far the robot is from the path.
  //
  // To the nearest point on the path, not the nearest waypoint. For each
  // segment, project the robot onto the line through it:
  //
  //     t = ((p - a) . (b - a)) / |b - a|^2
  //
  // clamp t to between 0 and 1 so a point past either end measures to that end,
  // and take the distance to a + t * (b - a). The answer is the smallest over
  // all the segments.
  //
  // Measuring to the nearest waypoint instead flatters the result by as much as
  // the spacing between them, which is how a tracking figure comes out better
  // than the driving was.
  //
  // Return 0.0 for a path with fewer than two points.
  double cross_track_error(const rc::sim::Pose& pose) const {
    (void)pose;
    return 0.0;
  }

  bool arrived(const rc::sim::Pose& pose, double tolerance) const {
    if (path_.empty()) return true;
    return std::hypot(pose.x - path_.back().x, pose.y - path_.back().y) <= tolerance;
  }

 private:
  // TODO 1: the first point on the path at least a lookahead away.
  //
  // Search forward from from_, which is where the last target was found, and
  // never before it. Update from_ as you go so the next call starts there.
  //
  // Searching the whole path from index zero is the natural first version and
  // it is wrong in a way that does not show on a straight test path. On any
  // path that comes back near itself, a point near the start is also more than
  // a lookahead away and it comes first in the array, so the robot aims at it.
  // The test in this lesson drives such a path both ways.
  //
  // If nothing left is far enough, the end of the path is within a lookahead,
  // so aim at the end.
  std::size_t target_index(const rc::sim::Pose& believed) {
    (void)believed;
    return 0;
  }

  std::vector<rc::math::Vec2> path_;
  double lookahead_ = 1.0;
  std::size_t from_ = 0;
};

#endif  // LESSON_SOLUTION_HPP
