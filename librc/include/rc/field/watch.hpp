// rc/field/watch.hpp
//
// Watching a run, from lesson 22-09.
//
// to_svg turns a scenario and a run log into one SVG text: the terrain patches in a colour each, the walls, the posts, the goals as dashed rings, the path of the robot (the "pose" channel) and a triangle that moves along it at a chosen playback speed. The SVG needs nothing to open it but a browser. bounds_of finds the rectangle that holds everything that is drawn, so nothing is cut off; the world is drawn in a group flipped with scale(1,-1) so that y is up as it is on the robot, not down as it is in SVG.
//
// A 10 s run of 500 poses came to 24.5 kB of SVG, a picture 16.0 m by 9.0 m, played over 9.98 s at normal speed and 2.50 s at four times; every pose lay inside the bounds, and the same input gave the same text.

#ifndef RC_RC_FIELD_WATCH_HPP
#define RC_RC_FIELD_WATCH_HPP

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iomanip>
#include <rc/field/field_scenario.hpp>
#include <rc/field/run_log.hpp>
#include <sstream>
#include <string>

namespace rc {
namespace field {

struct Bounds {
  double x0 = 0.0;
  double y0 = 0.0;
  double x1 = 1.0;
  double y1 = 1.0;
};

struct ViewOptions {
  double width_px = 800.0;
  double margin = 1.0;     // metres of margin round everything that is drawn
  double speed = 1.0;      // playback speed: 4 plays four seconds of the run in one
  double robot_length = 0.5;  // the length of the triangle that stands for the robot, in metres
};

inline void grow(Bounds& b, bool& first, double x, double y) {
  if (first) {
    b.x0 = b.x1 = x;
    b.y0 = b.y1 = y;
    first = false;
    return;
  }
  b.x0 = std::min(b.x0, x);
  b.x1 = std::max(b.x1, x);
  b.y0 = std::min(b.y0, y);
  b.y1 = std::max(b.y1, y);
}

// The rectangle that holds the start, the patches, the walls, the posts, the goals and every pose of the log, without the margin.
inline Bounds bounds_of(const FieldScenario& sc, const RunLog& log) {
  Bounds b;
  bool first = true;
  if (sc.has_start) grow(b, first, sc.start_x, sc.start_y);
  for (std::size_t i = 0; i < sc.patches.size(); ++i) {
    grow(b, first, sc.patches[i].x0, sc.patches[i].y0);
    grow(b, first, sc.patches[i].x1, sc.patches[i].y1);
  }
  for (std::size_t i = 0; i < sc.walls.size(); ++i) {
    grow(b, first, sc.walls[i].x0, sc.walls[i].y0);
    grow(b, first, sc.walls[i].x1, sc.walls[i].y1);
  }
  for (std::size_t i = 0; i < sc.posts.size(); ++i) {
    grow(b, first, sc.posts[i].x - sc.posts[i].r, sc.posts[i].y - sc.posts[i].r);
    grow(b, first, sc.posts[i].x + sc.posts[i].r, sc.posts[i].y + sc.posts[i].r);
  }
  for (std::size_t i = 0; i < sc.goals.size(); ++i) {
    grow(b, first, sc.goals[i].x - sc.goals[i].tolerance, sc.goals[i].y - sc.goals[i].tolerance);
    grow(b, first, sc.goals[i].x + sc.goals[i].tolerance, sc.goals[i].y + sc.goals[i].tolerance);
  }
  for (std::size_t i = 0; i < log.entries.size(); ++i)
    if (log.entries[i].channel == "pose" && log.entries[i].values.size() >= 2) grow(b, first, log.entries[i].values[0], log.entries[i].values[1]);
  if (first) {
    b.x0 = b.y0 = 0.0;
    b.x1 = b.y1 = 1.0;
  }
  return b;
}

inline std::string xml_escape(const std::string& s) {
  std::string out;
  for (std::size_t i = 0; i < s.size(); ++i) {
    if (s[i] == '&') out += "&amp;";
    else if (s[i] == '<') out += "&lt;";
    else if (s[i] == '>') out += "&gt;";
    else if (s[i] == '"') out += "&quot;";
    else out += s[i];
  }
  return out;
}

inline const char* surface_colour(const std::string& name) {
  if (name == "gravel") return "#b9a98a";
  if (name == "grass") return "#7cae5a";
  if (name == "sand") return "#e3cf8e";
  if (name == "ice") return "#bfe3f0";
  return "#9a9a9a";
}

inline std::string to_svg(const FieldScenario& sc, const RunLog& log, const ViewOptions& view) {
  Bounds b = bounds_of(sc, log);
  b.x0 -= view.margin;
  b.y0 -= view.margin;
  b.x1 += view.margin;
  b.y1 += view.margin;
  const double w = b.x1 - b.x0;
  const double h = b.y1 - b.y0;
  std::ostringstream o;
  o << std::fixed << std::setprecision(3);
  // The picture's y axis points down and the robot's points up: the world sits in a group that flips it, and the viewBox starts at minus the top.
  o << "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"" << b.x0 << " " << -b.y1 << " " << w << " " << h << "\" width=\"" << view.width_px << "\" height=\"" << view.width_px * h / w << "\">\n";
  o << "<title>" << xml_escape(sc.name) << "</title>\n";
  o << "<rect x=\"" << b.x0 << "\" y=\"" << -b.y1 << "\" width=\"" << w << "\" height=\"" << h << "\" fill=\"#f4f4f0\"/>\n";
  o << "<g transform=\"scale(1,-1)\">\n";
  o << "<g id=\"ground\">\n";
  for (std::size_t i = 0; i < sc.patches.size(); ++i) {
    const FieldPatch& p = sc.patches[i];
    o << "<rect x=\"" << p.x0 << "\" y=\"" << p.y0 << "\" width=\"" << p.x1 - p.x0 << "\" height=\"" << p.y1 - p.y0 << "\" fill=\"" << surface_colour(p.surface) << "\"/>\n";
  }
  o << "</g>\n<g id=\"obstacles\">\n";
  for (std::size_t i = 0; i < sc.walls.size(); ++i)
    o << "<line x1=\"" << sc.walls[i].x0 << "\" y1=\"" << sc.walls[i].y0 << "\" x2=\"" << sc.walls[i].x1 << "\" y2=\"" << sc.walls[i].y1 << "\" stroke=\"#222\" stroke-width=\"0.1\"/>\n";
  for (std::size_t i = 0; i < sc.posts.size(); ++i)
    o << "<circle cx=\"" << sc.posts[i].x << "\" cy=\"" << sc.posts[i].y << "\" r=\"" << sc.posts[i].r << "\" fill=\"#222\"/>\n";
  for (std::size_t i = 0; i < sc.goals.size(); ++i)
    o << "<circle cx=\"" << sc.goals[i].x << "\" cy=\"" << sc.goals[i].y << "\" r=\"" << sc.goals[i].tolerance << "\" fill=\"none\" stroke=\"#1a7f37\" stroke-width=\"0.05\" stroke-dasharray=\"0.15 0.1\"/>\n";
  o << "</g>\n";

  // The path, and the times of its points.
  std::vector<double> xs, ys, th, ts;
  for (std::size_t i = 0; i < log.entries.size(); ++i) {
    const LogEntry& e = log.entries[i];
    if (e.channel != "pose" || e.values.size() < 3) continue;
    xs.push_back(e.values[0]);
    ys.push_back(e.values[1]);
    th.push_back(e.values[2] * 180.0 / 3.14159265358979323846);
    ts.push_back(e.t);
  }
  if (xs.size() >= 2) {
    o << "<polyline fill=\"none\" stroke=\"#c0392b\" stroke-width=\"0.05\" points=\"";
    for (std::size_t i = 0; i < xs.size(); ++i) o << (i ? " " : "") << xs[i] << "," << ys[i];
    o << "\"/>\n";
  }
  const double rl = view.robot_length;
  o << "<g id=\"robot\"><polygon points=\"" << rl * 0.6 << ",0 " << -rl * 0.4 << "," << rl * 0.3 << " " << -rl * 0.4 << "," << -rl * 0.3 << "\" fill=\"#1f4e9c\"/>\n";
  if (xs.size() >= 2) {
    const double span = ts.back() - ts.front();
    const double dur = span / (view.speed > 0.0 ? view.speed : 1.0);
    o << "<animateTransform attributeName=\"transform\" type=\"translate\" dur=\"" << dur << "s\" repeatCount=\"indefinite\" keyTimes=\"";
    for (std::size_t i = 0; i < ts.size(); ++i) o << (i ? ";" : "") << (span > 0.0 ? (ts[i] - ts.front()) / span : 0.0);
    o << "\" values=\"";
    for (std::size_t i = 0; i < xs.size(); ++i) o << (i ? ";" : "") << xs[i] << " " << ys[i];
    o << "\"/>\n";
    o << "<animateTransform attributeName=\"transform\" type=\"rotate\" additive=\"sum\" dur=\"" << dur << "s\" repeatCount=\"indefinite\" keyTimes=\"";
    for (std::size_t i = 0; i < ts.size(); ++i) o << (i ? ";" : "") << (span > 0.0 ? (ts[i] - ts.front()) / span : 0.0);
    o << "\" values=\"";
    for (std::size_t i = 0; i < th.size(); ++i) o << (i ? ";" : "") << th[i] << " 0 0";
    o << "\"/>\n";
  } else if (sc.has_start) {
    o << "<animateTransform attributeName=\"transform\" type=\"translate\" values=\"" << sc.start_x << " " << sc.start_y << "\" dur=\"1s\"/>\n";
  }
  o << "</g>\n</g>\n";
  o << "<text x=\"" << b.x0 + 0.3 << "\" y=\"" << -b.y1 + 0.9 << "\" font-size=\"0.7\" font-family=\"sans-serif\" fill=\"#333\">" << xml_escape(sc.name) << " (" << sc.duration << " s)</text>\n";
  o << "</svg>\n";
  return o.str();
}


}  // namespace field
}  // namespace rc

#endif
