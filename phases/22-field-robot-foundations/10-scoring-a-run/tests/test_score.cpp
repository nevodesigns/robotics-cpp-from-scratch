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


namespace {

// A polyline walked at 0.02 s a sample, with the vertices among the samples: n samples a leg.
RunLog walk(const std::vector<double>& xs, const std::vector<double>& ys, int n, double dt = 0.02) {
  RunLog log;
  double t = 0.0;
  log.add(t, "pose", {xs[0], ys[0], 0.0});
  for (std::size_t leg = 1; leg < xs.size(); ++leg) {
    for (int k = 1; k <= n; ++k) {
      const double u = static_cast<double>(k) / n;
      t += dt;
      log.add(t, "pose", {xs[leg - 1] + u * (xs[leg] - xs[leg - 1]), ys[leg - 1] + u * (ys[leg] - ys[leg - 1]), 0.0});
    }
  }
  return log;
}

FieldScenario room() {
  const ScenarioParse p = parse_scenario(
      "scenario t\nduration 100\nstart 0 0 0\nwall -1 -2 11 -2\npost 5 0 0.3\npost 8 0 0.3\ngoal 10 0 0.3\n");
  return p.scenario;
}

}  // namespace

RC_TEST("a straight run is as efficient as a run can be, and a detour of the same distance apart is not") {
  FieldScenario sc = room();
  sc.posts.clear();
  const NavScore straight = score_navigation(sc, walk({0, 10}, {0, 0}, 500), 0.25);
  rec("eff_straight", straight.straightness, 4);
  RC_CHECK(straight.goals_reached == 1);
  RC_CHECK_NEAR(straight.path_length, 10.0, 1e-9);
  RC_CHECK_NEAR(straight.straightness, 1.0, 1e-9);
  RC_CHECK_NEAR(straight.time_to_last, 9.7, 0.021);
  RC_CHECK_NEAR(straight.max_speed, 1.0, 1e-6);
  RC_CHECK_NEAR(straight.min_clearance, 2.0 - 0.25, 1e-9);
  RC_CHECK(straight.collisions == 0);

  const NavScore v = score_navigation(sc, walk({0, 5, 10}, {0, 2, 0}, 250), 0.25);
  rec("eff_v", v.straightness, 4);
  RC_CHECK_NEAR(v.path_length, 2.0 * std::sqrt(29.0), 1e-9);
  RC_CHECK_NEAR(v.straightness, 10.0 / (2.0 * std::sqrt(29.0)), 1e-9);
  RC_CHECK(v.goals_reached == 1);
}

RC_TEST("a stretch of samples inside a post is one collision, and the clearance says how far inside") {
  const FieldScenario sc = room();
  const RunLog through = walk({0, 10}, {0, 0}, 500);
  const NavScore s = score_navigation(sc, through, 0.25);
  int samples = 0;
  for (std::size_t i = 0; i < through.entries.size(); ++i)
    if (clearance_at(sc, through.entries[i].values[0], through.entries[i].values[1]) - 0.25 < 0.0) ++samples;
  rec("samples", static_cast<double>(samples), 0);
  rec("events", static_cast<double>(s.collisions) / 2.0, 0);
  rec("both", static_cast<double>(s.collisions), 0);
  rec("min_clear", s.min_clearance, 2);
  RC_CHECK(samples > 100);
  RC_CHECK(s.collisions == 2);
  RC_CHECK_NEAR(s.min_clearance, -0.55, 0.02);
  const NavScore around = score_navigation(sc, walk({0, 5, 10}, {0, 1.5, 0}, 250), 0.25);
  RC_CHECK(around.collisions == 0);
}

RC_TEST("goals count in the order they were given, and a goal reached early does not count for the next") {
  FieldScenario sc = room();
  sc.goals.clear();
  sc.goals.push_back(FieldGoal{10.0, 0.0, 0.3});
  sc.goals.push_back(FieldGoal{5.0, 0.0, 0.3});
  const NavScore there_and_back = score_navigation(sc, walk({0, 10, 5}, {0, 0, 0}, 500), 0.25);
  RC_CHECK(there_and_back.goals_reached == 2);
  const NavScore wrong_way = score_navigation(sc, walk({0, 5}, {0, 0}, 500), 0.25);
  RC_CHECK(wrong_way.goals_reached == 0);
  RC_CHECK(wrong_way.time_to_last < 0.0);
}

RC_TEST("criteria turn a score into a list of what failed") {
  FieldScenario sc = room();
  sc.posts.clear();
  Criteria c;
  c.goals_needed = 1;
  c.max_time = 20.0;
  c.min_straightness = 0.95;
  c.min_clearance = 0.5;
  RC_CHECK(failures(score_navigation(sc, walk({0, 10}, {0, 0}, 500), 0.25), c).empty());
  const std::vector<std::string> bad = failures(score_navigation(sc, walk({0, 5, 10}, {0, 2, 0}, 250), 0.25), c);
  for (std::size_t i = 0; i < bad.size(); ++i) std::cout << "  fails: " << bad[i] << "\n";
  RC_CHECK(bad.size() == 1);
  RC_CHECK(bad[0].find("straightness") != std::string::npos);
  const std::vector<std::string> none = failures(score_navigation(sc, walk({0, 3}, {0, 0}, 150), 0.25), c);
  RC_CHECK(none.size() == 1);
  RC_CHECK(none[0].find("reached 0 goals of 1") != std::string::npos);
}

RC_TEST("manipulation trials: the mean hides the slow ones and a failed trial does not count as fast") {
  std::vector<Trial> trials;
  for (int i = 0; i < 20; ++i) {
    Trial t;
    t.success = i != 3 && i != 11;
    t.seconds = 4.0 + 0.05 * i;
    t.error_mm = 0.5 + 0.1 * (i % 5);
    if (i == 17) t.seconds = 9.0;
    trials.push_back(t);
  }
  const TrialSummary s = summarise_trials(trials);
  rec("success", s.success_rate, 1);
  rec("mean_time", s.mean_time, 2);
  rec("p95_time", s.p95_time, 2);
  RC_CHECK(s.count == 20);
  RC_CHECK_NEAR(s.success_rate, 90.0, 1e-9);
  RC_CHECK(s.p95_time > s.mean_time + 1.0);
  RC_CHECK_NEAR(s.p95_time, 9.0, 1e-9);
  RC_CHECK_NEAR(s.worst_error_mm, 0.9, 1e-9);
}

RC_TEST("gait metrics: duty factor, stride period and how irregular a stumble is") {
  std::vector<int> steady;
  std::vector<int> stumble;
  for (int i = 0; i < 1000; ++i) {
    steady.push_back((i % 50) < 30 ? 1 : 0);
    // the same, but one stride is missed (the foot stays up for a whole extra period)
    const bool missed = i >= 400 && i < 450;
    stumble.push_back(((i % 50) < 30 && !missed) ? 1 : 0);
  }
  const GaitMetrics a = gait_metrics(steady, 0.01);
  const GaitMetrics b = gait_metrics(stumble, 0.01);
  rec("duty", a.duty_factor, 2);
  rec("period", a.stride_period, 3);
  rec("cv_steady", a.irregularity, 3);
  rec("cv_stumble", b.irregularity, 3);
  RC_CHECK_NEAR(a.duty_factor, 0.6, 1e-9);
  RC_CHECK_NEAR(a.stride_period, 0.5, 1e-9);
  RC_CHECK(a.irregularity < 1e-9);
  RC_CHECK(b.irregularity > 0.1);
  RC_CHECK(b.duty_factor < a.duty_factor);
}
