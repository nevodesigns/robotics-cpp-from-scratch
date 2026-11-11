#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <cstddef>
#include <vector>

#include <rc/nav/grid.hpp>
#include <rc/nav/plan.hpp>

using rc::nav::Cell;
using rc::nav::GridPoint;
using rc::nav::OccupancyGrid;

// How the rover decides.
struct RoverPolicy {
  bool replan_when_blocked = true;     // plan again when the next step on the path is a wall on the map
  bool replan_every_step = false;      // plan again after every move
  bool trust_contradictions = true;    // a cell the map said was free, now seen to be a wall (or the reverse), is believed at once
  bool unknown_passable = true;        // plan through cells nobody has seen, at a higher cost
};

// A cell of the world that changes: from `at_step` on, these cells are walls.
struct WorldChange {
  int at_step = -1;
  std::vector<GridPoint> cells;
};

struct RoverResult {
  bool reached = false;
  int steps = 0;        // ticks of the loop, which is also the number of moves tried
  int plans = 0;        // how many times the planner was run
  int bumps = 0;        // moves into a cell that was a wall in the world
  int moves = 0;        // moves that happened
  bool no_route = false;   // the planner found nothing and the rover stopped
};

inline void force_cell(OccupancyGrid& grid, int x, int y, bool occupied) {
  for (int i = 0; i < 8; ++i) grid.observe(x, y, occupied);
}

// Look at every cell within `radius` cells of the rover: each is observed once as the world has it.
// With `trust` on, a cell whose observation contradicts the map's decision (the map says free and the world says wall, or the reverse)
// is forced to the world's answer straight away.
inline void sense_around(OccupancyGrid& map, const OccupancyGrid& world, GridPoint at, int radius, bool trust) {
  // TODO: for every cell (x, y) = (at.x + dx, at.y + dy) with dx * dx + dy * dy <= radius * radius that is inside the world:
  //   wall = world.classify(x, y) == Cell::occupied; believed = map.classify(x, y).
  //   If `trust` and the map decided the opposite (wall but believed free, or not a wall but believed occupied),
  //   force_cell(map, x, y, wall); otherwise map.observe(x, y, wall) once.
  (void)map;
  (void)world;
  (void)at;
  (void)radius;
  (void)trust;
}

// Drive from `start` to `goal` in `world`, starting with a map that knows nothing, one move of one cell (up, down, left or right) per step, for at most `max_steps`.
// Each step: apply the change if it is due, sense, plan if the policy says so, and make one move along the plan.
// A move into a wall of the world is a bump and the rover stays where it is.
inline RoverResult drive_to_goal(OccupancyGrid world, GridPoint start, GridPoint goal, const RoverPolicy& policy, int sense_radius,
                                 const WorldChange& change, int max_steps) {
  // TODO: the map starts as OccupancyGrid(world.width(), world.height(), world.resolution(), 0.0, 0.0), all unknown.
  // Planner options: allow_diagonal = false; unknown = UnknownIs::expensive if policy.unknown_passable, otherwise UnknownIs::blocked.
  // Keep the rover's cell `at`, the current `path` (a vector of GridPoint) and `index`, the rover being at path[index].
  // For step = 0 up to max_steps:
  //   1. If step == change.at_step, force_cell(world, c.x, c.y, true) for every cell of the change.
  //   2. sense_around(map, world, at, sense_radius, policy.trust_contradictions).
  //   3. If at is the goal: reached = true, steps = step, return.
  //   4. need_plan if the path is empty or the rover is at its end; or policy.replan_every_step;
  //      or policy.replan_when_blocked and the next cell on the path, path[index + 1], is Cell::occupied on the MAP.
  //   5. If need_plan: plan = rc::nav::plan_path(map, at, goal, options); plans += 1.
  //      If it found no path (or fewer than two cells): no_route = true, steps = step, return.
  //      Otherwise path = plan.cells, index = 0.
  //   6. next = path[index + 1]. If the WORLD has a wall there: bumps += 1. Otherwise at = next, index += 1, moves += 1.
  //   7. steps = step + 1.
  // Return the result.
  (void)world;
  (void)start;
  (void)goal;
  (void)policy;
  (void)sense_radius;
  (void)change;
  (void)max_steps;
  return RoverResult();
}

#endif  // LESSON_SOLUTION_HPP
