#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>
#include <cstddef>
#include <iomanip>
#include <rc/field/field_scenario.hpp>
#include <rc/field/robot_file.hpp>
#include <rc/field/terrain.hpp>
#include <sstream>
#include <string>

using namespace rc::field;

// A box in the world: where its centre is, its sides, and how it is turned.
struct WorldBox {
  double cx = 0.0;
  double cy = 0.0;
  double cz = 0.0;
  double sx = 0.0;
  double sy = 0.0;
  double sz = 0.0;
  double roll = 0.0;
  double pitch = 0.0;
  double yaw = 0.0;
};

// A wall from one point to another as a box of a thickness and a height standing on the ground, with its long side along the wall.
inline WorldBox wall_box(const rc::field::Segment& w, double thickness, double height) {
  WorldBox b;
  b.cx = 0.5 * (w.x0 + w.x1);
  b.cy = 0.5 * (w.y0 + w.y1);
  b.cz = 0.5 * height;
  b.sx = std::hypot(w.x1 - w.x0, w.y1 - w.y0);
  b.sy = thickness;
  b.sz = height;
  b.yaw = std::atan2(w.y1 - w.y0, w.x1 - w.x0);
  return b;
}

// The roll and pitch that give a slab the grades of a patch. A grade of g rising along +x is a pitch of minus atan(g); a grade rising along +y is a roll of atan(g).
inline void patch_tilt(const FieldPatch& p, double& roll, double& pitch) {
  roll = 0.0;
  pitch = 0.0;
  pitch = -std::atan(p.grade_x);
  roll = std::atan(p.grade_y);
}

inline void put_pose(std::ostringstream& o, double x, double y, double z, double roll, double pitch, double yaw) {
  o << "<pose>" << x << " " << y << " " << z << " " << roll << " " << pitch << " " << yaw << "</pose>";
}

inline void put_friction(std::ostringstream& o, double mu) {
  o << "<surface><friction><ode><mu>" << mu << "</mu><mu2>" << mu << "</mu2></ode></friction></surface>";
}

// A fixed box with a collision, a visual and the friction of a surface.
inline void put_static_box(std::ostringstream& o, const std::string& name, const WorldBox& b, double mu, const char* colour) {
  o << "<model name=\"" << name << "\"><static>true</static>";
  put_pose(o, b.cx, b.cy, b.cz, b.roll, b.pitch, b.yaw);
  o << "<link name=\"link\"><collision name=\"collision\"><geometry><box><size>" << b.sx << " " << b.sy << " " << b.sz << "</size></box></geometry>";
  put_friction(o, mu);
  o << "</collision><visual name=\"visual\"><geometry><box><size>" << b.sx << " " << b.sy << " " << b.sz << "</size></box></geometry>"
    << "<material><ambient>" << colour << "</ambient><diffuse>" << colour << "</diffuse></material></visual></link></model>\n";
}

inline const char* world_colour(const std::string& surface) {
  if (surface == "gravel") return "0.73 0.66 0.54 1";
  if (surface == "grass") return "0.49 0.68 0.35 1";
  if (surface == "sand") return "0.89 0.81 0.56 1";
  if (surface == "ice") return "0.75 0.89 0.94 1";
  return "0.6 0.6 0.6 1";
}

inline std::string to_world(const FieldScenario& sc, const RobotModel& robot) {
  const double slab = 0.2;
  const double pi = 3.14159265358979323846;
  std::ostringstream o;
  o << std::setprecision(6);
  o << "<?xml version=\"1.0\"?>\n<sdf version=\"1.9\">\n<world name=\"" << (sc.name.empty() ? "field" : sc.name) << "\">\n";
  o << "<physics name=\"fixed\" type=\"dart\"><max_step_size>0.001</max_step_size><real_time_factor>1.0</real_time_factor></physics>\n";
  o << "<plugin filename=\"gz-sim-physics-system\" name=\"gz::sim::systems::Physics\"/>\n";
  o << "<plugin filename=\"gz-sim-user-commands-system\" name=\"gz::sim::systems::UserCommands\"/>\n";
  o << "<plugin filename=\"gz-sim-scene-broadcaster-system\" name=\"gz::sim::systems::SceneBroadcaster\"/>\n";
  o << "<light type=\"directional\" name=\"sun\"><cast_shadows>true</cast_shadows><pose>0 0 10 0 0 0</pose><diffuse>0.9 0.9 0.9 1</diffuse><direction>-0.5 0.1 -0.9</direction></light>\n";

  rc::field::Surface ground = rc::field::tarmac();
  surface_by_name(sc.ground, ground);
  WorldBox base;
  base.cz = -0.5 * slab;
  base.sx = 200.0;
  base.sy = 200.0;
  base.sz = slab;
  put_static_box(o, "ground", base, ground.mu, world_colour(sc.ground));

  for (std::size_t i = 0; i < sc.patches.size(); ++i) {
    const FieldPatch& p = sc.patches[i];
    rc::field::Surface s = rc::field::tarmac();
    surface_by_name(p.surface, s);
    WorldBox b;
    b.cx = 0.5 * (p.x0 + p.x1);
    b.cy = 0.5 * (p.y0 + p.y1);
    b.cz = 0.001 * static_cast<double>(i + 1) - 0.5 * slab;
    b.sx = p.x1 - p.x0;
    b.sy = p.y1 - p.y0;
    b.sz = slab;
    patch_tilt(p, b.roll, b.pitch);
    put_static_box(o, "patch_" + std::to_string(i), b, s.mu, world_colour(p.surface));
  }
  for (std::size_t i = 0; i < sc.walls.size(); ++i) put_static_box(o, "wall_" + std::to_string(i), wall_box(sc.walls[i], 0.1, 1.0), 0.5, "0.13 0.13 0.13 1");
  for (std::size_t i = 0; i < sc.posts.size(); ++i) {
    const rc::field::Circle& c = sc.posts[i];
    o << "<model name=\"post_" << i << "\"><static>true</static>";
    put_pose(o, c.x, c.y, 0.5, 0, 0, 0);
    o << "<link name=\"link\"><collision name=\"collision\"><geometry><cylinder><radius>" << c.r << "</radius><length>1</length></cylinder></geometry></collision>"
      << "<visual name=\"visual\"><geometry><cylinder><radius>" << c.r << "</radius><length>1</length></cylinder></geometry></visual></link></model>\n";
  }
  for (std::size_t i = 0; i < sc.goals.size(); ++i) {
    const FieldGoal& g = sc.goals[i];
    o << "<model name=\"goal_" << i << "\"><static>true</static>";
    put_pose(o, g.x, g.y, 0.005, 0, 0, 0);
    o << "<link name=\"link\"><visual name=\"visual\"><geometry><cylinder><radius>" << g.tolerance << "</radius><length>0.01</length></cylinder></geometry>"
      << "<material><ambient>0.1 0.6 0.2 0.5</ambient><diffuse>0.1 0.6 0.2 0.5</diffuse></material></visual></link></model>\n";
  }

  std::ostringstream pose;
  pose << std::setprecision(6) << sc.start_x << " " << sc.start_y << " 0 0 0 " << sc.start_heading_deg * pi / 180.0;
  o << sdf_model(robot, pose.str());
  o << "</world>\n</sdf>\n";
  return o.str();
}


#endif
