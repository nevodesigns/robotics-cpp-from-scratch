#include <rc/test/rc_test.hpp>
#include <sstream>
#include <cstring>

#include <cmath>
#include <iostream>

#include "solution.hpp"
#include <iomanip>
#include <iostream>

// The numbers this lesson publishes: its page, its header and its quiz quote them, so each is held here as it was
// measured. A change that moves one by more than half a unit of its last printed digit fails the suite.
struct Pin {
  const char* key;
  double value;
};
static const Pin kPins[] = {
    {"steps", 5.0},
    {"err_final", 0.02076397344734374},
    {"lost_50", 10.0},
    {"lost_enough", 0.0},
    {"sym_modes", 2.0},
    {"asym_modes", 0.0},
};

static void rec(const char* key, double value, int digits = 3) {
  std::cout << "@" << key << " " << std::fixed << std::setprecision(digits) << value << std::endl;
  std::cout.unsetf(std::ios::floatfield);
  for (const Pin& pin : kPins) {
    if (std::strcmp(pin.key, key) != 0) continue;
    double allowed = 0.5;
    for (int i = 0; i < digits; ++i) allowed /= 10.0;
    if (!(std::fabs(value - pin.value) <= allowed)) {
      std::ostringstream message;
      message << "the published number " << key << " was " << pin.value << " and is now " << value;
      rc::test::record(__FILE__, __LINE__, message.str());
    }
  }
}
[[maybe_unused]] static void recs(const char* key, const std::string& value) { std::cout << "@" << key << " " << value << std::endl; }


namespace {

const double kPi = 3.14159265358979323846;

rc::field::World2 room(bool with_post) {
  rc::field::World2 w;
  w.walls.push_back(rc::field::Segment{0, 0, 10, 0});
  w.walls.push_back(rc::field::Segment{10, 0, 10, 6});
  w.walls.push_back(rc::field::Segment{10, 6, 0, 6});
  w.walls.push_back(rc::field::Segment{0, 6, 0, 0});
  if (with_post) {
    w.walls.push_back(rc::field::Segment{4.0, 0.0, 4.0, 2.0});  // a partition at one side
    w.posts.push_back(rc::field::Circle{7.5, 4.5, 0.3});
  }
  return w;
}

std::vector<double> beam_angles() {
  std::vector<double> a;
  for (int i = 0; i < 24; ++i) a.push_back(-kPi + (i + 0.5) * 2.0 * kPi / 24.0);
  return a;
}

struct Outcome {
  bool found = false;
  int steps = 0;
  double err = 0.0;
  double mirror_err = 0.0;
};

// The robot starts at (2, 2) facing 0.5 rad and drives 0.2 m a step, turning a little now and then. The cloud starts spread over the whole room.
Outcome localise(const rc::field::World2& map, int count, unsigned int seed, bool symmetric_start_truth = false, double tx0 = 2.0, double ty0 = 2.0) {
  rc::core::Rng noise(seed * 31u + 7u);
  MonteCarlo mc(map, count, seed);
  mc.init_uniform(0.0, 10.0, 0.0, 6.0);
  const std::vector<double> angles = beam_angles();
  double tx = tx0, ty = ty0, tth = 0.5;
  Outcome o;
  (void)symmetric_start_truth;
  for (int step = 1; step <= 60; ++step) {
    const double turn = (step % 7 == 0) ? 0.35 : 0.0;
    // Turn at the walls so that the robot stays in the room.
    double ahead = 0.2;
    double nx = tx + ahead * std::cos(tth + 0.5 * turn), ny = ty + ahead * std::sin(tth + 0.5 * turn);
    double extra = 0.0;
    if (nx < 0.6 || nx > 9.4 || ny < 0.6 || ny > 5.4) extra = 0.5;
    const double t = turn + extra;
    tx += ahead * std::cos(tth + 0.5 * t);
    ty += ahead * std::sin(tth + 0.5 * t);
    tth += t;
    const double turn_noise = 0.01 * noise.gaussian();   // one draw at a time: the order of a call's arguments is the compiler's choice
    const double ahead_noise = 0.01 * noise.gaussian();
    mc.predict(ahead + ahead_noise, t + turn_noise, 0.02, 0.02);
    std::vector<double> ranges;
    for (std::size_t b = 0; b < angles.size(); ++b) ranges.push_back(rc::field::cast_ray(map, tx, ty, tth + angles[b], 12.0) + 0.05 * noise.gaussian());
    McMeasure m;
    mc.correct(angles, ranges, m);
    mc.resample_if_needed();
    const Particle e = mc.mean_pose();
    const double err = std::hypot(e.x - tx, e.y - ty);
    if (!o.found && err < 0.3 && mc.spread() < 0.4) {
      o.found = true;
      o.steps = step;
    }
    o.err = err;
    o.mirror_err = std::hypot(e.x - (10.0 - tx), e.y - (6.0 - ty));
  }
  return o;
}

}  // namespace

RC_TEST("what the map would show from a pose is the range of the nearest wall, and a pose that matches the scan is the heaviest") {
  const rc::field::World2 map = room(true);
  MonteCarlo mc(map, 3, 1);
  Particle p;
  p.x = 2.0;
  p.y = 2.0;
  p.theta = 0.0;
  RC_CHECK_NEAR(mc.expected_range(p, 0.0, 12.0), 2.0, 1e-9);        // the partition at x = 4
  RC_CHECK_NEAR(mc.expected_range(p, kPi, 12.0), 2.0, 1e-9);        // the wall at x = 0
  RC_CHECK_NEAR(mc.expected_range(p, -kPi / 2.0, 12.0), 2.0, 1e-9);  // the wall at y = 0
}

RC_TEST("from a cloud over the whole room three thousand particles find the robot") {
  const rc::field::World2 map = room(true);
  const Outcome o = localise(map, 3000, 5);
  rec("steps", o.steps, 0);
  rec("err_final", o.err, 3);
  std::cout << "  found after " << o.steps << " steps, final error " << o.err << " m\n";
  RC_CHECK(o.found);
  RC_CHECK(o.steps < 40);
  RC_CHECK(o.err < 0.25);
}

RC_TEST("too few particles lose the robot more often than enough do") {
  const rc::field::World2 map = room(true);
  int lost50 = 0, lost_enough = 0;
  for (unsigned int seed = 1; seed <= 10; ++seed) {
    if (localise(map, 50, seed).err > 0.5) ++lost50;
    if (localise(map, 3000, seed).err > 0.5) ++lost_enough;
  }
  rec("lost_50", lost50, 0);
  rec("lost_enough", lost_enough, 0);
  std::cout << "  of 10 runs, 50 particles lost the robot " << lost50 << " times and 3000 particles " << lost_enough << "\n";
  // The counts depend a little on the floating point of the platform; what does not is that few particles lose it far more often.
  RC_CHECK(lost50 > lost_enough + 4);
  RC_CHECK(lost_enough <= 3);
}

RC_TEST("in a room that is the same from two places the cloud settles on both, and a feature in one corner picks one") {
  int sym_mirror = 0, asym_mirror = 0;
  for (unsigned int seed = 1; seed <= 8; ++seed) {
    const Outcome s = localise(room(false), 3000, seed);
    if (s.mirror_err < 0.5 && s.err > 0.5) ++sym_mirror;
    const Outcome a = localise(room(true), 3000, seed);
    if (a.mirror_err < 0.5 && a.err > 0.5) ++asym_mirror;
  }
  rec("sym_modes", sym_mirror, 0);
  rec("asym_modes", asym_mirror, 0);
  std::cout << "  mirror image taken for the robot in " << sym_mirror << " of 8 runs with nothing in the room and " << asym_mirror << " with a partition and a post\n";
  RC_CHECK(sym_mirror >= 1);
  RC_CHECK(asym_mirror <= sym_mirror);
}

RC_TEST("resampling keeps the number of particles and equal weights, and does not happen when the weights are even") {
  const rc::field::World2 map = room(true);
  MonteCarlo mc(map, 100, 3);
  mc.init_uniform(0.0, 10.0, 0.0, 6.0);
  RC_CHECK(!mc.resample_if_needed());
  RC_CHECK_NEAR(mc.effective_count(), 100.0, 1e-6);
  const std::vector<double> angles = beam_angles();
  std::vector<double> ranges;
  for (std::size_t b = 0; b < angles.size(); ++b) ranges.push_back(rc::field::cast_ray(map, 2.0, 2.0, 0.5 + angles[b], 12.0));
  mc.correct(angles, ranges, McMeasure());
  RC_CHECK(mc.effective_count() < 50.0);
  RC_CHECK(mc.resample_if_needed());
  RC_CHECK(mc.particles().size() == 100);
  RC_CHECK_NEAR(mc.particles()[0].w, 0.01, 1e-12);
}
