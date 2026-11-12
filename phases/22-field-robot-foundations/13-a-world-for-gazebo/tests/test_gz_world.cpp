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

const char* kBurger =
    "robot burger\nkind diff_drive\n"
    "wheel_radius 0.033 source test\ntrack_width 0.160 source test\nlength 0.138 source test\nwidth 0.178 source test\n"
    "height 0.192 source test\nmass 1.0 source test\nwheel_mass 0.03 source typical\nclearance 0.02 source typical\n"
    "wheel_width 0.018 source typical\nmax_speed 0.22 source test\nmax_turn_rate 2.84 source test\n";

const char* kYard =
    "scenario yard\nduration 30\nground gravel\n"
    "patch sand 5 -3 10 3\n"
    "patch ice 12 -3 14 3 0.1 0\n"
    "wall 0 0 3 4\n"
    "wall -2 -5 12 -5\n"
    "post 3 2 0.3\n"
    "start 1 2 90\n"
    "goal 8 1 0.5\n";

bool balanced(const std::string& svg) {
  std::vector<std::string> stack;
  std::size_t i = 0;
  while ((i = svg.find('<', i)) != std::string::npos) {
    const std::size_t end = svg.find('>', i);
    if (end == std::string::npos) return false;
    std::string tag = svg.substr(i + 1, end - i - 1);
    i = end + 1;
    if (tag.empty() || tag[0] == '?' || tag[0] == '!' || tag.back() == '/') continue;
    if (tag[0] == '/') {
      if (stack.empty() || stack.back() != tag.substr(1)) return false;
      stack.pop_back();
    } else {
      const std::size_t space = tag.find_first_of(" \n");
      stack.push_back(space == std::string::npos ? tag : tag.substr(0, space));
    }
  }
  return stack.empty();
}

int count_of(const std::string& s, const std::string& what) {
  int n = 0;
  for (std::size_t i = s.find(what); i != std::string::npos; i = s.find(what, i + what.size())) ++n;
  return n;
}

}  // namespace

RC_TEST("a wall becomes a box along it: its middle, its length and its angle") {
  rc::field::Segment s{0.0, 0.0, 3.0, 4.0};
  const WorldBox b = wall_box(s, 0.1, 1.0);
  rec("wall_len", b.sx, 3);
  rec("wall_yaw", b.yaw, 6);
  RC_CHECK_NEAR(b.cx, 1.5, 1e-12);
  RC_CHECK_NEAR(b.cy, 2.0, 1e-12);
  RC_CHECK_NEAR(b.cz, 0.5, 1e-12);
  RC_CHECK_NEAR(b.sx, 5.0, 1e-12);
  RC_CHECK_NEAR(b.yaw, std::atan2(4.0, 3.0), 1e-12);
  const WorldBox down = wall_box(rc::field::Segment{3.0, 4.0, 0.0, 0.0}, 0.1, 1.0);
  RC_CHECK_NEAR(down.sx, 5.0, 1e-12);
  RC_CHECK_NEAR(std::cos(down.yaw), -std::cos(b.yaw), 1e-12);
}

RC_TEST("a grade along x lowers the front of the slab and a grade along y raises the left") {
  FieldPatch p;
  p.grade_x = 0.1;
  double roll = 9.0, pitch = 9.0;
  patch_tilt(p, roll, pitch);
  rec("pitch", pitch, 7);
  RC_CHECK_NEAR(pitch, -std::atan(0.1), 1e-12);
  RC_CHECK_NEAR(roll, 0.0, 1e-12);
  FieldPatch q;
  q.grade_y = 0.2;
  patch_tilt(q, roll, pitch);
  RC_CHECK_NEAR(roll, std::atan(0.2), 1e-12);
  RC_CHECK_NEAR(pitch, 0.0, 1e-12);
  // The surface of a slab pitched by minus atan(g) rises along x by g a metre.
  const double c = std::cos(-std::atan(0.1)), s = std::sin(-std::atan(0.1));
  const double rise = -s / c;  // the z of the image of (1, 0, 0) is -sin(pitch), over its x, cos(pitch)
  RC_CHECK_NEAR(rise, 0.1, 1e-12);
}

RC_TEST("the world has a model for everything in the scenario, with its friction, and the robot at its start") {
  const ScenarioParse sp = parse_scenario(kYard);
  RC_CHECK(sp.ok());
  const RobotParse rp = parse_robot(kBurger);
  RC_CHECK(rp.ok());
  const std::string world = to_world(sp.scenario, rp.robot);
  RC_CHECK(balanced(world));
  RC_CHECK(count_of(world, "<model name=") == 1 + 2 + 2 + 1 + 1 + 1);
  RC_CHECK(count_of(world, "<static>true</static>") == 1 + 2 + 2 + 1 + 1);
  rec("n_models", count_of(world, "<model name="), 0);
  rec("n_patches", 2.0, 0);
  rec("n_walls", 2.0, 0);
  rec("world_kb", static_cast<double>(world.size()) / 1000.0, 1);
  RC_CHECK(world.find("<mu>0.45</mu>") != std::string::npos);  // the gravel ground
  RC_CHECK(world.find("<mu>0.3</mu>") != std::string::npos);   // the sand
  RC_CHECK(world.find("<mu>0.1</mu>") != std::string::npos);   // the ice
  RC_CHECK(world.find("<pose>1.5 2 0.5 0 0 0.927295</pose>") != std::string::npos);
  RC_CHECK(world.find("<size>5 0.1 1</size>") != std::string::npos);
  RC_CHECK(world.find("-0.0996687") != std::string::npos);
  RC_CHECK(world.find("<pose>1 2 0 0 0 1.5708</pose>") != std::string::npos);
  RC_CHECK(world.find("gz-sim-physics-system") != std::string::npos);
  RC_CHECK(world.find("<max_step_size>0.001</max_step_size>") != std::string::npos);
  RC_CHECK(world.find("nan") == std::string::npos);
  RC_CHECK(to_world(sp.scenario, rp.robot) == world);
}

RC_TEST("the goal is a marker with nothing to collide with") {
  const ScenarioParse sp = parse_scenario(kYard);
  const std::string world = to_world(sp.scenario, parse_robot(kBurger).robot);
  const std::size_t at = world.find("<model name=\"goal_0\">");
  RC_CHECK(at != std::string::npos);
  const std::size_t end = world.find("</model>", at);
  const std::string goal = world.substr(at, end - at);
  RC_CHECK(goal.find("<collision") == std::string::npos);
  RC_CHECK(goal.find("<visual") != std::string::npos);
}
