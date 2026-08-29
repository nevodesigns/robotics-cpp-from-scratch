#include <rc/test/rc_test.hpp>

#include <algorithm>
#include <string>
#include <vector>

#include "solution.hpp"

namespace {

Target target(const std::string& name, std::vector<std::string> depends,
              std::vector<std::string> inputs, const std::string& output) {
  Target made;
  made.name = name;
  made.depends_on = std::move(depends);
  made.inputs = std::move(inputs);
  made.output = output;
  return made;
}

// A small but realistic project: a header shared by two objects, a library made
// from them, and a program linking the library.
BuildGraph small_project() {
  BuildGraph graph;
  graph.add(target("pid.o", {}, {"pid.cpp", "rc.hpp"}, "pid.o"));
  graph.add(target("safety.o", {}, {"safety.cpp", "rc.hpp"}, "safety.o"));
  graph.add(target("librc", {"pid.o", "safety.o"}, {}, "librc.a"));
  graph.add(target("robot", {"librc"}, {"main.cpp"}, "robot"));
  return graph;
}

bool comes_before(const std::vector<std::string>& order, const std::string& first,
                  const std::string& second) {
  const auto at_first = std::find(order.begin(), order.end(), first);
  const auto at_second = std::find(order.begin(), order.end(), second);
  return at_first != order.end() && at_second != order.end() && at_first < at_second;
}

bool contains_name(const std::vector<std::string>& names, const std::string& name) {
  return std::find(names.begin(), names.end(), name) != names.end();
}

// Everything up to date: every output exists and is newer than its inputs.
FileTimes fresh_times() {
  return FileTimes{{"pid.cpp", 10},  {"safety.cpp", 10}, {"rc.hpp", 10}, {"main.cpp", 10},
                   {"pid.o", 20},    {"safety.o", 20},   {"librc.a", 30}, {"robot", 40}};
}

}  // namespace

RC_TEST("an empty graph has an empty order") {
  const BuildGraph graph;
  const auto ordering = graph.order();
  RC_REQUIRE(ordering.has_value());
  RC_CHECK(ordering.value().empty());
}

RC_TEST("every target appears exactly once in the order") {
  const auto ordering = small_project().order();
  RC_REQUIRE(ordering.has_value());
  RC_CHECK_EQ(ordering.value().size(), std::size_t{4});
}

RC_TEST("a target comes after everything it depends on") {
  const auto ordering = small_project().order();
  RC_REQUIRE(ordering.has_value());
  const std::vector<std::string>& order = ordering.value();

  RC_CHECK(comes_before(order, "pid.o", "librc"));
  RC_CHECK(comes_before(order, "safety.o", "librc"));
  RC_CHECK(comes_before(order, "librc", "robot"));
}

RC_TEST("a chain is ordered from one end to the other") {
  BuildGraph graph;
  graph.add(target("c", {"b"}, {}, "c.out"));
  graph.add(target("b", {"a"}, {}, "b.out"));
  graph.add(target("a", {}, {}, "a.out"));

  const auto ordering = graph.order();
  RC_REQUIRE(ordering.has_value());
  RC_CHECK(comes_before(ordering.value(), "a", "b"));
  RC_CHECK(comes_before(ordering.value(), "b", "c"));
}

RC_TEST("a dependency that was never added is reported as unknown") {
  BuildGraph graph;
  graph.add(target("robot", {"librc"}, {}, "robot"));

  const auto ordering = graph.order();
  RC_REQUIRE(!ordering.has_value());
  RC_CHECK(ordering.error() == GraphError::UnknownDependency);
}

RC_TEST("a cycle is refused rather than looped over") {
  BuildGraph graph;
  graph.add(target("a", {"b"}, {}, "a.out"));
  graph.add(target("b", {"a"}, {}, "b.out"));

  const auto ordering = graph.order();
  RC_REQUIRE(!ordering.has_value());
  RC_CHECK(ordering.error() == GraphError::Cycle);
}

RC_TEST("a longer cycle is caught too") {
  BuildGraph graph;
  graph.add(target("a", {"c"}, {}, "a.out"));
  graph.add(target("b", {"a"}, {}, "b.out"));
  graph.add(target("c", {"b"}, {}, "c.out"));

  const auto ordering = graph.order();
  RC_REQUIRE(!ordering.has_value());
  RC_CHECK(ordering.error() == GraphError::Cycle);
}

RC_TEST("an unknown dependency is reported even when a cycle also exists") {
  // Checking the unknown case first matters: reporting a cycle for a target
  // that simply is not there sends the reader looking in the wrong place.
  BuildGraph graph;
  graph.add(target("a", {"b"}, {}, "a.out"));
  graph.add(target("b", {"a", "missing"}, {}, "b.out"));

  const auto ordering = graph.order();
  RC_REQUIRE(!ordering.has_value());
  RC_CHECK(ordering.error() == GraphError::UnknownDependency);
}

RC_TEST("nothing is stale when everything is up to date") {
  RC_CHECK(small_project().stale(fresh_times()).empty());
}

RC_TEST("a target with no output yet is stale") {
  FileTimes times = fresh_times();
  times.erase("robot");
  const std::vector<std::string> work = small_project().stale(times);
  RC_CHECK(contains_name(work, "robot"));
  RC_CHECK_EQ(work.size(), std::size_t{1});
}

RC_TEST("touching a source rebuilds its target") {
  FileTimes times = fresh_times();
  times["main.cpp"] = 100;   // newer than the robot output
  const std::vector<std::string> work = small_project().stale(times);
  RC_CHECK(contains_name(work, "robot"));
  RC_CHECK(!contains_name(work, "librc"));
}

RC_TEST("touching a shared header rebuilds everything downstream") {
  // The clause people forget. One header, two objects, then the library, then
  // the program. Without the third rule this rebuilds the objects and links the
  // old library, and the program is quietly built from mismatched pieces.
  FileTimes times = fresh_times();
  times["rc.hpp"] = 100;

  const std::vector<std::string> work = small_project().stale(times);
  RC_CHECK(contains_name(work, "pid.o"));
  RC_CHECK(contains_name(work, "safety.o"));
  RC_CHECK(contains_name(work, "librc"));
  RC_CHECK(contains_name(work, "robot"));
  RC_CHECK_EQ(work.size(), std::size_t{4});
}

RC_TEST("stale targets are listed in build order") {
  FileTimes times = fresh_times();
  times["rc.hpp"] = 100;

  const std::vector<std::string> work = small_project().stale(times);
  RC_CHECK(comes_before(work, "pid.o", "librc"));
  RC_CHECK(comes_before(work, "librc", "robot"));
}

RC_TEST("an unrelated edit leaves the graph alone") {
  FileTimes times = fresh_times();
  times["notes.txt"] = 100;
  RC_CHECK(small_project().stale(times).empty());
}

RC_TEST("staleness spreads only downstream, never back up") {
  // Rebuilding the program must not mark the library that produced it.
  FileTimes times = fresh_times();
  times["main.cpp"] = 100;
  const std::vector<std::string> work = small_project().stale(times);
  RC_CHECK(!contains_name(work, "pid.o"));
  RC_CHECK(!contains_name(work, "librc"));
}
