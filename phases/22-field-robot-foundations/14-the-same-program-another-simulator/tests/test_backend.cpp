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


#ifdef RC_HAVE_GZ
#include <rc/gz/gz_backend.hpp>
#endif

namespace {

const double kPi = 3.14159265358979323846;

const char* kBurger =
    "robot burger\nkind diff_drive\n"
    "wheel_radius 0.033 source test\ntrack_width 0.160 source test\nlength 0.138 source test\nwidth 0.178 source test\n"
    "height 0.192 source test\nmass 1.0 source test\nwheel_mass 0.03 source typical\nclearance 0.02 source typical\n"
    "wheel_width 0.018 source typical\nmax_speed 0.22 source test\nmax_turn_rate 2.84 source test\n";

const char* kYard = "scenario yard\nduration 30\nground gravel\nwall -5 -5 5 -5\nstart 1 2 90\ngoal 1 4 0.3\n";

rc::field::FieldScenario scenario() { return rc::field::parse_scenario(kYard).scenario; }
rc::field::RobotModel robot() { return rc::field::parse_robot(kBurger).robot; }

// The program under test: it drives forward, then turns in place, then drives again, and reports where it ended.
std::vector<ScriptStep> program() {
  std::vector<ScriptStep> s;
  s.push_back(ScriptStep{0.2, 0.0, 5.0});
  s.push_back(ScriptStep{0.0, 1.0, kPi / 2.0});
  s.push_back(ScriptStep{0.2, 0.0, 3.0});
  return s;
}

}  // namespace

RC_TEST("the built in backend starts at the scenario's start, and refuses a scenario with none") {
  BuiltinBackend b;
  RC_CHECK(b.name() == "builtin");
  RC_CHECK(!b.start(rc::field::parse_scenario("scenario x\nduration 1\n").scenario, robot()));
  RC_CHECK(b.start(scenario(), robot()));
  const BackendPose p = b.pose();
  RC_CHECK_NEAR(p.x, 1.0, 1e-12);
  RC_CHECK_NEAR(p.y, 2.0, 1e-12);
  RC_CHECK_NEAR(p.theta, kPi / 2.0, 1e-12);
  RC_CHECK_NEAR(p.t, 0.0, 1e-12);
}

RC_TEST("a straight drive goes where speed times time, less the lag, says, along the heading") {
  BuiltinBackend b;
  RC_CHECK(b.start(scenario(), robot()));
  const std::vector<BackendPose> trace = run_script(b, std::vector<ScriptStep>(1, ScriptStep{0.2, 0.0, 5.0}), 0.1);
  RC_CHECK(trace.size() == 51);
  const BackendPose end = trace.back();
  rec("b_y", end.y, 3);
  RC_CHECK_NEAR(end.t, 5.0, 1e-6);
  RC_CHECK_NEAR(end.x, 1.0, 1e-9);
  RC_CHECK_NEAR(end.y, 2.0 + 0.2 * (5.0 - 0.05), 0.002);
  for (std::size_t i = 1; i < trace.size(); ++i) RC_CHECK(trace[i].y >= trace[i - 1].y);
}

RC_TEST("a quarter turn in place turns a quarter of a turn, and the limits of the file hold a command back") {
  BuiltinBackend b;
  RC_CHECK(b.start(scenario(), robot()));
  const std::vector<BackendPose> t = run_script(b, std::vector<ScriptStep>(1, ScriptStep{0.0, 1.0, kPi / 2.0}), 0.1);
  const double turned = angle_between(t.back().theta, kPi / 2.0) * 180.0 / kPi;
  rec("b_turn", turned, 1);
  // The script is cut into samples of 0.1 s, so 1.5708 s becomes 16 of them, 1.6 s; the lag costs the turn rate times the time constant.
  RC_CHECK_NEAR(turned, (1.6 - 0.05) * 180.0 / kPi, 0.1);
  RC_CHECK_NEAR(t.back().x, 1.0, 0.02);

  BuiltinBackend fast;
  RC_CHECK(fast.start(scenario(), robot()));
  const std::vector<BackendPose> f = run_script(fast, std::vector<ScriptStep>(1, ScriptStep{5.0, 0.0, 10.0}), 0.5);
  const double speed = (f.back().y - f[f.size() - 2].y) / 0.5;
  rec("b_speed", speed, 2);
  RC_CHECK_NEAR(speed, 0.22, 1e-3);
}

RC_TEST("the program is written against Backend and is the same program for any of them") {
  BuiltinBackend slow(0.3);
  BuiltinBackend quick(0.02);
  Backend* both[2] = {&slow, &quick};
  double ends[2] = {0.0, 0.0};
  for (int i = 0; i < 2; ++i) {
    RC_CHECK(both[i]->start(scenario(), robot()));
    const std::vector<BackendPose> t = run_script(*both[i], program(), 0.1);
    ends[i] = t.back().x;
  }
  // The heavier lag is behind: after a turn the program drives off along a heading that is a little short of a half turn.
  RC_CHECK(std::fabs(ends[0] - ends[1]) > 1e-4);
  RC_CHECK(slow.name() == quick.name());
}

#ifdef RC_HAVE_GZ
// The program is the same as above, but written with the library's types, because the library's backends are what the Gazebo backend is.
std::vector<rc::field::ScriptStep> library_program() {
  std::vector<rc::field::ScriptStep> s;
  s.push_back(rc::field::ScriptStep{0.2, 0.0, 5.0});
  s.push_back(rc::field::ScriptStep{0.0, 1.0, kPi / 2.0});
  s.push_back(rc::field::ScriptStep{0.2, 0.0, 3.0});
  return s;
}

RC_TEST("the same program on Gazebo Sim: it starts, drives, turns and ends near where the built in model does") {
  rc::gz::GzBackend g;
  RC_CHECK(g.start(scenario(), robot()));
  const rc::field::BackendPose first = g.pose();
  RC_CHECK_NEAR(first.x, 1.0, 0.01);
  RC_CHECK_NEAR(first.y, 2.0, 0.01);
  RC_CHECK_NEAR(first.theta, kPi / 2.0, 0.01);
  const std::vector<rc::field::BackendPose> gt = rc::field::run_script(g, library_program(), 0.1);

  rc::field::BuiltinBackend b;
  RC_CHECK(b.start(scenario(), robot()));
  const std::vector<rc::field::BackendPose> bt = rc::field::run_script(b, library_program(), 0.1);
  const rc::field::BackendPose ge = gt.back();
  const rc::field::BackendPose be = bt.back();
  const double gap = std::hypot(ge.x - be.x, ge.y - be.y);
  const double turn_gap = std::fabs(rc::field::angle_between(ge.theta, be.theta)) * 180.0 / kPi;
  std::cout << "  gazebo ended at (" << ge.x << ", " << ge.y << ") facing " << ge.theta << " at " << ge.t << " s; built in at (" << be.x << ", " << be.y << ") facing " << be.theta << " at " << be.t << " s\n";
  rec("gz_end_x", ge.x, 3);
  rec("gz_end_y", ge.y, 3);
  rec("b_end_x", be.x, 3);
  rec("b_end_y", be.y, 3);
  rec("gap", gap, 3);
  rec("turn_gap", turn_gap, 1);
  rec("gz_steps", static_cast<double>(gt.size()), 0);
  RC_CHECK(gt.size() == bt.size());
  RC_CHECK_NEAR(ge.t, be.t, 0.01);
  // They are not the same robot yet; the next lesson measures why and closes the gap. Here they only have to be in the same neighbourhood.
  RC_CHECK(gap < 0.5);
  RC_CHECK(turn_gap < 45.0);
}
#endif
