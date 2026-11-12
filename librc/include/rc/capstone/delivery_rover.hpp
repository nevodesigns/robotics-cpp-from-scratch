// rc/capstone/delivery_rover.hpp
//
// A rover that drives through a building under the safety layer of phase 19, from lesson 21-02, graduated.
//
// A Building is read from a picture (parse_building); a Person walks in a straight line and is gone when their time is up (person_at); the rover has a nine beam range sensor (cast_beam, front_reading) and a map that starts as the plan of the building (prior_map) and is updated by what it sees (map_scan). run_delivery plans through the map inflated by a margin, follows the path by pure pursuit, and every tick gives the supervisor the heartbeat, the fence, the state of the sensor and the speed the range limiter allows. A rover that has had no way through for a while forgets what it saw.
//
// Through two rooms and a door in 12.3 s over 9.6 m with 0 bumps. Somebody stepping into the path: with the limiter 0 bumps and closest 0.16 m; with neither limiter nor supervisor 144 ticks in contact (7.2 s). A lost link stopped it for good (cause link); a dead sensor stopped it after 2.4 m (cause sensors); somebody in the doorway left it waiting 2296 ticks if it never forgot and 301 if it forgot after 4 s.

#ifndef RC_CAPSTONE_DELIVERY_ROVER_HPP
#define RC_CAPSTONE_DELIVERY_ROVER_HPP

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <rc/control/limits.hpp>
#include <rc/math/vector.hpp>
#include <rc/nav/clearance.hpp>
#include <rc/nav/grid.hpp>
#include <rc/nav/plan.hpp>
#include <rc/nav/pursuit.hpp>
#include <rc/safety/blackbox.hpp>
#include <rc/safety/geofence.hpp>
#include <rc/safety/limiter.hpp>
#include <rc/safety/supervisor.hpp>
#include <rc/sim/diff_drive.hpp>
#include <string>
#include <vector>

namespace rc {
namespace capstone {

// A building as a picture: '#' is a wall and anything else is floor, one character a cell, 0.25 m a cell, the first row at the top (the largest y). Cell (0, 0) is the bottom left.
struct Building {
  int width = 0;
  int height = 0;
  std::vector<char> wall;
  double resolution = 0.25;

  bool is_wall(int x, int y) const { return x < 0 || y < 0 || x >= width || y >= height || wall[static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x)] != 0; }
  bool wall_at(double mx, double my) const { return is_wall(static_cast<int>(std::floor(mx / resolution)), static_cast<int>(std::floor(my / resolution))); }
};

inline Building parse_building(const std::vector<std::string>& rows) {
  Building b;
  b.height = static_cast<int>(rows.size());
  for (const std::string& r : rows) b.width = std::max(b.width, static_cast<int>(r.size()));
  b.wall.assign(static_cast<std::size_t>(b.width * b.height), 0);
  for (int row = 0; row < b.height; ++row)
    for (int x = 0; x < b.width; ++x) {
      const std::string& r = rows[static_cast<std::size_t>(row)];
      const bool w = x >= static_cast<int>(r.size()) || r[static_cast<std::size_t>(x)] == '#';
      b.wall[static_cast<std::size_t>(b.height - 1 - row) * static_cast<std::size_t>(b.width) + static_cast<std::size_t>(x)] = w ? 1 : 0;
    }
  return b;
}

// Somebody who walks in: a disc that appears at (x, y) at `from` seconds, walks in a straight line to (x_end, y_end), arriving at `arrive` seconds, stands there, and is gone at `to` seconds.
struct Person {
  double x = 0.0;
  double y = 0.0;
  double x_end = 0.0;
  double y_end = 0.0;
  double radius = 0.3;
  double from = 0.0;
  double arrive = 0.0;
  double to = 0.0;
};

inline bool person_at(const Person& p, double t, double& px, double& py) {
  if (t < p.from || t >= p.to) return false;
  const double f = t >= p.arrive || p.arrive <= p.from ? 1.0 : (t - p.from) / (p.arrive - p.from);
  px = p.x + (p.x_end - p.x) * f;
  py = p.y + (p.y_end - p.y) * f;
  return true;
}

// The range sensor: nine beams, from 60 degrees right of straight ahead to 60 left in steps of 15, each reading up to 3 m, found by stepping along the beam every 5 cm until it is inside a wall or a person. A beam that reaches 3 m without hitting anything reads infinity (nothing returned).
// A dead sensor reads NaN on every beam.
const int kBeams = 9;
const double kMaxRange = 3.0;

inline double beam_angle(int i) { return (i - 4) * 15.0 * 3.14159265358979323846 / 180.0; }

inline bool blocked_at(const Building& b, const std::vector<Person>& people, double t, double x, double y) {
  if (b.wall_at(x, y)) return true;
  for (const Person& p : people) {
    double px = 0.0;
    double py = 0.0;
    if (person_at(p, t, px, py) && std::hypot(x - px, y - py) <= p.radius) return true;
  }
  return false;
}

inline double cast_beam(const Building& b, const std::vector<Person>& people, double t, const rc::sim::Pose& pose, double angle, double max_range) {
  const double dx = std::cos(pose.theta + angle);
  const double dy = std::sin(pose.theta + angle);
  for (double r = 0.05; r <= max_range; r += 0.05)
    if (blocked_at(b, people, t, pose.x + dx * r, pose.y + dy * r)) return r;
  return INFINITY;
}

// What the limiter of 19-05 is given for the way ahead: the smallest range of the three beams nearest straight ahead (30 degrees either side), as a Reading with age 0. A NaN on any of them makes the reading NaN, and nothing returned on all three makes it infinity.
inline rc::safety::Reading front_reading(const double ranges[kBeams]) {
  rc::safety::Reading r;
  r.age = 0.0;
  double best = INFINITY;
  for (int i = 3; i <= 5; ++i) {
    if (std::isnan(ranges[i])) {
      r.range = std::nan("");
      return r;
    }
    best = std::min(best, ranges[i]);
  }
  r.range = best;
  return r;
}

// Put one scan on the map: for each beam, a ray from the robot to where it hit (and a hit on the last cell) or to its full length (no hit). Beams that are NaN are ignored.
inline void map_scan(rc::nav::OccupancyGrid& map, const rc::sim::Pose& pose, const double ranges[kBeams]) {
  for (int i = 0; i < kBeams; ++i) {
    if (std::isnan(ranges[i])) continue;
    const bool hit = std::isfinite(ranges[i]);
    const double r = hit ? ranges[i] : kMaxRange;
    const double a = pose.theta + beam_angle(i);
    map.integrate_ray(pose.x, pose.y, pose.x + std::cos(a) * r, pose.y + std::sin(a) * r, hit);
  }
}

// The map the robot starts with: what the building plan says. Floor is seen free twice and a wall seen occupied twice, so that both are decided (a cell needs two sightings to be believed).
inline rc::nav::OccupancyGrid prior_map(const Building& b) {
  rc::nav::OccupancyGrid m(b.width, b.height, b.resolution, 0.0, 0.0);
  for (int y = 0; y < b.height; ++y)
    for (int x = 0; x < b.width; ++x) {
      const bool w = b.is_wall(x, y);
      m.observe(x, y, w);
      m.observe(x, y, w);
    }
  return m;
}

// The centres of the cells of a path, in metres.
inline std::vector<rc::math::Vec2> cells_to_points(const std::vector<rc::nav::GridPoint>& cells, double resolution) {
  std::vector<rc::math::Vec2> pts;
  for (const rc::nav::GridPoint& c : cells) pts.push_back({(c.x + 0.5) * resolution, (c.y + 0.5) * resolution});
  return pts;
}

struct DeliveryConfig {
  double top_speed = 0.8;
  double robot_radius = 0.15;
  double inflate = 0.2;             // how far from a wall the planner keeps the path
  double lookahead = 0.5;
  bool use_limiter = true;          // the range limiter of lesson 19-05; without it the robot drives at top speed
  bool use_supervisor = true;       // the supervisor of 19-10; without it nothing can stop the robot
  double link_lost_at = -1.0;       // the base station's heartbeat stops at this time (negative: never)
  double link_back_at = -1.0;       // and returns at this time (negative: never)
  double sensor_dead_at = -1.0;     // the range sensor reads NaN from this time on
  double replan_every = 2.0;
  double forget_after = 4.0;        // after this many seconds with no way through, what was seen is forgotten and the map goes back to the plan of the building (negative: never)
};

struct DeliveryResult {
  bool reached = false;
  double time = 0.0;
  double distance = 0.0;
  int replans = 0;
  int bumps = 0;                    // ticks in which the robot was in contact with a wall or a person and so was not allowed to move
  double min_gap = 1e9;             // the closest the robot's edge came to a wall or person, in metres
  double max_speed = 0.0;
  int waiting_ticks = 0;            // ticks standing still because there was no way through
  int stopped_ticks = 0;            // ticks with the supervisor in stop
  unsigned causes = 0;
  bool moving_at_end = false;
};

// The distance from the edge of the robot to the nearest thing, by looking in 16 directions up to 1 m (and 1 m if nothing is nearer), stepping 2 cm.
inline double gap_to_obstacles(const Building& b, const std::vector<Person>& people, double t, const rc::sim::Pose& pose, double radius) {
  double best = 1.0;
  for (int k = 0; k < 16; ++k) {
    const double a = k * 3.14159265358979323846 / 8.0;
    for (double r = 0.0; r < best; r += 0.02) {
      if (blocked_at(b, people, t, pose.x + std::cos(a) * (radius + r), pose.y + std::sin(a) * (radius + r))) {
        best = r;
        break;
      }
    }
  }
  return best;
}

// Drive from `start` to `goal` (cells) through the building at 20 ticks a second, for at most `limit` seconds. Every tick, in this order:
//  - scan: nine beams from the robot's true pose, put on the map; the front reading from front_reading;
//  - plan (the planner of 16-06 on the map inflated by `inflate` (the inflation is redone every fourth tick, since it is the slow part), unknown cells blocked, from the cell the robot is in): at the start, when `replan_every` seconds have passed, or when a cell of the path ahead is a wall on the inflated map. No way through: the robot waits (speed 0) and tries again every half second;
//  - the supervisor (if used): the e-stop closed, the heartbeat fresh unless it is in the gap from link_lost_at to link_back_at, the fence the building's outline (inside, with a buffer of 0.1), sensors faulted after 3 ticks in which no beam gave a number and the front reading NaN, and range_cap the speed the limiter allows; its decision caps the speed;
//  - the speed asked for is top_speed, cut by the limiter (use_limiter), by the supervisor's cap, and by 0 when waiting; it changes by at most 1.5 m/s^2;
//  - steering: pure pursuit on the path with the lookahead, turned into wheel speeds by wheels_for; the move is refused if it would put the robot's centre within robot_radius of a wall or person (a bump).
// It has reached the goal when the centre is within 0.2 m of the centre of the goal cell.
inline DeliveryResult run_delivery(const Building& building, const std::vector<Person>& people, rc::nav::GridPoint start, rc::nav::GridPoint goal, const DeliveryConfig& cfg, double limit) {
  DeliveryResult r;
  const double dt = 0.05;
  const double wheel_base = 0.3;
  rc::nav::OccupancyGrid map = prior_map(building);
  rc::sim::Pose pose;
  pose.x = (start.x + 0.5) * building.resolution;
  pose.y = (start.y + 0.5) * building.resolution;
  const rc::math::Vec2 goal_pt{(goal.x + 0.5) * building.resolution, (goal.y + 0.5) * building.resolution};
  rc::nav::PlanOptions options;
  options.unknown = rc::nav::UnknownIs::blocked;
  rc::nav::PurePursuit pursuit({}, cfg.lookahead);
  std::vector<rc::nav::GridPoint> path_cells;
  rc::safety::LimiterConfig lc;
  lc.top_speed = cfg.top_speed;
  rc::safety::SupervisorConfig sc;
  sc.top_speed = cfg.top_speed;
  sc.slow_speed = 0.5 * cfg.top_speed;
  rc::safety::Supervisor sup(sc);
  const double wx = building.width * building.resolution;
  const double wy = building.height * building.resolution;
  const std::vector<rc::safety::Point> fence = {{0.1, 0.1}, {wx - 0.1, 0.1}, {wx - 0.1, wy - 0.1}, {0.1, wy - 0.1}};
  rc::safety::Inputs healthy;
  healthy.estop_closed = true;
  healthy.heartbeat_fresh = true;
  healthy.fence = rc::safety::FenceState::inside;
  healthy.tilt = rc::safety::TiltState::ok;
  healthy.sensors = rc::safety::CheckState::ok;
  healthy.power.state = rc::safety::PowerState::ok;
  healthy.power.factor = 1.0;
  healthy.range_cap = cfg.top_speed + 1.0;
  sup.reset(healthy);
  double v = 0.0;
  double next_plan = 0.0;
  int dead_ticks = 0;
  double no_path_since = -1.0;
  bool have_path = false;
  rc::nav::OccupancyGrid planning = rc::nav::inflated(map, cfg.inflate);
  const int steps = static_cast<int>(limit / dt);
  for (int i = 0; i < steps; ++i) {
    const double t = i * dt;
    const bool sensor_dead = cfg.sensor_dead_at >= 0.0 && t >= cfg.sensor_dead_at;
    double ranges[kBeams];
    for (int k = 0; k < kBeams; ++k) ranges[k] = sensor_dead ? std::nan("") : cast_beam(building, people, t, pose, beam_angle(k), kMaxRange);
    map_scan(map, pose, ranges);
    const rc::safety::Reading front = front_reading(ranges);
    // planning
    if (i % 4 == 0 || !have_path) planning = rc::nav::inflated(map, cfg.inflate);
    bool need = !have_path || t >= next_plan;
    if (have_path && !need) {
      for (const rc::nav::GridPoint& c : path_cells)
        if (planning.classify(c.x, c.y) == rc::nav::Cell::occupied) {
          need = true;
          break;
        }
    }
    bool waiting = false;
    if (need) {
      const rc::nav::CellIndex at = map.cell_for(pose.x, pose.y);
      const rc::nav::Plan plan = rc::nav::plan_path(planning, {at.x, at.y}, goal, options);
      ++r.replans;
      if (plan.found && plan.cells.size() >= 2) {
        path_cells = plan.cells;
        pursuit = rc::nav::PurePursuit(cells_to_points(path_cells, building.resolution), cfg.lookahead);
        have_path = true;
        next_plan = t + cfg.replan_every;
        no_path_since = -1.0;
      } else {
        have_path = false;
        next_plan = t + 0.5;
        if (no_path_since < 0.0) no_path_since = t;
        if (cfg.forget_after >= 0.0 && t - no_path_since >= cfg.forget_after) {
          map = prior_map(building);
          no_path_since = t;
        }
      }
    }
    if (!have_path) waiting = true;
    // the supervisor
    double cap = cfg.top_speed;
    bool stopped = false;
    if (cfg.use_supervisor) {
      const bool no_numbers = std::isnan(front.range);
      dead_ticks = no_numbers ? dead_ticks + 1 : 0;
      rc::safety::Inputs in = healthy;
      in.heartbeat_fresh = !(cfg.link_lost_at >= 0.0 && t >= cfg.link_lost_at && !(cfg.link_back_at >= 0.0 && t >= cfg.link_back_at));
      in.fence = rc::safety::fence_state(fence, {pose.x, pose.y}, 0.1, 0.1);
      in.sensors = dead_ticks >= 3 ? rc::safety::CheckState::faulted : (dead_ticks > 0 ? rc::safety::CheckState::suspect : rc::safety::CheckState::ok);
      in.range_cap = cfg.use_limiter ? std::fabs(rc::safety::limit_speed(cfg.top_speed, front, rc::safety::Reading{5.0, 0.0}, lc)) : cfg.top_speed + 1.0;
      const rc::safety::Decision d = sup.step(in);
      cap = d.speed_cap;
      if (d.mode == rc::safety::Mode::stopped) {
        stopped = true;
        r.causes |= d.stop_causes;
        ++r.stopped_ticks;
      }
    } else if (cfg.use_limiter) {
      cap = std::fabs(rc::safety::limit_speed(cfg.top_speed, front, rc::safety::Reading{5.0, 0.0}, lc));
    }
    double target = std::min(cfg.top_speed, cap);
    if (waiting || stopped) target = 0.0;
    if (waiting) ++r.waiting_ticks;
    v = rc::control::rate_limit(v, target, 1.5 * dt);
    double curvature = 0.0;
    if (have_path) curvature = pursuit.follow(pose).curvature;
    const rc::nav::WheelSpeeds w = rc::nav::wheels_for(v, curvature, wheel_base);
    const rc::sim::Pose next = rc::sim::step(pose, w.left, w.right, wheel_base, dt);
    if (blocked_at(building, people, t, next.x, next.y) || gap_to_obstacles(building, people, t, next, cfg.robot_radius) <= 0.0) {
      ++r.bumps;
      v = 0.0;
    } else {
      r.distance += std::hypot(next.x - pose.x, next.y - pose.y);
      pose = next;
    }
    r.max_speed = std::max(r.max_speed, v);
    r.min_gap = std::min(r.min_gap, gap_to_obstacles(building, people, t, pose, cfg.robot_radius));
    r.time = t + dt;
    if (std::hypot(pose.x - goal_pt.x, pose.y - goal_pt.y) <= 0.2) {
      r.reached = true;
      break;
    }
  }
  r.moving_at_end = v > 0.01;
  return r;
}

}  // namespace capstone
}  // namespace rc

#endif  // RC_CAPSTONE_DELIVERY_ROVER_HPP
