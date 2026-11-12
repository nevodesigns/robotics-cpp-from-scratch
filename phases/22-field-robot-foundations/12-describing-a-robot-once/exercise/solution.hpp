#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <cstddef>
#include <iomanip>
#include <rc/field/field_scenario.hpp>
#include <rc/field/limits.hpp>
#include <rc/hal/interfaces.hpp>
#include <sstream>
#include <string>
#include <vector>

using namespace rc::field;

// A number and where it came from. "typical" means: I did not find it, this is a value of the class.
struct Sourced {
  double value = 0.0;
  std::string source;
  bool set = false;
  bool is_typical() const { return source == "typical"; }
};

struct RobotModel {
  std::string name;
  std::string kind;
  Sourced wheel_radius;
  Sourced track_width;
  Sourced length;
  Sourced width;
  Sourced height;
  Sourced mass;
  Sourced wheel_mass;
  Sourced clearance;
  Sourced wheel_width;
  Sourced max_speed;
  Sourced max_turn_rate;
};

struct RobotParse {
  RobotModel robot;
  std::vector<ScenarioProblem> problems;
  bool ok() const { return problems.empty(); }
  int numbers = 0;
  int typical = 0;
};

inline Sourced* robot_slot(RobotModel& r, const std::string& key) {
  if (key == "wheel_radius") return &r.wheel_radius;
  if (key == "track_width") return &r.track_width;
  if (key == "length") return &r.length;
  if (key == "width") return &r.width;
  if (key == "height") return &r.height;
  if (key == "mass") return &r.mass;
  if (key == "wheel_mass") return &r.wheel_mass;
  if (key == "clearance") return &r.clearance;
  if (key == "wheel_width") return &r.wheel_width;
  if (key == "max_speed") return &r.max_speed;
  if (key == "max_turn_rate") return &r.max_turn_rate;
  return nullptr;
}

inline const char* const* robot_keys() {
  static const char* const keys[] = {"wheel_radius", "track_width", "length", "width", "height", "mass", "wheel_mass", "clearance", "wheel_width", "max_speed", "max_turn_rate", nullptr};
  return keys;
}

inline RobotParse parse_robot(const std::string& text) {
  RobotParse out;
  std::istringstream in(text);
  std::string line;
  int number = 0;
  auto bad = [&](int at, const std::string& message) {
    ScenarioProblem p;
    p.line = at;
    p.message = message;
    out.problems.push_back(p);
  };
  while (std::getline(in, line)) {
    ++number;
    const std::size_t hash = line.find('#');
    if (hash != std::string::npos) line.erase(hash);
    const std::vector<std::string> w = split_words(line);
    if (w.empty()) continue;
    if (w[0] == "robot") {
      if (w.size() == 2) out.robot.name = w[1];
      else bad(number, "'robot' needs a name");
    } else if (w[0] == "kind") {
      if (w.size() != 2) bad(number, "'kind' needs one word");
      else if (w[1] != "diff_drive") bad(number, "unsupported kind '" + w[1] + "'");
      else out.robot.kind = w[1];
    } else {
      Sourced* slot = robot_slot(out.robot, w[0]);
      if (slot == nullptr) {
        bad(number, "unknown key '" + w[0] + "'");
        continue;
      }
      (void)slot;  // TODO: refuse a key given twice, a missing or bad number, and a line without the word source and some text after it; otherwise store the value and its source (the words after 'source', joined by spaces) and count it
    }
  }
  if (out.robot.name.empty()) bad(0, "no robot name");
  if (out.robot.kind.empty()) bad(0, "no kind");
  for (const char* const* k = robot_keys(); *k != nullptr; ++k)
    if (!robot_slot(out.robot, *k)->set) bad(0, std::string("'") + *k + "' is missing");
  return out;
}

// The built in model's limits from the file: the same numbers the SDF is given.
inline VehicleLimits limits_of(const RobotModel& r) {
  VehicleLimits l;
  l.max_v = Speed::from_mps(r.max_speed.value);
  l.max_w = Rate::from_rps(r.max_turn_rate.value);
  return l;
}

inline rc::hal::DriveLimits drive_limits_of(const RobotModel& r) {
  rc::hal::DriveLimits l;
  l.max_v = r.max_speed.value;
  l.max_w = r.max_turn_rate.value;
  return l;
}

struct Inertia {
  double ixx = 0.0;
  double iyy = 0.0;
  double izz = 0.0;
};

// A solid box of a mass, with sides along x, y and z.
inline Inertia box_inertia(double m, double sx, double sy, double sz) {
  Inertia i;
  (void)m;  // TODO: about x it is m / 12 times (sy squared plus sz squared), and likewise for the others
  (void)sx;
  (void)sy;
  (void)sz;
  return i;
}

// A solid cylinder of a mass, radius and length, with its axis along z.
inline Inertia cylinder_inertia(double m, double radius, double length) {
  Inertia i;
  (void)m;  // TODO: about the axis it is half m r squared; about the other two m / 12 times (3 r squared plus length squared)
  (void)radius;
  (void)length;
  return i;
}

inline double sphere_inertia(double m, double radius) { return 0.4 * m * radius * radius; }

// The robot as an SDF model for Gazebo Sim: a body, two wheels on revolute joints, a ball caster at the back, and the DiffDrive system with the file's numbers.
inline std::string sdf_model(const RobotModel& r, const std::string& pose) {
  const double wheel_r = r.wheel_radius.value;
  const double body_h = r.height.value - r.clearance.value;
  const double body_z = r.clearance.value + 0.5 * body_h;
  const double body_m = r.mass.value - 3.0 * r.wheel_mass.value;
  const Inertia bi = box_inertia(body_m, r.length.value, r.width.value, body_h);
  const Inertia wi = cylinder_inertia(r.wheel_mass.value, wheel_r, r.wheel_width.value);
  const double ci = sphere_inertia(r.wheel_mass.value, wheel_r);
  const double half_track = 0.5 * r.track_width.value;
  const double caster_x = -0.5 * r.length.value + wheel_r;
  const double pi_half = 1.5707963267948966;
  std::ostringstream o;
  o << std::setprecision(6);
  o << "<model name=\"" << r.name << "\">\n";
  if (!pose.empty()) o << "<pose>" << pose << "</pose>\n";
  o << "<link name=\"chassis\">\n<pose>0 0 " << body_z << " 0 0 0</pose>\n";
  o << "<inertial><mass>" << body_m << "</mass><inertia><ixx>" << bi.ixx << "</ixx><iyy>" << bi.iyy << "</iyy><izz>" << bi.izz << "</izz></inertia></inertial>\n";
  o << "<collision name=\"collision\"><geometry><box><size>" << r.length.value << " " << r.width.value << " " << body_h << "</size></box></geometry></collision>\n";
  o << "<visual name=\"visual\"><geometry><box><size>" << r.length.value << " " << r.width.value << " " << body_h << "</size></box></geometry></visual>\n</link>\n";
  const char* const names[2] = {"left", "right"};
  const double sides[2] = {half_track, -half_track};
  for (int k = 0; k < 2; ++k) {
    o << "<link name=\"" << names[k] << "_wheel\">\n<pose>0 " << sides[k] << " " << wheel_r << " " << -pi_half << " 0 0</pose>\n";
    o << "<inertial><mass>" << r.wheel_mass.value << "</mass><inertia><ixx>" << wi.ixx << "</ixx><iyy>" << wi.iyy << "</iyy><izz>" << wi.izz << "</izz></inertia></inertial>\n";
    o << "<collision name=\"collision\"><geometry><cylinder><radius>" << wheel_r << "</radius><length>" << r.wheel_width.value << "</length></cylinder></geometry>"
      << "<surface><friction><ode><mu>1.0</mu><mu2>1.0</mu2></ode></friction></surface></collision>\n";
    o << "<visual name=\"visual\"><geometry><cylinder><radius>" << wheel_r << "</radius><length>" << r.wheel_width.value << "</length></cylinder></geometry></visual>\n</link>\n";
  }
  o << "<link name=\"caster\">\n<pose>" << caster_x << " 0 " << wheel_r << " 0 0 0</pose>\n";
  o << "<inertial><mass>" << r.wheel_mass.value << "</mass><inertia><ixx>" << ci << "</ixx><iyy>" << ci << "</iyy><izz>" << ci << "</izz></inertia></inertial>\n";
  o << "<collision name=\"collision\"><geometry><sphere><radius>" << wheel_r << "</radius></sphere></geometry>"
    << "<surface><friction><ode><mu>0</mu><mu2>0</mu2></ode></friction></surface></collision>\n";
  o << "<visual name=\"visual\"><geometry><sphere><radius>" << wheel_r << "</radius></sphere></geometry></visual>\n</link>\n";
  for (int k = 0; k < 2; ++k) {
    o << "<joint name=\"" << names[k] << "_wheel_joint\" type=\"revolute\"><parent>chassis</parent><child>" << names[k] << "_wheel</child>"
      << "<axis><xyz expressed_in=\"__model__\">0 1 0</xyz></axis></joint>\n";
  }
  o << "<joint name=\"caster_joint\" type=\"ball\"><parent>chassis</parent><child>caster</child></joint>\n";
  o << "<plugin filename=\"gz-sim-diff-drive-system\" name=\"gz::sim::systems::DiffDrive\">\n"
    << "<left_joint>left_wheel_joint</left_joint><right_joint>right_wheel_joint</right_joint>\n"
    << "<wheel_separation>" << r.track_width.value << "</wheel_separation><wheel_radius>" << wheel_r << "</wheel_radius>\n"
    << "<max_linear_velocity>" << r.max_speed.value << "</max_linear_velocity><min_linear_velocity>" << -r.max_speed.value << "</min_linear_velocity>\n"
    << "<max_angular_velocity>" << r.max_turn_rate.value << "</max_angular_velocity><min_angular_velocity>" << -r.max_turn_rate.value << "</min_angular_velocity>\n"
    << "<odom_publish_frequency>50</odom_publish_frequency><topic>/model/" << r.name << "/cmd_vel</topic><odom_topic>/model/" << r.name << "/odometry</odom_topic>\n"
    << "</plugin>\n";
  o << "</model>\n";
  return o.str();
}

// The robot as a file of its own: the model inside an sdf element.
inline std::string to_sdf(const RobotModel& r) { return "<?xml version=\"1.0\"?>\n<sdf version=\"1.9\">\n" + sdf_model(r, "") + "</sdf>\n"; }


#endif
