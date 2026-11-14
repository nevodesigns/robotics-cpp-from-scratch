#include <rc/test/rc_test.hpp>
#include <sstream>
#include <cstring>

#include <cmath>
#include <iostream>

#include "rc/drive/offroad.hpp"
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
    {"t_policy", 20.900000000000027},
    {"plans_policy", 5.0},
    {"t_thrash", 21.400000000000034},
    {"plans_thrash", 86.0},
    {"wait_short", 2.1000000000000005},
    {"wait_long", 0.30000000000000004},
    {"jump_now", 1.1615751117363047},
    {"plans_now", 83.0},
    {"limit_now", 89.99999999999916},
    {"plans_pred", 5.0},
    {"t_pred", 20.100000000000016},
    {"jump_pred", 0.06259153741758124},
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

rc::perceive::GridSpec field() {
  rc::perceive::GridSpec s;
  s.origin_x = 0.0;
  s.origin_y = 0.0;
  s.resolution = 0.25;
  s.width = 80;
  s.height = 80;
  return s;
}

CarPose pt(double x, double y) {
  CarPose p;
  p.x = x;
  p.y = y;
  return p;
}

std::vector<CarPose> line(double x0, double x1, double y) {
  std::vector<CarPose> v;
  for (double x = x0; x <= x1 + 1e-9; x += 0.1) v.push_back(pt(x, y));
  return v;
}

struct SimOut {
  bool reached = false;
  double time = 0.0;
  int plans = 0;
  double wait = 0.0;
  int collisions = 0;
  double jump = 0.0;
};

// A point vehicle at 1 m/s from (2, 10) to (18, 10). A wall at x = 10 from y = 6 to y = 14 is in the true grid and is
// learned when the vehicle is within 4 m. mode 0: plan once; 1: plan for the reasons; 2: plan every 0.2 s.
SimOut sim(int mode, double latency, bool predicted, double check_ahead) {
  const rc::perceive::GridSpec spec = field();
  // The wall is 0.9 m thick; the planner is given it with 0.3 m more on each side, so a vehicle that cuts a corner does not touch it.
  rc::perceive::CostGrid truth(spec), padded(spec), known(spec);
  for (int j = 0; j < spec.height; ++j)
    for (int i = 0; i < spec.width; ++i) {
      const double x = truth.centre_x(i), y = truth.centre_y(j);
      if (std::fabs(x - 10.0) < 0.45 && y >= 6.0 && y <= 14.0) truth.set(i, j, 254);
      if (std::fabs(x - 10.0) < 0.75 && y >= 5.7 && y <= 14.3) padded.set(i, j, 254);
    }
  TerrainGrid terrain(spec);
  OffroadWeights w;
  ReplanConfig cfg;
  cfg.check_ahead = check_ahead;
  if (mode == 2) {
    cfg.period = 0.2;
    cfg.min_interval = 0.0;
  }
  CarPose pos = pt(2.0, 10.0);
  const CarPose goal = pt(18.0, 10.0);
  OffroadPath active = plan_offroad(terrain, &known, pos.x, pos.y, goal.x, goal.y, w);
  SimOut out;
  out.plans = 1;
  double last_plan = 0.0;
  bool pending = false;
  OffroadPath next;
  double next_time = 0.0;
  std::size_t idx = 0;
  const double dt = 0.1;
  for (double t = 0.0; t < 90.0; t += dt) {
    for (int j = 0; j < spec.height; ++j)
      for (int i = 0; i < spec.width; ++i)
        if (std::hypot(truth.centre_x(i) - pos.x, truth.centre_y(j) - pos.y) < 4.0) known.set(i, j, padded.at(i, j));
    if (pending && t >= next_time) {
      out.jump = std::max(out.jump, std::hypot(pos.x - next.points.front().x, pos.y - next.points.front().y));
      active = next;
      idx = 0;
      pending = false;
    }
    idx = nearest_index(active.points, pos.x, pos.y, idx);
    if (mode != 0 && !pending && should_replan(cfg, t, last_plan, pos, active.points, idx, known, false) != kNoReplan) {
      const CarPose start = predicted ? predicted_pose(pos, active.points, idx, 1.0, latency) : pos;
      const OffroadPath p = plan_offroad(terrain, &known, start.x, start.y, goal.x, goal.y, w);
      ++out.plans;
      last_plan = t;
      if (p.found) {
        next = p;
        next_time = t + latency;
        pending = true;
      }
    }
    if (first_blocked(known, active.points, idx, 0.5) >= 0) {
      out.wait += dt;
    } else {
      std::size_t j = idx;
      while (j + 1 < active.points.size() && std::hypot(active.points[j].x - pos.x, active.points[j].y - pos.y) < 0.4) ++j;
      const double d = std::hypot(active.points[j].x - pos.x, active.points[j].y - pos.y);
      if (d > 1e-9) {
        const double step = std::min(d, 1.0 * dt);
        pos.x += (active.points[j].x - pos.x) / d * step;
        pos.y += (active.points[j].y - pos.y) / d * step;
      }
    }
    if (truth.at(truth.cell_i(pos.x), truth.cell_j(pos.y)) >= 253) ++out.collisions;
    out.time = t;
    if (std::hypot(pos.x - goal.x, pos.y - goal.y) < 0.4) {
      out.reached = true;
      return out;
    }
  }
  return out;
}

}  // namespace

RC_TEST("the reasons come in order, and a blocked cell beyond the look-ahead is not a reason yet") {
  const rc::perceive::GridSpec spec = field();
  rc::perceive::CostGrid grid(spec);
  const std::vector<CarPose> path = line(2.0, 18.0, 10.0);
  ReplanConfig cfg;
  const CarPose on = pt(2.0, 10.0);
  RC_CHECK(should_replan(cfg, 5.0, 0.0, on, path, 0, grid, false) == kNoReplan);
  RC_CHECK(should_replan(cfg, 5.0, 0.0, on, path, 0, grid, true) == kGoalChanged);
  // A blocked cell 8 m along the path: beyond 3 m, not yet; within 3 m, yes.
  grid.set(grid.cell_i(10.0), grid.cell_j(10.0), 254);
  RC_CHECK(should_replan(cfg, 5.0, 0.0, on, path, 0, grid, false) == kNoReplan);
  RC_CHECK(should_replan(cfg, 5.0, 0.0, pt(8.0, 10.0), path, 60, grid, false) == kBlocked);
  RC_CHECK(first_blocked(grid, path, 0, 20.0) >= 0);
  RC_CHECK(first_blocked(grid, path, 0, 3.0) == -1);
}

RC_TEST("the cooldown holds back everything but a blocked path, which can be made to wait too") {
  const rc::perceive::GridSpec spec = field();
  rc::perceive::CostGrid grid(spec);
  const std::vector<CarPose> path = line(2.0, 18.0, 10.0);
  ReplanConfig cfg;
  const CarPose off = pt(5.0, 11.0);
  RC_CHECK(should_replan(cfg, 0.5, 0.0, off, path, 30, grid, false) == kNoReplan);
  RC_CHECK(should_replan(cfg, 2.0, 0.0, off, path, 30, grid, false) == kOffPath);
  RC_CHECK(should_replan(cfg, 12.0, 0.0, pt(5.0, 10.0), path, 30, grid, false) == kPeriodic);
  grid.set(grid.cell_i(6.0), grid.cell_j(10.0), 254);
  RC_CHECK(should_replan(cfg, 0.5, 0.0, pt(5.0, 10.0), path, 30, grid, false) == kBlocked);
  cfg.blocked_bypasses = false;
  RC_CHECK(should_replan(cfg, 0.5, 0.0, pt(5.0, 10.0), path, 30, grid, false) == kNoReplan);
}

RC_TEST("never replanning stops at the wall, planning for the reasons arrives, planning every tick arrives but plans a great deal") {
  const SimOut never = sim(0, 0.0, true, 3.0);
  const SimOut policy = sim(1, 0.0, true, 3.0);
  const SimOut thrash = sim(2, 0.0, true, 3.0);
  rec("t_policy", policy.time, 1);
  rec("plans_policy", policy.plans, 0);
  rec("t_thrash", thrash.time, 1);
  rec("plans_thrash", thrash.plans, 0);
  RC_CHECK(!never.reached);
  RC_CHECK(never.wait > 10.0);
  RC_CHECK(policy.reached && thrash.reached);
  RC_CHECK(policy.collisions == 0 && thrash.collisions == 0);
  RC_CHECK(thrash.plans > 10 * policy.plans);
}

RC_TEST("an early look-ahead hides the time a plan takes, a late one makes the vehicle stand and wait") {
  const SimOut late = sim(1, 2.0, true, 0.6);
  const SimOut early = sim(1, 2.0, true, 3.0);
  rec("wait_short", late.wait, 1);
  rec("wait_long", early.wait, 1);
  RC_CHECK(late.reached && early.reached);
  RC_CHECK(late.wait > early.wait + 0.5);
}

RC_TEST("a plan begun where the vehicle will be starts near it when it arrives, and one begun where it was sends the vehicle chasing its own plan") {
  const SimOut now = sim(1, 1.0, false, 3.0);
  const SimOut pred = sim(1, 1.0, true, 3.0);
  rec("jump_now", now.jump, 2);
  rec("plans_now", now.plans, 0);
  rec("limit_now", now.time, 0);
  rec("plans_pred", pred.plans, 0);
  rec("t_pred", pred.time, 1);
  rec("jump_pred", pred.jump, 2);
  RC_CHECK(pred.reached);
  RC_CHECK(!now.reached);
  RC_CHECK(now.plans > 10 * pred.plans);
  RC_CHECK(now.jump > 0.7);
  RC_CHECK(pred.jump < 0.3);
}

RC_TEST("the predicted pose is on the path, the right distance along it") {
  const std::vector<CarPose> path = line(0.0, 10.0, 0.0);
  const CarPose p = predicted_pose(pt(1.0, 0.0), path, 10, 2.0, 1.5);
  RC_CHECK_NEAR(p.x, 4.0, 1e-9);
  RC_CHECK_NEAR(p.y, 0.0, 1e-12);
  const CarPose end = predicted_pose(pt(9.0, 0.0), path, 90, 5.0, 5.0);
  RC_CHECK_NEAR(end.x, 10.0, 1e-9);
}
