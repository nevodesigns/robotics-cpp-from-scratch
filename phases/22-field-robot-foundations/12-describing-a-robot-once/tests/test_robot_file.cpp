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

// A TurtleBot3 Burger class base. Where a number was found, the source says where; where it was not, it says typical.
const char* kBurger =
    "# a small differential drive base, after the TurtleBot3 Burger\n"
    "robot burger\n"
    "kind diff_drive\n"
    "wheel_radius 0.033 source ROBOTIS TurtleBot3 Burger constants, as quoted in the Webots example\n"
    "track_width 0.160 source ROBOTIS TurtleBot3 Burger constants, as quoted in the Webots example\n"
    "length 0.138 source third party guide to the Burger, not checked against the e-Manual\n"
    "width 0.178 source third party guide to the Burger, not checked against the e-Manual\n"
    "height 0.192 source third party guide to the Burger, not checked against the e-Manual\n"
    "mass 1.0 source third party guide to the Burger, about\n"
    "wheel_mass 0.03 source typical\n"
    "clearance 0.02 source typical\n"
    "wheel_width 0.018 source typical\n"
    "max_speed 0.22 source ROBOTIS e-Manual, Burger\n"
    "max_turn_rate 2.84 source ROBOTIS e-Manual, Burger\n";

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

}  // namespace

RC_TEST("the reference robot parses, every number has its source, and the typical ones are counted") {
  const RobotParse p = parse_robot(kBurger);
  for (std::size_t i = 0; i < p.problems.size(); ++i) std::cout << "  " << p.problems[i].line << ": " << p.problems[i].message << "\n";
  RC_CHECK(p.ok());
  rec("n_numbers", p.numbers, 0);
  rec("n_typical", p.typical, 0);
  RC_CHECK(p.numbers == 11);
  RC_CHECK(p.typical == 3);
  RC_CHECK(p.robot.name == "burger");
  RC_CHECK_NEAR(p.robot.track_width.value, 0.160, 1e-12);
  RC_CHECK(p.robot.max_speed.source == "ROBOTIS e-Manual, Burger");
  RC_CHECK(p.robot.wheel_mass.is_typical());
  RC_CHECK(!p.robot.wheel_radius.is_typical());
}

RC_TEST("a number with no source, a given twice, an unknown key and a missing one are all refused") {
  const RobotParse p = parse_robot(
      "robot r\nkind diff_drive\n"
      "wheel_radius 0.033\n"
      "track_width 0.16 source typical\n"
      "track_width 0.17 source typical\n"
      "colour red\n"
      "mass lots source typical\n");
  for (std::size_t i = 0; i < p.problems.size(); ++i) std::cout << "  " << p.problems[i].line << ": " << p.problems[i].message << "\n";
  RC_CHECK(!p.ok());
  RC_CHECK(p.problems[0].line == 3);
  RC_CHECK(p.problems[0].message.find("no source") != std::string::npos);
  RC_CHECK(p.problems[1].line == 5);
  RC_CHECK(p.problems[1].message.find("twice") != std::string::npos);
  RC_CHECK(p.problems[2].line == 6);
  RC_CHECK(p.problems[3].line == 7);
  RC_CHECK(p.problems[3].message.find("needs a number") != std::string::npos);
  RC_CHECK(p.problems.size() > 8);  // and the keys that were never given
}

RC_TEST("the inertias are the ones of the shapes") {
  const Inertia b = box_inertia(1.0, 0.138, 0.178, 0.172);
  rec("ixx", b.ixx, 5);
  RC_CHECK_NEAR(b.ixx, (0.178 * 0.178 + 0.172 * 0.172) / 12.0, 1e-12);
  RC_CHECK_NEAR(b.izz, (0.138 * 0.138 + 0.178 * 0.178) / 12.0, 1e-12);
  const Inertia c = cylinder_inertia(0.03, 0.033, 0.018);
  RC_CHECK_NEAR(c.izz, 0.5 * 0.03 * 0.033 * 0.033, 1e-15);
  RC_CHECK_NEAR(c.ixx, 0.03 / 12.0 * (3.0 * 0.033 * 0.033 + 0.018 * 0.018), 1e-15);
  RC_CHECK(c.ixx == c.iyy);
  // For any body, no moment is more than the sum of the other two.
  RC_CHECK(b.ixx <= b.iyy + b.izz && b.iyy <= b.ixx + b.izz && b.izz <= b.ixx + b.iyy);
  RC_CHECK(c.izz <= c.ixx + c.iyy);
}

RC_TEST("the same file gives the built in limits and the SDF, with the same numbers in both") {
  const RobotParse p = parse_robot(kBurger);
  const VehicleLimits lim = limits_of(p.robot);
  const rc::hal::DriveLimits dl = drive_limits_of(p.robot);
  RC_CHECK_NEAR(lim.max_v.mps, 0.22, 1e-12);
  RC_CHECK_NEAR(lim.max_w.rps, 2.84, 1e-12);
  RC_CHECK_NEAR(dl.max_v, 0.22, 1e-12);
  RC_CHECK_NEAR(dl.max_w, 2.84, 1e-12);

  const std::string sdf = to_sdf(p.robot);
  rec("sdf_kb", static_cast<double>(sdf.size()) / 1000.0, 1);
  RC_CHECK(balanced(sdf));
  RC_CHECK(sdf.find("<model name=\"burger\">") != std::string::npos);
  RC_CHECK(sdf.find("<wheel_separation>0.16</wheel_separation>") != std::string::npos);
  RC_CHECK(sdf.find("<wheel_radius>0.033</wheel_radius>") != std::string::npos);
  RC_CHECK(sdf.find("<max_linear_velocity>0.22</max_linear_velocity>") != std::string::npos);
  RC_CHECK(sdf.find("<min_angular_velocity>-2.84</min_angular_velocity>") != std::string::npos);
  RC_CHECK(sdf.find("<pose>0 0.08 0.033 -1.5708 0 0</pose>") != std::string::npos);
  RC_CHECK(sdf.find("<pose>0 -0.08 0.033 -1.5708 0 0</pose>") != std::string::npos);
  RC_CHECK(sdf.find("gz-sim-diff-drive-system") != std::string::npos);
  RC_CHECK(sdf.find("nan") == std::string::npos);

  // The masses of the links add up to the mass in the file.
  double sum = 0.0;
  std::size_t at = 0;
  while ((at = sdf.find("<mass>", at)) != std::string::npos) {
    sum += std::atof(sdf.c_str() + at + 6);
    at += 6;
  }
  rec("mass_sum", sum, 3);
  RC_CHECK_NEAR(sum, 1.0, 1e-6);
}
RC_TEST("the weight is put behind the axle, towards the caster, so that the robot rests on three points and does not rock") {
  const RobotParse p = parse_robot(kBurger);
  const std::string sdf = to_sdf(p.robot);
  // 40 percent of the way from the axle to the caster (0.036 m back): the chassis' own centre is a little further back than the whole robot's.
  RC_CHECK(sdf.find("<inertial><pose>-0.0146374 0 0 0 0 0</pose>") != std::string::npos);
  rec("com_chassis", 0.0146374 * 1000.0, 1);
  const RobotParse on_axle = parse_robot(std::string(kBurger) + "com_back 0 source test: the weight over the axle, to show what that does\n");
  RC_CHECK(on_axle.ok());
  rec("n_numbers_with_com", on_axle.numbers, 0);
  RC_CHECK(on_axle.numbers == 12);
  RC_CHECK(to_sdf(on_axle.robot).find("<inertial><pose>0.00118681 0 0 0 0 0</pose>") != std::string::npos);
  RC_CHECK(parse_robot(kBurger).numbers == 11);
}
