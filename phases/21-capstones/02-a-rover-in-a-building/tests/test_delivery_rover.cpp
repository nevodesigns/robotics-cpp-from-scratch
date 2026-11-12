#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iomanip>
#include <iostream>

#include "solution.hpp"

namespace {

// Two rooms of 5 by 6 metres side by side, with a door of 1.5 m in the wall between them, and a start and a goal in the corners.
Building two_rooms() {
  std::vector<std::string> rows(24, std::string(40, '.'));
  for (int r = 0; r < 24; ++r) {
    rows[static_cast<std::size_t>(r)][0] = '#';
    rows[static_cast<std::size_t>(r)][39] = '#';
  }
  rows[0] = std::string(40, '#');
  rows[23] = std::string(40, '#');
  for (int y = 0; y < 24; ++y)
    if (y < 9 || y > 14) rows[static_cast<std::size_t>(23 - y)][20] = '#';
  return parse_building(rows);
}

const rc::nav::GridPoint kStart = {3, 3};
const rc::nav::GridPoint kGoal = {36, 20};

DeliveryConfig base() {
  DeliveryConfig c;
  c.inflate = 0.3;
  return c;
}

DeliveryConfig blind() {
  DeliveryConfig c = base();
  c.use_limiter = false;
  c.use_supervisor = false;
  return c;
}

// Somebody steps out from the side into the path, 1 to 2 m ahead of the rover, and stands there until 12 s.
Person steps_out() {
  Person p;
  p.x = 4.2;
  p.y = 4.2;
  p.x_end = 4.2;
  p.y_end = 2.2;
  p.radius = 0.25;
  p.from = 3.0;
  p.arrive = 4.2;
  p.to = 12.0;
  return p;
}

// Somebody walks into the doorway and stands there until 20 s.
Person in_the_doorway() {
  Person p;
  p.x = 7.2;
  p.y = 3.0;
  p.x_end = 5.1;
  p.y_end = 3.0;
  p.radius = 0.3;
  p.from = 2.0;
  p.arrive = 6.0;
  p.to = 20.0;
  return p;
}

DeliveryResult drive(const DeliveryConfig& c, const std::vector<Person>& people = std::vector<Person>(), double limit = 120.0) {
  return run_delivery(two_rooms(), people, kStart, kGoal, c, limit);
}

void line(const char* what, const DeliveryResult& r) {
  std::cout << "  " << what << std::string(42 - std::string(what).size(), ' ') << (r.reached ? "reached " : "NOT reached ") << std::fixed << std::setprecision(1) << std::setw(5) << r.time << " s, " << std::setw(4) << r.distance << " m, bumps " << std::setw(3) << r.bumps
            << ", closest " << std::setprecision(2) << r.min_gap << " m\n";
}

}  // namespace

RC_TEST("a building is read from its picture, a person walks in a straight line and is gone when their time is up, and the sensor reports the way ahead as the smallest of three") {
  const Building b = parse_building({"#####", "#..##", "#...#", "#####"});
  RC_CHECK(b.width == 5 && b.height == 4);
  RC_CHECK(b.is_wall(0, 0) && b.is_wall(4, 3) && b.is_wall(4, 2));
  RC_CHECK(!b.is_wall(1, 2) && !b.is_wall(3, 1));                    // the first row is the top: y = 3
  RC_CHECK(b.is_wall(3, 2));
  RC_CHECK(b.is_wall(-1, 0) && b.is_wall(0, 99));
  RC_CHECK(b.wall_at(0.1, 0.1) && !b.wall_at(0.3, 0.3 + 0.25));
  const Building ragged = parse_building({"###", "#"});              // a short row is wall where it is missing
  RC_CHECK(ragged.width == 3 && ragged.is_wall(1, 0) && ragged.is_wall(2, 0));
  Person p;
  p.x = 0.0;
  p.y = 0.0;
  p.x_end = 4.0;
  p.y_end = 2.0;
  p.from = 1.0;
  p.arrive = 5.0;
  p.to = 9.0;
  double x = -1.0;
  double y = -1.0;
  RC_CHECK(!person_at(p, 0.5, x, y));
  RC_CHECK(person_at(p, 1.0, x, y) && x == 0.0 && y == 0.0);
  RC_CHECK(person_at(p, 3.0, x, y) && std::fabs(x - 2.0) < 1e-12 && std::fabs(y - 1.0) < 1e-12);
  RC_CHECK(person_at(p, 7.0, x, y) && x == 4.0 && y == 2.0);
  RC_CHECK(!person_at(p, 9.0, x, y));
  double ranges[kBeams];
  for (int i = 0; i < kBeams; ++i) ranges[i] = 2.0;
  ranges[4] = 0.9;
  ranges[0] = 0.1;                                                    // a beam far to the side does not count for the way ahead
  RC_CHECK(front_reading(ranges).range == 0.9);
  ranges[5] = std::nan("");
  RC_CHECK(std::isnan(front_reading(ranges).range));
  for (int i = 0; i < kBeams; ++i) ranges[i] = INFINITY;
  RC_CHECK(std::isinf(front_reading(ranges).range));
  RC_CHECK(front_reading(ranges).age == 0.0);
  std::cout << "  the picture, the walking person and the front reading all gave what they should\n\n";
}

RC_TEST("a plan of the building gives a map in which walls and floor are both decided, and the sensor sees a wall where the plan has one") {
  const Building b = two_rooms();
  const rc::nav::OccupancyGrid m = prior_map(b);
  RC_CHECK(m.width() == 40 && m.height() == 24);
  RC_CHECK(m.classify(0, 0) == rc::nav::Cell::occupied);
  RC_CHECK(m.classify(20, 3) == rc::nav::Cell::occupied);
  RC_CHECK(m.classify(20, 11) == rc::nav::Cell::free);               // the door
  RC_CHECK(m.classify(10, 10) == rc::nav::Cell::free);
  rc::sim::Pose p;
  p.x = 4.0;
  p.y = 3.0;
  p.theta = 0.0;
  const double r0 = cast_beam(b, std::vector<Person>(), 0.0, p, 0.0, kMaxRange);
  RC_CHECK(std::isinf(r0));                                           // straight ahead through the door and across the other room: more than 3 m of free floor
  p.y = 1.0;
  const double wall = cast_beam(b, std::vector<Person>(), 0.0, p, 0.0, kMaxRange);
  std::cout << "  looking straight ahead from (4.0, 3.0) through the door: " << (std::isinf(r0) ? "nothing within 3 m" : "something") << "; from (4.0, 1.0) at the wall: " << std::fixed << std::setprecision(2) << wall << " m\n\n";
  RC_CHECK_NEAR(wall, 1.0, 0.06);
}

RC_TEST("through the door and across the second room, under the limiter and the supervisor, with nobody in the way") {
  const DeliveryResult r = drive(base());
  std::cout << "  the building with nobody in it:\n";
  line("limiter and supervisor on", r);
  std::cout << "  replans " << r.replans << ", top speed " << std::setprecision(2) << r.max_speed << " m/s\n\n";
  RC_CHECK(r.reached);
  RC_CHECK(r.time > 12.0 && r.time < 12.6);
  RC_CHECK(r.distance > 9.4 && r.distance < 9.8);
  RC_CHECK(r.bumps == 0 && r.stopped_ticks == 0 && r.waiting_ticks == 0);
  RC_CHECK(r.min_gap > 0.1);
  RC_CHECK(r.max_speed <= 0.8 + 1e-9);
}

RC_TEST("somebody steps out into the path, and a rover with the limiter stops short of them where one without it pushes against them for seven seconds") {
  const std::vector<Person> people = {steps_out()};
  const DeliveryResult all = drive(base(), people);
  const DeliveryResult none = drive(blind(), people);
  DeliveryConfig sup_only = base();
  sup_only.use_limiter = false;
  DeliveryConfig lim_only = base();
  lim_only.use_supervisor = false;
  const DeliveryResult s = drive(sup_only, people);
  const DeliveryResult l = drive(lim_only, people);
  std::cout << "  somebody steps into the path at 4.2 s and leaves at 12 s:\n";
  line("limiter and supervisor", all);
  line("limiter only", l);
  line("supervisor only (nothing tells it)", s);
  line("neither", none);
  std::cout << "  the one with neither was in contact for " << none.bumps << " ticks of 0.05 s: " << std::setprecision(1) << none.bumps * 0.05 << " s\n\n";
  RC_CHECK(all.reached && all.bumps == 0 && all.min_gap > 0.1);
  RC_CHECK(l.reached && l.bumps == 0);
  RC_CHECK(none.bumps > 100);
  RC_CHECK(none.min_gap < 0.05);
  RC_CHECK(s.bumps == none.bumps);                    // the supervisor decides from what it is told, and without the limiter it is told nothing about a person
}

RC_TEST("somebody in the doorway that the rover cannot see from where it waits is a ghost on the map for ever, unless it forgets") {
  const std::vector<Person> people = {in_the_doorway()};
  DeliveryConfig never = base();
  never.forget_after = -1.0;
  const DeliveryResult stuck = drive(never, people);
  const DeliveryResult forgets = drive(base(), people);
  std::cout << "  somebody in the doorway from 6 s to 20 s:\n";
  line("never forgets what it saw", stuck);
  line("forgets after 4 s with no way through", forgets);
  std::cout << "  waiting: " << stuck.waiting_ticks << " ticks against " << forgets.waiting_ticks << "\n\n";
  RC_CHECK(!stuck.reached);
  RC_CHECK(stuck.waiting_ticks > 2000);
  RC_CHECK(forgets.reached);
  RC_CHECK(forgets.time > 25.0 && forgets.time < 30.0);
  RC_CHECK(forgets.bumps == 0);
}

RC_TEST("a lost link stops the rover and keeps it stopped when the link returns, and a rover with no supervisor never notices") {
  DeliveryConfig link = base();
  link.link_lost_at = 5.0;
  link.link_back_at = 9.0;
  DeliveryConfig deaf = link;
  deaf.use_supervisor = false;
  const DeliveryResult stopped = drive(link);
  const DeliveryResult unheeding = drive(deaf);
  std::cout << "  the base station's heartbeat lost from 5 s to 9 s:\n";
  line("supervisor on", stopped);
  line("no supervisor", unheeding);
  std::cout << "  stopped for " << stopped.stopped_ticks << " ticks, cause " << rc::safety::causes_text(stopped.causes) << "; at the end it was " << (stopped.moving_at_end ? "moving" : "still") << "\n\n";
  RC_CHECK(!stopped.reached);
  RC_CHECK(stopped.causes == rc::safety::cause_link);
  RC_CHECK(stopped.stopped_ticks > 2000);
  RC_CHECK(!stopped.moving_at_end);
  RC_CHECK(unheeding.reached);
}

RC_TEST("a sensor that goes dead is a stop with the supervisor, a stop without a fault with only the limiter, and a rover driving blind with neither") {
  const std::vector<Person> people = {steps_out()};
  DeliveryConfig all = base();
  all.sensor_dead_at = 3.0;
  DeliveryConfig lim = all;
  lim.use_supervisor = false;
  DeliveryConfig none = blind();
  none.sensor_dead_at = 3.0;
  const DeliveryResult a = drive(all, people);
  const DeliveryResult l = drive(lim, people);
  const DeliveryResult n = drive(none, people);
  std::cout << "  the range sensor dead from 3 s, and somebody stepping out at 4.2 s:\n";
  line("limiter and supervisor", a);
  line("limiter only", l);
  line("neither", n);
  std::cout << "  with both, a stop with cause " << rc::safety::causes_text(a.causes) << " after " << std::setprecision(1) << a.distance << " m\n\n";
  RC_CHECK(!a.reached && a.bumps == 0 && a.causes == rc::safety::cause_sensors);
  RC_CHECK(a.distance < 3.0);
  RC_CHECK(!l.reached && l.bumps == 0 && l.stopped_ticks == 0 && !l.moving_at_end);   // stopped by a speed limit of zero, which is not a fault anybody is told about
  RC_CHECK(n.bumps > 100);
}
