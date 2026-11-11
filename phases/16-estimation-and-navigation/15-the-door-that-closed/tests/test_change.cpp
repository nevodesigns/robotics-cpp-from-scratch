#include <rc/test/rc_test.hpp>

#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <vector>

#include "solution.hpp"

namespace {

struct Rng {
  std::uint32_t state;
  std::uint32_t next() {
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
  }
  double uniform() { return static_cast<double>(next() >> 8) / 16777216.0; }
  double gaussian() {
    double sum = 0.0;
    for (int i = 0; i < 12; ++i) sum += uniform();
    return sum - 6.0;
  }
};

const double kNoReturn = 6.0;
const double kPi = 3.14159265358979323846;

void wall(OccupancyGrid& g, int x, int y) {
  for (int i = 0; i < 4; ++i) g.observe(x, y, true);   // enough to outweigh the two observations of free that every cell has had
}

void floor_cell(OccupancyGrid& g, int x, int y) {
  g.observe(x, y, false);
  g.observe(x, y, false);
}

// Two rooms, 4 m by 3 m each side by side, 5 cm cells. The wall between them is at x = 2 m (cell 40), with a doorway of 60 cm
// (cells 24 to 35). `door_open` says whether the doorway is a gap or filled in.
OccupancyGrid rooms(bool door_open) {
  OccupancyGrid g(160, 60, 0.05, 0.0, 0.0);
  for (int y = 0; y < 60; ++y)
    for (int x = 0; x < 160; ++x) floor_cell(g, x, y);
  for (int x = 0; x < 160; ++x) {
    wall(g, x, 0);
    wall(g, x, 59);
  }
  for (int y = 0; y < 60; ++y) {
    wall(g, 0, y);
    wall(g, 159, y);
    const bool in_door = y >= 24 && y <= 35;
    if (!(in_door && door_open)) wall(g, 40, y);
  }
  return g;
}

// What a range finder would read from `pose` in `world`: 91 beams over 270 degrees, found by marching along each ray.
// This is the world's side and does not use the code under test.
std::vector<Beam> scan_from(const OccupancyGrid& world, const Candidate& pose, double noise, Rng& rng) {
  std::vector<Beam> scan;
  for (int i = 0; i < 91; ++i) {
    Beam b;
    b.angle = (-135.0 + 270.0 * i / 90.0) * kPi / 180.0;
    for (double r = 0.02; r < kNoReturn; r += 0.01) {
      const double x = pose.x + r * std::cos(pose.theta + b.angle);
      const double y = pose.y + r * std::sin(pose.theta + b.angle);
      const rc::nav::CellIndex c = world.cell_for(x, y);
      if (!c.inside) break;
      if (world.classify(c.x, c.y) == Cell::occupied) {
        b.range = r + (noise > 0.0 ? noise * rng.gaussian() : 0.0);
        break;
      }
    }
    scan.push_back(b);
  }
  return scan;
}

const Candidate kRobot{1.0, 1.5, 0.0};     // facing the doorway, 1 m from it

double fraction(const Surprise& s) {
  const int judged = s.agree + s.appeared + s.vanished;
  return judged > 0 ? static_cast<double>(s.appeared + s.vanished) / judged : 0.0;
}

}  // namespace

RC_TEST("a scan of the map as it is agrees with it everywhere, and counts nothing as new or gone") {
  const OccupancyGrid map = rooms(true);
  Rng rng{1};
  const std::vector<Beam> scan = scan_from(map, kRobot, 0.0, rng);
  const Surprise s = compare_scan(map, scan, kNoReturn, kRobot);
  std::cout << "  scan of the map as it is: " << s.agree << " beams agree, " << s.appeared << " appeared, " << s.vanished << " vanished\n\n";
  RC_CHECK(s.agree > 60);
  RC_CHECK(s.appeared == 0);
  RC_CHECK(s.vanished == 0);
  RC_CHECK(compare_scan(map, std::vector<Beam>(), kNoReturn, kRobot).agree == 0);
}

RC_TEST("a door that has closed is a wall that appeared, and a door that has opened is a wall that vanished") {
  const OccupancyGrid map_open = rooms(true);       // what the robot believes: the doorway is a gap
  const OccupancyGrid truth_closed = rooms(false);
  Rng rng{1};
  const std::vector<Beam> closed_scan = scan_from(truth_closed, kRobot, 0.0, rng);
  const Surprise closed = compare_scan(map_open, closed_scan, kNoReturn, kRobot);

  const OccupancyGrid map_closed = rooms(false);    // what it believes: shut
  const OccupancyGrid truth_open = rooms(true);
  const std::vector<Beam> open_scan = scan_from(truth_open, kRobot, 0.0, rng);
  const Surprise opened = compare_scan(map_closed, open_scan, kNoReturn, kRobot);

  std::cout << "  the robot believes the door is open and it has closed: " << closed.agree << " agree, " << closed.appeared << " appeared, "
            << closed.vanished << " vanished\n  it believes the door is closed and it has opened:   " << opened.agree << " agree, "
            << opened.appeared << " appeared, " << opened.vanished << " vanished\n\n";
  RC_CHECK(closed.appeared >= 3);
  RC_CHECK(closed.vanished == 0);
  RC_CHECK(opened.vanished >= 3);
  RC_CHECK(opened.appeared == 0);
}

RC_TEST("noise alone makes some beams disagree with the map, and where the line is set decides how often that is called a change") {
  const OccupancyGrid map = rooms(true);
  std::cout << "  250 scans of the room as it is, ranges noisy by 3 cm, and 250 after the door closed\n"
            << "  threshold   share of unchanged scans called different   share of closed-door scans called different\n";
  double low_false = 0.0;
  double high_false = 1.0;
  double low_hit = 0.0;
  double high_hit = 1.0;
  Rng rng{5};
  const OccupancyGrid truth_closed = rooms(false);
  std::vector<double> still;
  std::vector<double> after;
  for (int i = 0; i < 250; ++i) {
    still.push_back(fraction(compare_scan(map, scan_from(map, kRobot, 0.03, rng), kNoReturn, kRobot)));
    after.push_back(fraction(compare_scan(map, scan_from(truth_closed, kRobot, 0.03, rng), kNoReturn, kRobot)));
  }
  for (double threshold : {0.01, 0.02, 0.04, 0.08, 0.15}) {
    int f = 0;
    int h = 0;
    for (int i = 0; i < 250; ++i) {
      if (still[static_cast<std::size_t>(i)] > threshold) ++f;
      if (after[static_cast<std::size_t>(i)] > threshold) ++h;
    }
    std::cout << "  " << std::setw(9) << std::fixed << std::setprecision(2) << threshold << "   " << std::setw(39) << std::setprecision(3) << f / 250.0
              << "   " << std::setw(40) << h / 250.0 << "\n";
    if (threshold == 0.01) {
      low_false = f / 250.0;
      low_hit = h / 250.0;
    }
    if (threshold == 0.08) {
      high_false = f / 250.0;
      high_hit = h / 250.0;
    }
  }
  std::cout << "\n";
  RC_CHECK(low_hit > 0.9);
  RC_CHECK(low_false > high_false);
  RC_CHECK(high_false < 0.05);
  RC_CHECK(high_hit > 0.7);
}

RC_TEST("a detector wants the disagreement to last, so one odd scan is ignored and a lasting one is not") {
  ChangeDetector detector(0.04, 3);
  Surprise fine;
  fine.agree = 80;
  Surprise odd;
  odd.agree = 70;
  odd.appeared = 10;
  Surprise nothing;
  RC_CHECK(!detector.update(fine));
  RC_CHECK(!detector.update(odd));                  // one odd scan
  RC_CHECK(detector.streak() == 1);
  RC_CHECK(!detector.update(fine));                  // and it is gone
  RC_CHECK(detector.streak() == 0);

  RC_CHECK(!detector.update(odd));
  RC_CHECK(!detector.update(nothing));               // a scan that judged nothing does not end the run
  RC_CHECK(detector.streak() == 1);
  RC_CHECK(!detector.update(odd));
  RC_CHECK(detector.update(odd));                    // the third odd scan in a row
  RC_CHECK(detector.changed());
  detector.reset();
  RC_CHECK(!detector.changed());
  RC_CHECK(detector.streak() == 0);
}

RC_TEST("compared with the map as it was, a closed door is noticed in three scans; compared with the map being repaired, never") {
  const OccupancyGrid truth_closed = rooms(false);
  const OccupancyGrid reference = rooms(true);          // the map as it was when the door was open, kept as it is
  OccupancyGrid working = rooms(true);                   // the map the robot keeps taking in what it sees
  Rng rng{8};
  ChangeDetector against_reference(0.04, 3);
  ChangeDetector against_working(0.04, 3);
  int noticed_reference = -1;
  int noticed_working = -1;
  int working_gave_way = -1;
  const rc::nav::CellIndex doorway = working.cell_for(2.0, 1.5);    // the middle of the doorway, where the wall now stands
  for (int scan = 1; scan <= 40; ++scan) {
    const std::vector<Beam> beams = scan_from(truth_closed, kRobot, 0.02, rng);
    if (against_reference.update(compare_scan(reference, beams, kNoReturn, kRobot)) && noticed_reference < 0) noticed_reference = scan;
    if (against_working.update(compare_scan(working, beams, kNoReturn, kRobot)) && noticed_working < 0) noticed_working = scan;
    for (const Beam& b : beams) {
      if (b.range <= 0.0 || b.range >= kNoReturn) continue;
      working.integrate_ray(kRobot.x, kRobot.y, kRobot.x + b.range * std::cos(kRobot.theta + b.angle),
                            kRobot.y + b.range * std::sin(kRobot.theta + b.angle), true);
    }
    if (working_gave_way < 0 && working.classify(doorway.x, doorway.y) == Cell::occupied) working_gave_way = scan;
  }
  std::cout << "  the door closes before scan 1, three scans in a row are wanted\n"
            << "  compared with the map as it was:         noticed at scan " << noticed_reference << "\n"
            << "  compared with the map being repaired:    noticed at scan " << noticed_working << " (-1 is never)\n"
            << "  the repaired map's doorway cell became a wall at scan " << working_gave_way << "\n\n";
  RC_CHECK(noticed_reference == 3);
  RC_CHECK(noticed_working == -1);
  RC_CHECK(working_gave_way > noticed_reference);
}
