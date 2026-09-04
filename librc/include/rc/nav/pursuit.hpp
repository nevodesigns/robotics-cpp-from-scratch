// rc/nav/pursuit.hpp
//
// Following a path by aiming at a point on it, from lesson 16-04.
//
// The method is one line of geometry and the whole difficulty is in the
// lookahead distance, which looks like a tuning preference and is a gain.
//
// With a perfect estimate, shorter is simply better and there is no argument
// for a long lookahead at all: a right angle corner is cut by about 27 percent
// of the lookahead, measured, and nothing else changes. The reason to lengthen
// it is that the follower never sees the truth. It sees an estimate, and the
// lookahead decides how hard it acts on it.
//
// Measured with a pose 100 ms behind, the smallest lookahead that works is the
// first one longer than speed times that latency, at every speed:
//
//   speed    speed x 0.1 s    0.05    0.10    0.20    0.40
//   0.5              0.05     lost  0.0023  0.0028  0.0098
//   1.0              0.10     lost    lost  0.0104  0.0077
//   2.0              0.20     lost    lost    lost  0.0415
//   4.0              0.40     lost    lost    lost    lost
//
// "lost" means the robot left the path and did not come back. A follower with a
// fixed lookahead in it is a follower with a maximum speed nobody wrote down.

#ifndef RC_NAV_PURSUIT
#define RC_NAV_PURSUIT

#include <cmath>
#include <cstddef>
#include <utility>
#include <vector>

#include <rc/math/vector.hpp>
#include <rc/sim/diff_drive.hpp>

namespace rc {
namespace nav {

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
//
// The whole method is one line of geometry: an arc from where you are, through
// a point ahead of you, has curvature 2y/L squared, where y is how far to the
// side that point is and L is how far away it is. Drive that arc, do it again
// next tick, and the path is followed.
//
// What is not obvious is that the lookahead distance is a gain. Short means
// aiming close, which corrects hard, and the correction is computed from the
// pose you believe you are at. When that belief is wrong by as much as the
// lookahead, the target point is inside the error and the steering command is
// noise. Measured, the smallest lookahead that works is the first one larger
// than speed times the estimate's latency, and it is that at every speed.
class PurePursuit {
 public:
  PurePursuit(std::vector<rc::math::Vec2> path, double lookahead)
      : path_(std::move(path)), lookahead_(lookahead > 0.0 ? lookahead : 0.01) {}

  double lookahead() const { return lookahead_; }
  const std::vector<rc::math::Vec2>& path() const { return path_; }
  void reset() { from_ = 0; }

  Steering follow(const rc::sim::Pose& believed) {
    Steering steering;
    if (path_.empty()) return steering;

    const std::size_t index = target_index(believed);
    steering.have_target = true;
    steering.target = path_[index];

    // The target, seen from the robot: x forward, y to the left. A rotation by
    // minus the heading, which is the transpose of the rotation by it, and
    // lesson 06-02 is where that came from.
    const double dx = steering.target.x - believed.x;
    const double dy = steering.target.y - believed.y;
    const double forward = std::cos(believed.theta) * dx + std::sin(believed.theta) * dy;
    const double left = -std::sin(believed.theta) * dx + std::cos(believed.theta) * dy;

    const double distance_squared = forward * forward + left * left;
    steering.curvature = distance_squared > 0.0 ? 2.0 * left / distance_squared : 0.0;
    return steering;
  }

  // How far the robot actually is from the path, measured to the nearest point
  // on it rather than to the nearest waypoint. Those differ by as much as the
  // spacing between waypoints, which is exactly the amount that makes a
  // tracking figure look better than it is.
  double cross_track_error(const rc::sim::Pose& pose) const {
    double best = -1.0;
    for (std::size_t i = 0; i + 1 < path_.size(); ++i) {
      const rc::math::Vec2 a = path_[i], b = path_[i + 1];
      const double dx = b.x - a.x, dy = b.y - a.y;
      const double length_squared = dx * dx + dy * dy;

      // Where along the segment the foot of the perpendicular falls, clamped to
      // the segment so a point beyond either end measures to the end.
      double t = 0.0;
      if (length_squared > 0.0)
        t = ((pose.x - a.x) * dx + (pose.y - a.y) * dy) / length_squared;
      if (t < 0.0) t = 0.0;
      if (t > 1.0) t = 1.0;

      const double distance = std::hypot(pose.x - (a.x + t * dx), pose.y - (a.y + t * dy));
      if (best < 0.0 || distance < best) best = distance;
    }
    return best < 0.0 ? 0.0 : best;
  }

  bool arrived(const rc::sim::Pose& pose, double tolerance) const {
    if (path_.empty()) return true;
    return std::hypot(pose.x - path_.back().x, pose.y - path_.back().y) <= tolerance;
  }

 private:
  // The first point at least a lookahead away, searching forward from where the
  // last one was found and never before it.
  //
  // Searching the whole path instead is the natural first version and it fails
  // on any path that comes back near itself: from a metre along a corridor, the
  // start of the path is also more than a lookahead away, and it is earlier in
  // the array. Measured on a path that returns down a parallel leg, that robot
  // drove eighty metres in a straight line and never turned.
  std::size_t target_index(const rc::sim::Pose& believed) {
    for (std::size_t k = from_; k < path_.size(); ++k) {
      const double distance = std::hypot(path_[k].x - believed.x, path_[k].y - believed.y);
      if (distance >= lookahead_) {
        from_ = k > 0 ? k - 1 : 0;
        return k;
      }
    }
    // Nothing left is far enough, which means the end is within a lookahead.
    // Aim at it.
    return path_.size() - 1;
  }

  std::vector<rc::math::Vec2> path_;
  double lookahead_ = 1.0;
  std::size_t from_ = 0;
};

}  // namespace nav
}  // namespace rc

#endif  // RC_NAV_PURSUIT
