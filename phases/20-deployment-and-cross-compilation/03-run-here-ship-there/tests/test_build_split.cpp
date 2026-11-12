#include <rc/test/rc_test.hpp>

#include <iostream>

#include "solution.hpp"

namespace {

using rc::deploy::parse_triple;

std::string joined(const std::vector<Issue>& v) {
  std::string s;
  for (const Issue& i : v) s += "\n    " + i.step + ": " + i.message;
  return v.empty() ? " none" : s;
}

// The robot's build as it is first written: everything is built for the target, because that is what the toolchain says.
void first_attempt(std::vector<Step>& steps, std::vector<Edge>& edges, std::vector<std::string>& install) {
  steps = {{"msg_gen", Where::target, true},       // a code generator that the build runs
           {"libmsgs", Where::host, false},        // a library somebody built on the laptop and checked in
           {"robot_node", Where::target, false},
           {"unit_tests", Where::target, true},    // run by the build
           {"calibrate", Where::host, false}};     // a host tool
  edges = {{"robot_node", "libmsgs"}};
  install = {"robot_node", "calibrate"};
}

}  // namespace

RC_TEST("what runs where: the same architecture, a 32 bit x86 binary on a 64 bit x86 host, and nothing else") {
  const auto host = parse_triple("x86_64-linux-gnu");
  const struct { const char* triple; bool runs; } rows[] = {{"x86_64-linux-gnu", true}, {"i686-pc-linux-gnu", true}, {"aarch64-linux-gnu", false}, {"arm-linux-gnueabihf", false}, {"x86_64-w64-mingw32", false}};
  std::cout << "  on an x86_64-linux-gnu host:\n";
  for (const auto& r : rows) {
    std::cout << "  " << r.triple << std::string(24 - std::string(r.triple).size(), ' ') << (can_run(parse_triple(r.triple), host) ? "runs" : "does not run") << "\n";
    RC_CHECK(can_run(parse_triple(r.triple), host) == r.runs);
  }
  std::cout << "\n";
  RC_CHECK(!can_run(host, parse_triple("i686-pc-linux-gnu")));       // 64 bit on a 32 bit machine: no
  RC_CHECK(!can_run(parse_triple("nonsense"), host));
  RC_CHECK(!can_run(host, parse_triple("nonsense")));
}

RC_TEST("a native build hides every one of these mistakes, because the target is the host") {
  std::vector<Step> steps;
  std::vector<Edge> edges;
  std::vector<std::string> install;
  first_attempt(steps, edges, install);
  const auto host = parse_triple("x86_64-linux-gnu");
  const std::vector<Issue> native = check(steps, edges, install, host, host);
  std::cout << "  the first attempt, built for the host itself: " << native.size() << " problems\n";
  RC_CHECK(native.empty());
}

RC_TEST("the same graph built for the robot has four, each named") {
  std::vector<Step> steps;
  std::vector<Edge> edges;
  std::vector<std::string> install;
  first_attempt(steps, edges, install);
  const std::vector<Issue> cross = check(steps, edges, install, parse_triple("x86_64-linux-gnu"), parse_triple("aarch64-linux-gnu"));
  std::cout << "  the same graph, built on x86_64 for aarch64: " << cross.size() << " problems:" << joined(cross) << "\n\n";
  RC_CHECK(cross.size() == 4);
  if (cross.size() != 4) return;
  RC_CHECK(cross[0].step == "msg_gen" && cross[0].message == "built for the target but run during the build");
  RC_CHECK(cross[1].step == "unit_tests" && cross[1].message == "built for the target but run during the build");
  RC_CHECK(cross[2].step == "robot_node" && cross[2].message == "links libmsgs, built for the other machine");
  RC_CHECK(cross[3].step == "calibrate" && cross[3].message == "host binary in the bundle");
}

RC_TEST("with the generator built for the host, the library built for the target, the tests left to run on the robot or an emulator, and only target binaries in the bundle, there is nothing left") {
  std::vector<Step> steps = {{"msg_gen", Where::host, true}, {"libmsgs", Where::target, false}, {"robot_node", Where::target, false}, {"unit_tests", Where::target, false}, {"calibrate", Where::host, false}};
  std::vector<Edge> edges = {{"robot_node", "libmsgs"}, {"unit_tests", "libmsgs"}};
  const std::vector<std::string> install = {"robot_node"};
  const std::vector<Issue> cross = check(steps, edges, install, parse_triple("x86_64-linux-gnu"), parse_triple("aarch64-linux-gnu"));
  std::cout << "  corrected: " << cross.size() << " problems\n\n";
  RC_CHECK(cross.empty());
}

RC_TEST("a name that is not a step, and a triple that cannot be read, are problems and not crashes") {
  const std::vector<Step> steps = {{"robot_node", Where::target, false}};
  const std::vector<Issue> a = check(steps, {{"robot_node", "libghost"}, {"ghost", "robot_node"}}, {"missing"}, parse_triple("x86_64-linux-gnu"), parse_triple("aarch64-linux-gnu"));
  RC_CHECK(a.size() == 3);
  if (a.size() != 3) return;
  RC_CHECK(a[0].step == "libghost" && a[0].message == "not a step");
  RC_CHECK(a[1].step == "ghost" && a[1].message == "not a step");
  RC_CHECK(a[2].step == "missing" && a[2].message == "not a step");
  const std::vector<Issue> b = check(steps, {}, {}, parse_triple("nonsense"), parse_triple("aarch64-linux-gnu"));
  RC_CHECK(b.size() == 1 && b[0].step == "triple");
  RC_CHECK(check({}, {}, {}, parse_triple("x86_64-linux-gnu"), parse_triple("x86_64-linux-gnu")).empty());
  std::cout << "  an edge or an install naming nothing, and an unreadable triple, gave problems and not crashes\n\n";
}
