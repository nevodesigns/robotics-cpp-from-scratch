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
  g.observe(x, y, true);
  g.observe(x, y, true);
}

// A room 4 m by 3 m with 5 cm cells, walls all round, and a block 75 cm by 50 cm against the north wall if asked. The robot's coordinates are in metres from the corner.
OccupancyGrid room(bool pillar) {
  OccupancyGrid g(80, 60, 0.05, 0.0, 0.0);
  for (int x = 0; x < 80; ++x) {
    wall(g, x, 0);
    wall(g, x, 59);
  }
  for (int y = 0; y < 60; ++y) {
    wall(g, 0, y);
    wall(g, 79, y);
  }
  if (pillar)
    for (int y = 40; y < 50; ++y)
      for (int x = 20; x < 35; ++x) wall(g, x, y);
  return g;
}

// A corridor 30 m long and 1 m wide: walls on both long sides and closed ends too far to be seen from the middle.
OccupancyGrid corridor() {
  OccupancyGrid g(600, 20, 0.05, 0.0, 0.0);
  for (int x = 0; x < 600; ++x) {
    wall(g, x, 0);
    wall(g, x, 19);
  }
  for (int y = 0; y < 20; ++y) {
    wall(g, 0, y);
    wall(g, 599, y);
  }
  return g;
}

// What a range finder would read from `pose` in `map`: 61 beams across 270 degrees, each found by marching along the ray.
// This is the world's side and does not use the code under test.
std::vector<Beam> scan_from(const OccupancyGrid& map, const Candidate& pose, double noise, std::uint32_t seed) {
  Rng rng{seed};
  std::vector<Beam> scan;
  for (int i = 0; i < 61; ++i) {
    Beam b;
    b.angle = (-135.0 + 270.0 * i / 60.0) * kPi / 180.0;
    b.range = 0.0;
    for (double r = 0.02; r < kNoReturn; r += 0.01) {
      const double x = pose.x + r * std::cos(pose.theta + b.angle);
      const double y = pose.y + r * std::sin(pose.theta + b.angle);
      const rc::nav::CellIndex c = map.cell_for(x, y);
      if (!c.inside) break;
      if (map.classify(c.x, c.y) == Cell::occupied) {
        b.range = r + (noise > 0.0 ? noise * rng.gaussian() : 0.0);
        break;
      }
    }
    scan.push_back(b);
  }
  return scan;
}

}  // namespace

RC_TEST("a perfect scan fits the map exactly from where it was taken, and fits worse the further the pose is from there") {
  const OccupancyGrid map = room(true);
  const Candidate truth{1.2, 1.0, 0.3};
  const std::vector<Beam> scan = scan_from(map, truth, 0.0, 1);
  std::cout << "  score of a perfect scan, tolerance of one cell\n  offset in x (m)   score\n";
  double previous = 2.0;
  bool falls = true;
  double at_truth = 0.0;
  double at_far = 0.0;
  for (double dx : {0.0, 0.05, 0.1, 0.2, 0.4}) {
    Candidate c = truth;
    c.x += dx;
    const double s = score_pose(map, scan, kNoReturn, c, 1);
    std::cout << "  " << std::setw(15) << std::fixed << std::setprecision(2) << dx << "   " << std::setw(5) << std::setprecision(3) << s << "\n";
    if (dx > 0.0 && !(s <= previous)) falls = false;
    previous = s;
    if (dx == 0.0) at_truth = s;
    if (dx == 0.4) at_far = s;
  }
  std::cout << "\n";
  RC_CHECK(at_truth > 0.95);
  RC_CHECK(falls);
  RC_CHECK(at_far < 0.75 * at_truth);
  RC_CHECK_NEAR(score_pose(map, std::vector<Beam>(), kNoReturn, truth, 1), 0.0, 1e-12);
}

RC_TEST("beams that found nothing are left out, not counted as misses") {
  const OccupancyGrid map = room(true);
  std::vector<Beam> scan = scan_from(map, Candidate{1.2, 1.0, 0.3}, 0.0, 1);
  const double before = score_pose(map, scan, kNoReturn, Candidate{1.2, 1.0, 0.3}, 1);
  for (std::size_t i = 0; i < scan.size(); i += 2) scan[i].range = 0.0;       // half of them find nothing
  scan.push_back(Beam{0.0, 99.0});                                              // and one off the scale
  const double after = score_pose(map, scan, kNoReturn, Candidate{1.2, 1.0, 0.3}, 1);
  RC_CHECK(before > 0.95);
  RC_CHECK(after > 0.95);
  std::vector<Beam> none(10, Beam{0.0, 0.0});
  RC_CHECK_NEAR(score_pose(map, none, kNoReturn, Candidate{1.2, 1.0, 0.3}, 1), 0.0, 1e-12);
}

RC_TEST("a search around the odometry's guess finds the true pose to within a step") {
  const OccupancyGrid map = room(true);
  const Candidate truth{1.2, 1.0, 0.3};
  const std::vector<Beam> scan = scan_from(map, truth, 0.0, 1);
  const Candidate guess{1.2 + 0.20, 1.0 - 0.15, 0.3 + 0.10};     // odometry has drifted
  const MatchResult m = search_pose(map, scan, kNoReturn, guess, 0.30, 0.05, 0.20, 0.05, 1);
  const double position_error_before = std::hypot(guess.x - truth.x, guess.y - truth.y);
  const double position_error_after = std::hypot(m.best.x - truth.x, m.best.y - truth.y);
  std::cout << "  odometry believed (" << std::fixed << std::setprecision(2) << guess.x << ", " << guess.y << ", " << guess.theta << "), truth ("
            << truth.x << ", " << truth.y << ", " << truth.theta << ")\n  after matching: (" << m.best.x << ", " << m.best.y << ", "
            << m.best.theta << ") with score " << std::setprecision(3) << m.score << "; position error " << std::setprecision(3)
            << position_error_before << " m before, " << position_error_after << " m after; " << m.tied << " pose tied\n\n";
  RC_CHECK(m.score > 0.95);
  RC_CHECK(position_error_after < 0.08);
  RC_CHECK(position_error_after < 0.3 * position_error_before);
  RC_CHECK(std::fabs(m.best.theta - truth.theta) < 0.08);
}

RC_TEST("with noise on the ranges a tolerance of zero cells loses half the beams, and one or two cells loses almost none") {
  const OccupancyGrid map = room(true);
  const Candidate truth{1.2, 1.0, 0.3};
  const std::vector<Beam> scan = scan_from(map, truth, 0.03, 7);       // 3 cm of noise on each range
  std::cout << "  ranges noisy by 3 cm, score at the true pose\n  tolerance (cells)   score\n";
  double s0 = 0.0;
  double s1 = 0.0;
  double s2 = 0.0;
  for (int tol : {0, 1, 2}) {
    const double s = score_pose(map, scan, kNoReturn, truth, tol);
    std::cout << "  " << std::setw(17) << tol << "   " << std::fixed << std::setprecision(3) << std::setw(5) << s << "\n";
    if (tol == 0) s0 = s;
    if (tol == 1) s1 = s;
    if (tol == 2) s2 = s;
  }
  std::cout << "\n";
  RC_CHECK(s1 > s0);
  RC_CHECK(s2 >= s1);
  RC_CHECK(s0 < 0.7);
  RC_CHECK(s2 > 0.9);
}

RC_TEST("a long corridor fits the scan equally well all the way along, and a room with a block against one wall does not") {
  const OccupancyGrid hall = corridor();
  const Candidate in_hall{15.0, 0.5, 0.0};
  const std::vector<Beam> hall_scan = scan_from(hall, in_hall, 0.0, 1);
  // Search along the corridor only: y and theta fixed by a tight range, x over a metre either way.
  const MatchResult along = search_pose(hall, hall_scan, kNoReturn, in_hall, 1.0, 0.05, 0.0, 0.05, 1);

  const OccupancyGrid with_pillar = room(true);
  const Candidate in_room{1.2, 1.0, 0.3};
  const std::vector<Beam> room_scan = scan_from(with_pillar, in_room, 0.0, 1);
  const MatchResult around = search_pose(with_pillar, room_scan, kNoReturn, in_room, 1.0, 0.05, 0.0, 0.05, 1);

  std::cout << "  searching one metre round the true pose\n"
            << "  in the corridor:          best score " << std::fixed << std::setprecision(3) << along.score << ", " << along.tied << " poses tied with it\n"
            << "  in the room with a block:  best score " << around.score << ", " << around.tied << " poses tied with it\n"
            << "  the corridor's answer stays at " << std::setprecision(2) << along.best.x << " (truth " << in_hall.x << ")\n\n";
  RC_CHECK(along.score > 0.9);
  RC_CHECK(around.score > 0.9);
  RC_CHECK(along.tied >= 40);
  RC_CHECK(around.tied < 5);
  RC_CHECK(along.tied > 10 * around.tied);
  RC_CHECK_NEAR(along.best.x, in_hall.x, 1e-9);   // of equal poses, the one nearest the guess wins, so it does not wander
}

RC_TEST("an empty rectangular room cannot tell a heading from the opposite one, and a block against one wall can") {
  const OccupancyGrid empty = room(false);
  const Candidate truth{1.0, 1.0, 0.4};
  const std::vector<Beam> scan = scan_from(empty, truth, 0.0, 1);
  // Two poses that a half turn apart about the middle of the room would give: the true one and its mirror through the centre (2, 1.5).
  const Candidate mirror{4.0 - truth.x, 3.0 - truth.y, truth.theta + kPi};
  const double s_true = score_pose(empty, scan, kNoReturn, truth, 1);
  const double s_mirror = score_pose(empty, scan, kNoReturn, mirror, 1);

  const OccupancyGrid pillared = room(true);
  const std::vector<Beam> scan2 = scan_from(pillared, truth, 0.0, 1);
  const double p_true = score_pose(pillared, scan2, kNoReturn, truth, 1);
  const double p_mirror = score_pose(pillared, scan2, kNoReturn, mirror, 1);
  std::cout << "  the true pose and its mirror through the middle of the room\n"
            << "                       true   mirror\n"
            << "  empty room          " << std::fixed << std::setprecision(3) << std::setw(6) << s_true << "   " << std::setw(6) << s_mirror << "\n"
            << "  room with a block  " << std::setw(6) << p_true << "   " << std::setw(6) << p_mirror << "\n\n";
  RC_CHECK(s_true > 0.9);
  RC_CHECK(s_mirror > 0.9);
  RC_CHECK(p_true > 0.9);
  RC_CHECK(p_mirror < p_true - 0.1);
}
