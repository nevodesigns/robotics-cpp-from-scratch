// rc/drive/follow.hpp
//
// Following a path, from lesson 26-04.
//
// cross_track gives the signed distance of a point from a path (positive on the left). nearest_index finds the closest point of the path, searching only forward from where it last was, so a path that passes near itself does not make the follower jump back. pure_pursuit_steer takes the first point at least a lookahead distance ahead and steers along the arc of the bicycle model that reaches it: atan2(2 L sin(alpha), lookahead), where alpha is the angle to that point from the heading. stanley_steer steers by the heading error to the path plus atan2(k times the cross-track error of the front axle, speed), with the sign that turns the car back towards the path. Both are clamped to the largest steering angle. follow_path drives the bicycle model with one of them and reports the worst and final cross-track error, how often the car crossed the path, and whether it got to the end.
//
// Starting 1 m to the side of a straight path, pure pursuit with a lookahead of 1.0 m ended 0.000 m off it and Stanley 0.000 m. Where a straight turned into a circle of radius 3 m with no easing in, a lookahead of 0.6 m strayed from the path by at most 0.015 m and one of 2.0 m by 0.149 m, Stanley 0.036 m. A very short lookahead crossed the path 9 times and a longer one 2. On the unsmoothed corner of 90 degrees pure pursuit cut inside by 0.38 m and on the smoothed one by 0.41 m.

#ifndef RC_RC_DRIVE_FOLLOW_HPP
#define RC_RC_DRIVE_FOLLOW_HPP

#include <cmath>
#include <cstddef>
#include <rc/drive/hybrid_astar.hpp>
#include <rc/drive/smooth.hpp>
#include <vector>

namespace rc {
namespace drive {

enum FollowMethod { kPurePursuit = 0, kStanley = 1 };

struct FollowConfig {
  double wheelbase = 0.33;
  double max_steer = 0.4189;
  double lookahead = 1.0;   // metres, pure pursuit
  double k = 1.0;           // gain, Stanley
  double speed = 1.0;       // metres per second
  double dt = 0.02;
  int max_steps = 6000;
  FollowMethod method = kPurePursuit;
};

struct FollowRun {
  std::vector<CarPose> trace;
  double max_cte = 0.0;     // metres, the worst distance from the path
  double final_cte = 0.0;
  int crossings = 0;        // how often the car crossed from one side of the path to the other
  bool reached = false;
};

inline double clamp_steer(double s, double max_steer) {
  if (!(s == s)) return 0.0;
  return s > max_steer ? max_steer : (s < -max_steer ? -max_steer : s);
}

// The signed distance of a point from the path, positive on its left, found against the nearest segment.
inline double cross_track(const std::vector<CarPose>& path, double x, double y) {
  double best = 1e18, signed_d = 0.0;
  for (std::size_t i = 1; i < path.size(); ++i) {
    const double ax = path[i - 1].x, ay = path[i - 1].y;
    const double bx = path[i].x - ax, by = path[i].y - ay;
    const double len2 = bx * bx + by * by;
    double u = len2 > 0.0 ? ((x - ax) * bx + (y - ay) * by) / len2 : 0.0;
    u = u < 0.0 ? 0.0 : (u > 1.0 ? 1.0 : u);
    const double px = ax + u * bx - x, py = ay + u * by - y;
    const double d = std::hypot(px, py);
    if (d < best) {
      best = d;
      const double side = bx * (y - ay) - by * (x - ax);
      signed_d = side >= 0.0 ? d : -d;
    }
  }
  return signed_d;
}

// The nearest point of the path to (x, y), looking only at `from` and later.
inline std::size_t nearest_index(const std::vector<CarPose>& path, double x, double y, std::size_t from) {
  std::size_t best = from;
  double best_d = 1e18;
  for (std::size_t i = from; i < path.size(); ++i) {
    const double d = std::hypot(path[i].x - x, path[i].y - y);
    if (d < best_d) {
      best_d = d;
      best = i;
    }
    if (d > best_d + 2.0) break;  // well past the nearest, and the path only goes on from here
  }
  return best;
}

// Pure pursuit: steer along the arc to the first point at least `lookahead` away, from index `idx` on.
inline double pure_pursuit_steer(const CarPose& pose, const std::vector<CarPose>& path, std::size_t idx, const FollowConfig& cfg) {
  std::size_t j = idx;
  while (j + 1 < path.size() && std::hypot(path[j].x - pose.x, path[j].y - pose.y) < cfg.lookahead) ++j;
  double steer = 0.0;
  const double alpha = wrap_pi(std::atan2(path[j].y - pose.y, path[j].x - pose.x) - pose.theta);
  const double dist = std::hypot(path[j].x - pose.x, path[j].y - pose.y);
  if (dist > 1e-9) steer = std::atan2(2.0 * cfg.wheelbase * std::sin(alpha), dist);
  return clamp_steer(steer, cfg.max_steer);
}

// Stanley: the heading error to the path plus an angle from the cross-track error of the front axle.
inline double stanley_steer(const CarPose& pose, const std::vector<CarPose>& path, std::size_t idx, const FollowConfig& cfg) {
  const double fx = pose.x + cfg.wheelbase * std::cos(pose.theta);
  const double fy = pose.y + cfg.wheelbase * std::sin(pose.theta);
  const std::size_t i = idx + 1 < path.size() ? idx : path.size() - 2;
  const double path_heading = std::atan2(path[i + 1].y - path[i].y, path[i + 1].x - path[i].x);
  const double e = cross_track(path, fx, fy);
  double steer = 0.0;
  steer = wrap_pi(path_heading - pose.theta) + std::atan2(-cfg.k * e, cfg.speed + 1e-3);
  return clamp_steer(steer, cfg.max_steer);
}

// Drives the bicycle model from `start` along the path with one of the two followers.
inline FollowRun follow_path(const std::vector<CarPose>& path, const CarPose& start, const FollowConfig& cfg) {
  FollowRun run;
  if (path.size() < 2) return run;
  CarPose pose = start;
  std::size_t idx = 0;
  double last_side = 0.0;
  for (int step = 0; step < cfg.max_steps; ++step) {
    const double rx = cfg.method == kStanley ? pose.x + cfg.wheelbase * std::cos(pose.theta) : pose.x;
    const double ry = cfg.method == kStanley ? pose.y + cfg.wheelbase * std::sin(pose.theta) : pose.y;
    idx = nearest_index(path, rx, ry, idx);
    const double cte = cross_track(path, pose.x, pose.y);
    if (std::fabs(cte) > run.max_cte) run.max_cte = std::fabs(cte);
    if (last_side != 0.0 && cte != 0.0 && (cte > 0.0) != (last_side > 0.0) && std::fabs(cte) > 1e-3) ++run.crossings;
    if (std::fabs(cte) > 1e-3) last_side = cte;
    run.final_cte = std::fabs(cte);
    run.trace.push_back(pose);
    if (idx + 1 >= path.size() && std::hypot(path.back().x - pose.x, path.back().y - pose.y) < 0.15) {
      run.reached = true;
      break;
    }
    const double steer = cfg.method == kStanley ? stanley_steer(pose, path, idx, cfg) : pure_pursuit_steer(pose, path, idx, cfg);
    pose = drive_arc(pose, steer, cfg.speed * cfg.dt, cfg.wheelbase);
  }
  return run;
}


}  // namespace drive
}  // namespace rc

#endif
