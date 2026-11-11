#include <rc/test/rc_test.hpp>
#include <rc/nav/plan.hpp>

#include <iomanip>
#include <iostream>
#include <vector>

#include "solution.hpp"

namespace {

// A building 40 by 30 cells. A wall runs down x = 20 with a door 3 cells wide at y = 12 to 14 and a gap at the far end, y = 27 to 29.
OccupancyGrid building() {
  OccupancyGrid g(40, 30, 0.1, 0.0, 0.0);
  for (int y = 0; y < 30; ++y)
    for (int x = 0; x < 40; ++x) {
      const bool wall = x == 20 && !(y >= 12 && y <= 14) && !(y >= 27);
      for (int i = 0; i < 4; ++i) g.observe(x, y, wall);
    }
  return g;
}

WorldChange door_closes(int at_step) {
  WorldChange c;
  c.at_step = at_step;
  for (int y = 12; y <= 14; ++y) c.cells.push_back(GridPoint{20, y});
  return c;
}

const GridPoint kStart{3, 13};
const GridPoint kGoal{36, 13};

// The fewest moves the world allows, found by the library planner on the true building, four neighbours.
int shortest(const OccupancyGrid& world) {
  rc::nav::PlanOptions o;
  o.allow_diagonal = false;
  o.unknown = rc::nav::UnknownIs::blocked;
  const rc::nav::Plan p = rc::nav::plan_path(world, kStart, kGoal, o);
  return p.found ? static_cast<int>(p.cells.size()) - 1 : -1;
}

void print_row(const char* label, const RoverResult& r) {
  std::cout << "  " << std::left << std::setw(40) << label << std::right << std::setw(8) << (r.reached ? "reached" : "not") << std::setw(7)
            << r.steps << std::setw(7) << r.plans << std::setw(7) << r.bumps << "\n";
}

}  // namespace

RC_TEST("in a building that does not change, the rover reaches the goal in the fewest moves the building allows") {
  const OccupancyGrid world = building();
  const RoverResult r = drive_to_goal(world, kStart, kGoal, RoverPolicy(), 6, WorldChange(), 300);
  const int best = shortest(world);
  std::cout << "  building 40 by 30 with a door at the middle: the rover reached the goal in " << r.steps << " steps with " << r.plans << " plans and "
            << r.bumps << " bumps; the fewest moves the building allows is " << best << "\n\n";
  RC_CHECK(r.reached);
  RC_CHECK(r.steps == best);
  RC_CHECK(r.bumps == 0);
  RC_CHECK(r.plans >= 1);
  RC_CHECK(r.plans < r.steps);
  RC_CHECK(r.moves == r.steps);
}

RC_TEST("planning every step costs a plan for every move and does not make the route any shorter") {
  const OccupancyGrid world = building();
  RoverPolicy every;
  every.replan_every_step = true;
  const RoverResult a = drive_to_goal(world, kStart, kGoal, RoverPolicy(), 6, WorldChange(), 300);
  const RoverResult b = drive_to_goal(world, kStart, kGoal, every, 6, WorldChange(), 300);
  std::cout << "  replan when blocked: " << a.steps << " steps, " << a.plans << " plans; replan every step: " << b.steps << " steps, " << b.plans
            << " plans\n\n";
  RC_CHECK(a.reached);
  RC_CHECK(b.reached);
  RC_CHECK(b.plans >= b.steps);
  RC_CHECK(b.plans > 3 * a.plans);
  RC_CHECK(b.steps <= a.steps);
}

RC_TEST("a map that starts unknown cannot be planned through if unknown is a wall, and can if it is merely dear") {
  const OccupancyGrid world = building();
  RoverPolicy cautious;
  cautious.unknown_passable = false;
  const RoverResult stuck = drive_to_goal(world, kStart, kGoal, cautious, 6, WorldChange(), 300);
  const RoverResult goes = drive_to_goal(world, kStart, kGoal, RoverPolicy(), 6, WorldChange(), 300);
  std::cout << "  unknown cells forbidden: " << (stuck.reached ? "reached" : "did not get there") << (stuck.no_route ? ", no route" : "") << " after "
            << stuck.steps << " steps; unknown cells allowed at a cost: reached after " << goes.steps << "\n\n";
  RC_CHECK(goes.reached);
  RC_CHECK(!stuck.reached);
  RC_CHECK(stuck.no_route || stuck.steps >= 300);
}

RC_TEST("when a door closes in front of it, a rover that never replans keeps bumping, and one that does goes round") {
  const OccupancyGrid world = building();
  const WorldChange change = door_closes(15);

  RoverPolicy never;
  never.replan_when_blocked = false;
  const RoverResult a = drive_to_goal(world, kStart, kGoal, never, 6, change, 300);
  const RoverResult b = drive_to_goal(world, kStart, kGoal, RoverPolicy(), 6, change, 300);

  OccupancyGrid closed = world;
  for (const GridPoint& c : change.cells)
    for (int i = 0; i < 8; ++i) closed.observe(c.x, c.y, true);
  const int around = shortest(closed);

  std::cout << "  the door closes at step 15, the rover is nearly there\n"
            << "  policy                                    outcome  steps  plans  bumps\n";
  print_row("never replans", a);
  print_row("replans when blocked", b);
  std::cout << "  the fewest moves from the start with the door shut: " << around << "\n\n";
  RC_CHECK(!a.reached);
  RC_CHECK(a.bumps > 100);
  RC_CHECK(b.reached);
  RC_CHECK(b.bumps <= 3);
  RC_CHECK(b.steps > around - 20);
}

RC_TEST("believing a contradiction at once saves the bumps that a slowly forgetting map would cost") {
  const OccupancyGrid world = building();
  const WorldChange change = door_closes(15);
  RoverPolicy slow;
  slow.trust_contradictions = false;
  const RoverResult trusting = drive_to_goal(world, kStart, kGoal, RoverPolicy(), 6, change, 300);
  const RoverResult averaging = drive_to_goal(world, kStart, kGoal, slow, 6, change, 300);
  std::cout << "  the door closes at step 15\n  policy                                    outcome  steps  plans  bumps\n";
  print_row("believes a contradiction at once", trusting);
  print_row("lets the map average it away", averaging);
  std::cout << "\n";
  RC_CHECK(trusting.reached);
  RC_CHECK(averaging.reached);
  RC_CHECK(averaging.bumps > trusting.bumps);
  RC_CHECK(averaging.steps > trusting.steps);
  RC_CHECK(averaging.bumps >= 3);
}

RC_TEST("when every way through is shut, the rover stops with no route and does not loop") {
  OccupancyGrid walled = building();
  // Close every way through before the rover starts: the door and the gap.
  WorldChange sealed;
  sealed.at_step = 0;
  for (int y = 0; y < 30; ++y) sealed.cells.push_back(GridPoint{20, y});
  const RoverResult r = drive_to_goal(walled, kStart, kGoal, RoverPolicy(), 6, sealed, 400);
  std::cout << "  every way through shut: " << (r.reached ? "reached" : "not reached") << ", " << (r.no_route ? "stopped with no route" : "ran out of steps")
            << " after " << r.steps << " steps and " << r.plans << " plans\n\n";
  RC_CHECK(!r.reached);
  RC_CHECK(r.steps < 400);
  RC_CHECK(r.no_route || r.bumps > 0);
  RC_CHECK(r.plans > 0);
  RC_CHECK(r.steps >= 0);
}
