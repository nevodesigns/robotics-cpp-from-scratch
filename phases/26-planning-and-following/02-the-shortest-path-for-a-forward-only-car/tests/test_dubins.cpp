#include <rc/test/rc_test.hpp>
#include <sstream>
#include <cstring>

#include <cmath>
#include <iostream>
#include <set>

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
    {"pairs", 200.0},
    {"max_pos_err", 1.0367366991496098e-14},
    {"max_head_err", 2.021286199229721e-15},
    {"words_used", 6.0},
    {"curv_limit", 1.3492540115317775},
    {"curv_small_r", 2.5002590581840134},
    {"tail_gap", 0.07324712688178923},
    {"tail_len", 4.696604359359277},
    {"tail_head", 2.7678724222398685},
    {"best_tail", 1.246872600204355},
    {"ratio_behind", 10.324246995854507},
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

CarPose pose(double x, double y, double th);
double unit();
// the draws are taken one at a time, in a written order: the order in which the arguments of a call are evaluated is the compiler's choice, and the numbers must not depend on it
CarPose random_pose() {
  const double th = 2.0 * kPi * unit();
  const double y = 10.0 * unit();
  const double x = 10.0 * unit();
  return pose(x, y, th);
}

CarPose pose(double x, double y, double th) {
  CarPose p;
  p.x = x;
  p.y = y;
  p.theta = th;
  return p;
}

unsigned long long g_state = 12345ULL;
double unit() {
  g_state = g_state * 6364136223846793005ULL + 1442695040888963407ULL;
  return static_cast<double>((g_state >> 33) & 0xFFFFFFULL) / 16777216.0;
}

rc::perceive::GridSpec field() {
  rc::perceive::GridSpec s;
  s.origin_x = 0.0;
  s.origin_y = 0.0;
  s.resolution = 0.1;
  s.width = 200;
  s.height = 200;
  return s;
}

}  // namespace

RC_TEST("the walked end of the shortest word is the goal, for many pairs of poses") {
  const double r = 0.742;
  double worst_pos = 0.0, worst_head = 0.0;
  std::set<int> used;
  int found = 0;
  for (int i = 0; i < 200; ++i) {
    const CarPose a = random_pose();
    const CarPose b = random_pose();
    const Dubins d = dubins_shortest(a, b, r);
    RC_CHECK(d.ok);
    if (!d.ok) continue;
    ++found;
    used.insert(d.word);
    const std::vector<CarPose> s = dubins_sample(d, 0.05);
    worst_pos = std::max(worst_pos, std::hypot(s.back().x - b.x, s.back().y - b.y));
    worst_head = std::max(worst_head, std::fabs(wrap_pi(s.back().theta - b.theta)));
  }
  rec("pairs", found, 0);
  rec("max_pos_err", worst_pos, 9);
  rec("max_head_err", worst_head, 9);
  rec("words_used", static_cast<double>(used.size()), 0);
  RC_CHECK(found == 200);
  RC_CHECK(worst_pos < 1e-6);
  RC_CHECK(worst_head < 1e-6);
  RC_CHECK(used.size() >= 4);
}

RC_TEST("a goal straight ahead is a straight line, and no path is shorter than the distance") {
  const Dubins d = dubins_shortest(pose(0, 0, 0.0), pose(5, 0, 0.0), 0.742);
  RC_CHECK(d.ok);
  RC_CHECK_NEAR(d.length, 5.0, 1e-9);
  for (int i = 0; i < 100; ++i) {
    const CarPose a = random_pose();
    const CarPose b = random_pose();
    const Dubins e = dubins_shortest(a, b, 0.742);
    RC_CHECK(e.length >= std::hypot(b.x - a.x, b.y - a.y) - 1e-9);
  }
}

RC_TEST("the shortest is the least of the words that exist") {
  const CarPose a = pose(1, 1, 0.3), b = pose(6, 4, 2.5);
  double least = 1e9;
  for (int w = 0; w < 6; ++w) {
    const Dubins d = dubins_word(a, b, 0.742, w);
    if (d.ok) least = std::min(least, d.length);
  }
  RC_CHECK_NEAR(dubins_shortest(a, b, 0.742).length, least, 1e-12);
}

RC_TEST("a path at the car's radius is a path the car can drive") {
  const double r = 0.33 / std::tan(0.4189);
  const double limit = 1.0 / r;
  rec("curv_limit", limit, 2);
  const Dubins d = dubins_shortest(pose(2, 2, 0.0), pose(4, 3, kPi), r);
  RC_CHECK(d.ok);
  RC_CHECK(max_curvature(dubins_sample(d, 0.02)) <= limit * 1.02);
}

RC_TEST("a radius smaller than the car's turns tighter than the car can") {
  const double limit = std::tan(0.4189) / 0.33;
  const Dubins d = dubins_shortest(pose(2, 2, 0.0), pose(4, 3, kPi), 0.4);
  RC_CHECK(d.ok);
  const double c = max_curvature(dubins_sample(d, 0.02));
  rec("curv_small_r", c, 2);
  RC_CHECK(c > limit * 1.5);
}

RC_TEST("a Dubins tail finishes the hybrid search exactly on the goal") {
  rc::perceive::CostGrid g(field());
  rc::field::World2 w;
  rc::perceive::rasterise_world(g, w);
  rc::perceive::inflate(g, 0.2, 0.2, 3.0);
  HybridConfig cfg;
  const CarPose goal = pose(12, 8, kPi / 2.0);
  const HybridResult h = hybrid_astar(g, pose(2, 2, 0.0), goal, cfg);
  RC_CHECK(h.found);
  const CarPose last = h.path.back();
  const double r = cfg.wheelbase / std::tan(cfg.max_steer);
  const Dubins tail = dubins_shortest(last, goal, r);
  rec("tail_gap", std::hypot(last.x - goal.x, last.y - goal.y), 3);
  rec("tail_len", tail.length, 2);
  rec("tail_head", std::fabs(wrap_pi(last.theta - goal.theta)) * 180.0 / kPi, 1);
  double best = tail.length;
  const std::size_t from = h.path.size() > 9 ? h.path.size() - 9 : 0;
  for (std::size_t i = from; i < h.path.size(); ++i) {
    const Dubins e = dubins_shortest(h.path[i], goal, r);
    if (e.ok) best = std::min(best, e.length);
  }
  rec("best_tail", best, 2);
  RC_CHECK(tail.ok);
  RC_CHECK(best <= tail.length);
  const std::vector<CarPose> s = dubins_sample(tail, 0.05);
  RC_CHECK(std::hypot(s.back().x - goal.x, s.back().y - goal.y) < 1e-6);
  RC_CHECK(std::fabs(wrap_pi(s.back().theta - goal.theta)) < 1e-6);
  RC_CHECK(max_curvature(s) <= 1.02 / r);
}

RC_TEST("a goal just behind the car is a long loop, because the car cannot reverse") {
  const double r = 0.742;
  const Dubins d = dubins_shortest(pose(5, 5, 0.0), pose(4.5, 5, 0.0), r);
  RC_CHECK(d.ok);
  rec("ratio_behind", d.length / 0.5, 1);
  RC_CHECK(d.length > 5.0 * 0.5);
}
