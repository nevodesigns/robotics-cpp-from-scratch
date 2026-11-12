#include <rc/test/rc_test.hpp>

#include <iostream>

#include "solution.hpp"
#include <iomanip>
#include <iostream>

static void rec(const char* key, double value, int digits = 3) {
  std::cout << "@" << key << " " << std::fixed << std::setprecision(digits) << value << std::endl;
  std::cout.unsetf(std::ios::floatfield);
}


namespace {

const char* kGood =
    "# a yard loop\n"
    "scenario yard_loop\n"
    "duration 60\n"
    "seed 7   # the noise\n"
    "ground gravel\n"
    "\n"
    "patch sand 5 0 10 10\n"
    "patch ice 7 0 8 10 0.1 0\n"
    "wall -10 -10 10 -10\n"
    "post 3 2 0.3\n"
    "start 0 0 90\n"
    "goal 8 5 0.3\n"
    "fault gnss_outage 20 30\n"
    "fault lidar_dropout 10 15 0.5\n";

const char* kBad =
    "scenario\n"
    "duration sixty\n"
    "ground mud\n"
    "patch sand 5 0 10\n"
    "wibble 1 2\n"
    "fault gnss_outage 30 20\n";

}  // namespace

RC_TEST("a good file is read into the scenario, comments and blank lines and all") {
  const ScenarioParse p = parse_scenario(kGood);
  RC_CHECK(p.ok());
  const FieldScenario& s = p.scenario;
  rec("n_patches", static_cast<double>(s.patches.size()), 0);
  rec("n_walls", static_cast<double>(s.walls.size()), 0);
  rec("n_faults", static_cast<double>(s.faults.size()), 0);
  rec("n_lines", 15.0, 0);
  RC_CHECK(s.name == "yard_loop");
  RC_CHECK_NEAR(s.duration, 60.0, 1e-12);
  RC_CHECK(s.seed == 7u);
  RC_CHECK(s.ground == "gravel");
  RC_CHECK(s.patches.size() == 2);
  RC_CHECK(s.walls.size() == 1);
  RC_CHECK(s.posts.size() == 1);
  RC_CHECK(s.has_start);
  RC_CHECK_NEAR(s.start_heading_deg, 90.0, 1e-12);
  RC_CHECK(s.goals.size() == 1);
  RC_CHECK(s.faults.size() == 2);
  RC_CHECK_NEAR(s.faults[1].param, 0.5, 1e-12);
}

RC_TEST("every mistake in a bad file is reported once, with the line it is on") {
  const ScenarioParse p = parse_scenario(kBad);
  rec("n_bad", 6.0, 0);
  rec("n_found", static_cast<double>(p.problems.size()), 0);
  for (std::size_t i = 0; i < p.problems.size(); ++i) std::cout << "  line " << p.problems[i].line << ": " << p.problems[i].message << "\n";
  RC_CHECK(!p.ok());
  RC_CHECK(p.problems.size() == 8);
  const int lines[] = {1, 2, 3, 4, 5, 6, 0, 0};
  for (std::size_t i = 0; i < p.problems.size() && i < 8; ++i) RC_CHECK(p.problems[i].line == lines[i]);
  RC_CHECK(p.problems[2].message.find("mud") != std::string::npos);
  RC_CHECK(p.problems[4].message.find("wibble") != std::string::npos);
}

RC_TEST("writing a scenario and reading it back repeats it exactly") {
  const ScenarioParse first = parse_scenario(kGood);
  const std::string once = write_scenario(first.scenario);
  const ScenarioParse second = parse_scenario(once);
  RC_CHECK(second.ok());
  const std::string twice = write_scenario(second.scenario);
  RC_CHECK(once == twice);
  RC_CHECK(once.find("seed 7") != std::string::npos);
}

RC_TEST("the terrain, the lidar world and the faults come out of the scenario") {
  const ScenarioParse p = parse_scenario(kGood);
  const rc::field::Terrain t = terrain_of(p.scenario);
  RC_CHECK(std::string(t.at(1.0, 1.0).surface.name) == "gravel");
  RC_CHECK(std::string(t.at(6.0, 1.0).surface.name) == "sand");
  RC_CHECK(std::string(t.at(7.5, 1.0).surface.name) == "ice");
  RC_CHECK_NEAR(t.at(7.5, 1.0).grade_x, 0.1, 1e-12);
  const rc::field::World2 w = world_of(p.scenario);
  RC_CHECK(w.walls.size() == 1 && w.posts.size() == 1);
  double drop = 0.0;
  RC_CHECK(fault_active(p.scenario, "lidar_dropout", 12.0, &drop));
  RC_CHECK_NEAR(drop, 0.5, 1e-12);
  RC_CHECK(!fault_active(p.scenario, "lidar_dropout", 15.0, &drop));
  RC_CHECK(fault_active(p.scenario, "gnss_outage", 20.0, nullptr));
  RC_CHECK(!fault_active(p.scenario, "gnss_outage", 19.99, nullptr));
}

RC_TEST("a file with no start and no duration says so, and a fault past the end is a problem") {
  const ScenarioParse p = parse_scenario("scenario x\nfault gnss_outage 1 5\n");
  RC_CHECK(p.problems.size() == 2);
  const ScenarioParse q = parse_scenario("scenario x\nduration 10\nstart 0 0 0\nfault gnss_outage 5 50\n");
  RC_CHECK(q.problems.size() == 1);
  RC_CHECK(q.problems[0].line == 4);
}
