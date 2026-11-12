#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iostream>
#include <vector>

#include <rc/hal/interfaces.hpp>

#include "solution.hpp"
#include <iomanip>
#include <iostream>

static void rec(const char* key, double value, int digits = 3) {
  std::cout << "@" << key << " " << std::fixed << std::setprecision(digits) << value << std::endl;
  std::cout.unsetf(std::ios::floatfield);
}


namespace {

const char* kYard =
    "scenario yard<loop>\n"
    "duration 10\n"
    "ground gravel\n"
    "patch sand 5 -3 10 3\n"
    "wall -2 -4 12 -4\n"
    "post 3 2 0.3\n"
    "start 0 0 0\n"
    "goal 8 1 0.5\n";

RunLog drive() {
  RunLog log;
  rc::hal::FakeDrive base;
  for (int i = 1; i <= 500; ++i) {
    rc::hal::DriveCommand c;
    c.v = 1.0;
    c.w = (i / 125) % 2 == 0 ? 0.2 : -0.2;
    base.command(c);
    base.step(0.02);
    const rc::hal::DriveState s = base.state();
    log.add(0.02 * i, "pose", {s.x, s.y, s.theta});
  }
  return log;
}

// A tag balance check: every tag opened is closed, in order. Enough to catch an SVG that a browser will refuse.
bool balanced(const std::string& svg) {
  std::vector<std::string> stack;
  std::size_t i = 0;
  while ((i = svg.find('<', i)) != std::string::npos) {
    const std::size_t end = svg.find('>', i);
    if (end == std::string::npos) return false;
    std::string tag = svg.substr(i + 1, end - i - 1);
    i = end + 1;
    if (tag.empty() || tag[0] == '?' || tag[0] == '!') continue;
    if (tag.back() == '/') continue;
    if (tag[0] == '/') {
      const std::string name = tag.substr(1);
      if (stack.empty() || stack.back() != name) return false;
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

RC_TEST("the bounds hold everything that is drawn") {
  const ScenarioParse p = parse_scenario(kYard);
  RC_CHECK(p.ok());
  const RunLog log = drive();
  const Bounds b = bounds_of(p.scenario, log);
  RC_CHECK_NEAR(b.x0, -2.0, 1e-9);
  RC_CHECK_NEAR(b.x1, 12.0, 1e-9);
  RC_CHECK_NEAR(b.y0, -4.0, 1e-9);
  RC_CHECK(b.y1 >= 3.0);
  for (std::size_t i = 0; i < log.entries.size(); ++i) {
    RC_CHECK(log.entries[i].values[0] >= b.x0 && log.entries[i].values[0] <= b.x1);
    RC_CHECK(log.entries[i].values[1] >= b.y0 && log.entries[i].values[1] <= b.y1);
  }
  ScenarioParse empty = parse_scenario("scenario e\nduration 1\n");
  const Bounds none = bounds_of(empty.scenario, RunLog());
  RC_CHECK(none.x1 > none.x0 && none.y1 > none.y0);
}

RC_TEST("the picture is well formed, has one element for each thing in the scenario, and one point for each pose") {
  const ScenarioParse p = parse_scenario(kYard);
  const RunLog log = drive();
  const std::string svg = to_svg(p.scenario, log, ViewOptions());
  RC_CHECK(balanced(svg));
  RC_CHECK(count_of(svg, "<line ") == 1);
  RC_CHECK(count_of(svg, "<circle ") == 2);
  RC_CHECK(count_of(svg, "<rect ") == 2);
  RC_CHECK(count_of(svg, "<polyline ") == 1);
  RC_CHECK(count_of(svg, "<animateTransform ") == 2);
  RC_CHECK(svg.find("yard&lt;loop&gt;") != std::string::npos);
  RC_CHECK(svg.find("nan") == std::string::npos && svg.find("inf") == std::string::npos);
  rec("n_points", static_cast<double>(log.entries.size()), 0);
  rec("svg_kb", static_cast<double>(svg.size()) / 1000.0, 1);
  // 500 points in the path: 499 spaces between them
  const std::size_t from = svg.find("points=\"", svg.find("<polyline"));
  const std::size_t to = svg.find('"', from + 8);
  RC_CHECK(count_of(svg.substr(from, to - from), " ") == 499);
}

RC_TEST("the world is flipped so that y is up, the view starts at minus the top, and the playback follows the speed") {
  const ScenarioParse p = parse_scenario(kYard);
  const RunLog log = drive();
  ViewOptions view;
  const std::string svg = to_svg(p.scenario, log, view);
  RC_CHECK(svg.find("scale(1,-1)") != std::string::npos);
  const Bounds b = bounds_of(p.scenario, log);
  std::ostringstream expect;
  expect << std::fixed << std::setprecision(3) << "viewBox=\"" << b.x0 - view.margin << " " << -(b.y1 + view.margin);
  RC_CHECK(svg.find(expect.str()) != std::string::npos);
  rec("vb_w", b.x1 - b.x0 + 2.0, 1);
  rec("vb_h", b.y1 - b.y0 + 2.0, 1);

  const double span = log.entries.back().t - log.entries.front().t;
  rec("dur", span, 2);
  RC_CHECK(svg.find("dur=\"9.980s\"") != std::string::npos);
  view.speed = 4.0;
  const std::string fast = to_svg(p.scenario, log, view);
  rec("dur_fast", span / 4.0, 2);
  RC_CHECK(fast.find("dur=\"2.495s\"") != std::string::npos);
  RC_CHECK(to_svg(p.scenario, log, view) == fast);
}

RC_TEST("a run with no poses still draws the world and the robot at its start") {
  const ScenarioParse p = parse_scenario(kYard);
  const std::string svg = to_svg(p.scenario, RunLog(), ViewOptions());
  RC_CHECK(balanced(svg));
  RC_CHECK(count_of(svg, "<polyline ") == 0);
  RC_CHECK(count_of(svg, "<animateTransform ") == 1);
}
