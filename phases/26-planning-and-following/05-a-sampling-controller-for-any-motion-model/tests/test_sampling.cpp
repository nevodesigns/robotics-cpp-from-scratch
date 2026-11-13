#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iostream>

#include "solution.hpp"
#include <iomanip>
#include <iostream>

static void rec(const char* key, double value, int digits = 3) {
  std::cout << "@" << key << " " << std::fixed << std::setprecision(digits) << value << std::endl;
  std::cout.unsetf(std::ios::floatfield);
}
[[maybe_unused]] static void recs(const char* key, const std::string& value) { std::cout << "@" << key << " " << value << std::endl; }


namespace {

const double kPi = 3.14159265358979323846;

CarPose pose(double x, double y, double th) {
  CarPose p;
  p.x = x;
  p.y = y;
  p.theta = th;
  return p;
}

std::vector<CarPose> straight(double x0, double x1) {
  std::vector<CarPose> v;
  for (double x = x0; x <= x1 + 1e-9; x += 0.1) v.push_back(pose(x, 0.0, 0.0));
  return v;
}

MotionModel unicycle() {
  MotionModel m;
  m.bicycle = false;
  return m;
}

MotionModel car() {
  MotionModel m;
  m.bicycle = true;
  return m;
}

CostGrid block_world() {
  rc::perceive::GridSpec s;
  s.origin_x = -2.0;
  s.origin_y = -4.0;
  s.resolution = 0.05;
  s.width = 500;
  s.height = 160;
  CostGrid g(s);
  rc::field::World2 w;
  // A block of 0.6 m square across the path at x = 5.
  w.walls.push_back(rc::field::Segment{4.7, -0.3, 5.3, -0.3});
  w.walls.push_back(rc::field::Segment{5.3, -0.3, 5.3, 0.3});
  w.walls.push_back(rc::field::Segment{5.3, 0.3, 4.7, 0.3});
  w.walls.push_back(rc::field::Segment{4.7, 0.3, 4.7, -0.3});
  rc::perceive::rasterise_world(g, w);
  rc::perceive::inflate(g, 0.2, 0.2, 3.0);
  return g;
}

}  // namespace

RC_TEST("a car that starts to the side is brought onto the path with its own model, within its limits") {
  const std::vector<CarPose> path = straight(0.0, 12.0);
  MppiConfig cfg;
  const MppiRun r = mppi_run(car(), car(), cfg, path, nullptr, nullptr, pose(0.0, 1.0, 0.0), 400, 7);
  const double limit = model_max_curvature(car()) * car().max_v;
  double worst = 0.0;
  for (std::size_t i = 0; i < r.yaw_rate.size(); ++i) worst = std::max(worst, std::fabs(r.yaw_rate[i]));
  rec("samples", cfg.samples, 0);
  rec("final_ok", r.final_cte, 3);
  RC_CHECK(r.reached);
  RC_CHECK(r.final_cte < 0.2);
  RC_CHECK(worst <= limit * 1.02);
}

RC_TEST("the same controller drives a differential drive, which may turn harder") {
  const std::vector<CarPose> path = straight(0.0, 12.0);
  MppiConfig cfg;
  const MppiRun r = mppi_run(unicycle(), unicycle(), cfg, path, nullptr, nullptr, pose(0.0, 1.0, 0.0), 400, 7);
  RC_CHECK(r.reached);
  RC_CHECK(r.final_cte < 0.2);
}

RC_TEST("planning with the wrong model hardly hurts tracking at ten plans a second, but its predictions are wrong") {
  // A path of a straight, a half circle of radius 1.5 m and a straight back, which a car can make.
  std::vector<CarPose> path = straight(0.0, 3.0);
  for (double a = 0.1 / 1.5; a <= kPi + 1e-9; a += 0.1 / 1.5) path.push_back(pose(3.0 + 1.5 * std::sin(a), 1.5 - 1.5 * std::cos(a), a));
  for (double x = 3.0; x >= -3.0; x -= 0.1) path.push_back(pose(x, 3.0, kPi));
  MppiConfig cfg;
  MotionModel slow_car = car();
  slow_car.max_v = 0.8;
  MotionModel slow_uni = unicycle();
  slow_uni.max_v = 0.8;
  double own = 0.0, wrong = 0.0;
  for (int seed = 1; seed <= 2; ++seed) {
    own += mppi_run(slow_car, slow_car, cfg, path, nullptr, nullptr, pose(0.0, 0.0, 0.0), 700, seed).mean_cte / 2.0;
    wrong += mppi_run(slow_car, slow_uni, cfg, path, nullptr, nullptr, pose(0.0, 0.0, 0.0), 700, seed).mean_cte / 2.0;
  }
  rec("err_wrong", wrong, 3);
  rec("err_own", own, 3);
  RC_CHECK(wrong < 3.0 * own + 0.05);
  // What the plan says will happen over 2 s, against what the car does with the same controls.
  CarPose pu = pose(0.0, 0.0, 0.0), pc = pose(0.0, 0.0, 0.0), pp = pose(0.0, 0.0, 0.0);
  for (int k = 0; k < 20; ++k) {
    pu = step_model(unicycle(), pu, 1.0, 1.8, 0.1);
    pc = step_model(car(), pc, 1.0, 1.8, 0.1);
    pp = step_model(car(), pp, 1.0, 1.8, 0.1);
  }
  rec("pred_gap", std::hypot(pu.x - pc.x, pu.y - pc.y), 2);
  RC_CHECK(std::hypot(pu.x - pc.x, pu.y - pc.y) > 0.3);
  RC_CHECK_NEAR(std::hypot(pp.x - pc.x, pp.y - pc.y), 0.0, 1e-12);
}

RC_TEST("a controller that knows the grid goes round a block in the path, one that does not drives into it") {
  const CostGrid g = block_world();
  const std::vector<CarPose> path = straight(0.0, 10.0);
  MppiConfig cfg;
  const MppiRun with = mppi_run(unicycle(), unicycle(), cfg, path, &g, &g, pose(0.0, 0.0, 0.0), 500, 3);
  const MppiRun blind = mppi_run(unicycle(), unicycle(), cfg, path, nullptr, &g, pose(0.0, 0.0, 0.0), 500, 3);
  rec("hits_grid", with.hits, 0);
  rec("hits_blind", blind.hits, 0);
  RC_CHECK(with.hits == 0);
  RC_CHECK(blind.hits > 0);
  RC_CHECK(with.reached);
}

RC_TEST("more samples track the path better") {
  const std::vector<CarPose> path = straight(0.0, 12.0);
  MppiConfig few, many;
  few.samples = 8;
  many.samples = 100;
  double ef = 0.0, em = 0.0;
  for (int seed = 1; seed <= 4; ++seed) {
    ef += mppi_run(car(), car(), few, path, nullptr, nullptr, pose(0.0, 1.0, 0.0), 300, seed).mean_cte / 4.0;
    em += mppi_run(car(), car(), many, path, nullptr, nullptr, pose(0.0, 1.0, 0.0), 300, seed).mean_cte / 4.0;
  }
  rec("few", few.samples, 0);
  rec("many", many.samples, 0);
  rec("err_few", ef, 3);
  rec("err_many", em, 3);
  RC_CHECK(em < ef);
}

RC_TEST("the same seed gives the same run") {
  const std::vector<CarPose> path = straight(0.0, 6.0);
  MppiConfig cfg;
  const MppiRun a = mppi_run(car(), car(), cfg, path, nullptr, nullptr, pose(0.0, 0.5, 0.0), 200, 11);
  const MppiRun b = mppi_run(car(), car(), cfg, path, nullptr, nullptr, pose(0.0, 0.5, 0.0), 200, 11);
  RC_CHECK(a.trace.size() == b.trace.size());
  RC_CHECK_NEAR(a.trace.back().x, b.trace.back().x, 1e-12);
  RC_CHECK_NEAR(a.trace.back().y, b.trace.back().y, 1e-12);
}
