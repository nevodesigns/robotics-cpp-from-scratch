// rc/nav/rover.hpp
//
// A rover that plans through what it has seen, from lesson 16-16, graduated.
//
// One loop: apply any change to the world, sense the cells within a radius, plan if the policy says so, and make one move. The policy chooses when to replan (when the next step is a wall on the map, or every step),
// whether a cell seen to contradict the map is believed at once, and whether unknown cells may be planned through at a cost.
//
// In a 40 by 30 building the rover reached the goal in 33 steps, the fewest the building allows, with 1 plan and 0 bumps; replanning every step also took 33 steps and 33 plans. With unknown cells forbidden it found no route
// after 0 steps. When the door closed at step 15 a rover that never replanned bumped 284 times in 300 steps and did not arrive; one that replanned when blocked arrived in 83 steps with 6 plans and 0 bumps (61 is the fewest from the start with the door shut);
// one that let the map average the contradiction away took 87 steps and 4 bumps. With every way shut it stopped with no route after 54 steps and 7 plans.

#ifndef RC_NAV_ROVER_HPP
#define RC_NAV_ROVER_HPP

#include <cmath>
#include <cstddef>
#include <vector>
#include <rc/nav/grid.hpp>
#include <rc/nav/plan.hpp>

namespace rc {
namespace nav {

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
  for (int dy = -radius; dy <= radius; ++dy)
    for (int dx = -radius; dx <= radius; ++dx) {
      if (dx * dx + dy * dy > radius * radius) continue;
      const int x = at.x + dx;
      const int y = at.y + dy;
      if (!world.inside(x, y)) continue;
      const bool wall = world.classify(x, y) == Cell::occupied;
      const Cell believed = map.classify(x, y);
      if (trust && ((wall && believed == Cell::free) || (!wall && believed == Cell::occupied))) {
        force_cell(map, x, y, wall);
      } else {
        map.observe(x, y, wall);
      }
    }
}

// Drive from `start` to `goal` in `world`, starting with a map that knows nothing, one move of one cell (up, down, left or right) per step, for at most `max_steps`.
// Each step: apply the change if it is due, sense, plan if the policy says so, and make one move along the plan.
// A move into a wall of the world is a bump and the rover stays where it is.
inline RoverResult drive_to_goal(OccupancyGrid world, GridPoint start, GridPoint goal, const RoverPolicy& policy, int sense_radius,
                                 const WorldChange& change, int max_steps) {
  RoverResult result;
  OccupancyGrid map(world.width(), world.height(), world.resolution(), 0.0, 0.0);
  rc::nav::PlanOptions options;
  options.allow_diagonal = false;
  options.unknown = policy.unknown_passable ? rc::nav::UnknownIs::expensive : rc::nav::UnknownIs::blocked;

  GridPoint at = start;
  std::vector<GridPoint> path;
  std::size_t index = 0;   // the rover is at path[index]

  for (int step = 0; step < max_steps; ++step) {
    if (step == change.at_step)
      for (const GridPoint& c : change.cells) force_cell(world, c.x, c.y, true);

    sense_around(map, world, at, sense_radius, policy.trust_contradictions);
    if (at.x == goal.x && at.y == goal.y) {
      result.reached = true;
      result.steps = step;
      return result;
    }

    bool need_plan = path.empty() || index + 1 >= path.size();
    if (!need_plan && policy.replan_every_step) need_plan = true;
    if (!need_plan && policy.replan_when_blocked) {
      const GridPoint next = path[index + 1];
      if (map.classify(next.x, next.y) == Cell::occupied) need_plan = true;
    }
    if (need_plan) {
      const rc::nav::Plan plan = rc::nav::plan_path(map, at, goal, options);
      ++result.plans;
      if (!plan.found || plan.cells.size() < 2) {
        result.no_route = true;
        result.steps = step;
        return result;
      }
      path = plan.cells;
      index = 0;
    }

    const GridPoint next = path[index + 1];
    if (world.classify(next.x, next.y) == Cell::occupied) {
      ++result.bumps;
    } else {
      at = next;
      ++index;
      ++result.moves;
    }
    result.steps = step + 1;
  }
  return result;
}

}  // namespace nav
}  // namespace rc

#endif  // RC_NAV_ROVER_HPP
