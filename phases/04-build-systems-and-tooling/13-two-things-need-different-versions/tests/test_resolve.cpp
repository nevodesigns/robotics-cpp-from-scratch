#include <rc/test/rc_test.hpp>

#include <iostream>
#include <string>
#include <vector>

#include "solution.hpp"

namespace {

Package pkg(const char* name, const char* version) {
  Package p;
  p.name = name;
  p.version = ver(version);
  return p;
}

Package needs(Package p, const char* dep, const Range& r) {
  Dep d;
  d.name = dep;
  d.range = r;
  d.from = p.name + " " + std::to_string(p.version.major) + "." + std::to_string(p.version.minor);
  p.deps.push_back(d);
  return p;
}

Dep root(const char* name, const Range& r) {
  Dep d;
  d.name = name;
  d.range = r;
  d.from = "the project";
  return d;
}

std::string chosen_version(const Resolution& r, const char* name) {
  for (const std::pair<std::string, std::string>& c : r.chosen) {
    if (c.first == name) return c.second;
  }
  return "";
}

}  // namespace

RC_TEST("a range includes its low end and not its high end, and may have no high end") {
  const Range r = range("1.0.0", "2.0.0");
  RC_CHECK(contains(r, ver("1.0.0")));
  RC_CHECK(contains(r, ver("1.9.9")));
  RC_CHECK(!contains(r, ver("2.0.0")));
  RC_CHECK(!contains(r, ver("0.9.9")));

  const Range open = at_least("1.5.0");
  RC_CHECK(contains(open, ver("1.5.0")));
  RC_CHECK(contains(open, ver("99.0.0")));
  RC_CHECK(!contains(open, ver("1.4.9")));

  RC_CHECK(!is_empty(r));
  RC_CHECK(!is_empty(open));
  RC_CHECK(is_empty(range("2.0.0", "2.0.0")));
  RC_CHECK(is_empty(range("3.0.0", "2.0.0")));
}

RC_TEST("what two ranges both allow is the larger low end up to the smaller high end") {
  const Range both = intersect(range("1.0.0", "3.0.0"), range("1.5.0", "2.5.0"));
  RC_CHECK_EQ(compare(both.low, ver("1.5.0")), 0);
  RC_REQUIRE(both.has_high);
  RC_CHECK_EQ(compare(both.high, ver("2.5.0")), 0);

  const Range with_open = intersect(at_least("1.2.0"), range("1.0.0", "2.0.0"));
  RC_CHECK_EQ(compare(with_open.low, ver("1.2.0")), 0);
  RC_REQUIRE(with_open.has_high);
  RC_CHECK_EQ(compare(with_open.high, ver("2.0.0")), 0);

  const Range neither = intersect(at_least("1.0.0"), at_least("2.0.0"));
  RC_CHECK(!neither.has_high);
  RC_CHECK_EQ(compare(neither.low, ver("2.0.0")), 0);

  RC_CHECK(is_empty(intersect(range("1.0.0", "2.0.0"), range("2.0.0", "3.0.0"))));      // touching is not overlapping
  RC_CHECK(is_empty(intersect(range("1.0.0", "1.5.0"), range("2.0.0", "3.0.0"))));
}

RC_TEST("a diamond: two things need the same package in different ranges and the newest that suits both is chosen") {
  std::vector<Package> repo;
  repo.push_back(needs(pkg("A", "1.0.0"), "C", range("1.0.0", "3.0.0")));
  repo.push_back(needs(pkg("B", "1.0.0"), "C", range("1.5.0", "2.5.0")));
  const char* c_versions[] = {"1.0.0", "1.4.0", "1.8.0", "2.4.0", "2.9.0", "3.1.0"};
  for (const char* v : c_versions) repo.push_back(pkg("C", v));

  std::vector<Dep> roots;
  roots.push_back(root("A", at_least("0.0.0")));
  roots.push_back(root("B", at_least("0.0.0")));

  const Resolution r = resolve(roots, repo);
  std::cout << "  A needs C in [1.0, 3.0), B needs C in [1.5, 2.5), C is available as 1.0 1.4 1.8 2.4 2.9 3.1: chosen";
  for (const std::pair<std::string, std::string>& c : r.chosen) std::cout << " " << c.first << " " << c.second << ",";
  std::cout << "\n\n";
  RC_REQUIRE(r.ok);
  RC_CHECK_EQ(chosen_version(r, "C"), std::string("2.4.0"));
  RC_CHECK_EQ(r.chosen.size(), static_cast<std::size_t>(3));
}

RC_TEST("when no version suits everyone, the answer names the package and who disagreed") {
  std::vector<Package> repo;
  repo.push_back(needs(pkg("A", "1.0.0"), "C", range("1.0.0", "2.0.0")));
  repo.push_back(needs(pkg("B", "1.0.0"), "C", range("2.0.0", "3.0.0")));
  repo.push_back(pkg("C", "1.5.0"));
  repo.push_back(pkg("C", "2.5.0"));

  std::vector<Dep> roots;
  roots.push_back(root("A", at_least("0.0.0")));
  roots.push_back(root("B", at_least("0.0.0")));

  const Resolution r = resolve(roots, repo);
  std::cout << "  " << r.problem << "\n\n";
  RC_CHECK(!r.ok);
  RC_CHECK(r.problem.find("C") == 0);
  RC_CHECK(r.problem.find("A 1.0") != std::string::npos);
  RC_CHECK(r.problem.find("B 1.0") != std::string::npos);
  RC_CHECK(r.chosen.empty());
}

RC_TEST("when the newest version of something cannot be used, the resolver backs up to an older one") {
  std::vector<Package> repo;
  repo.push_back(needs(pkg("A", "2.0.0"), "C", range("2.0.0", "3.0.0")));       // the newest A wants a new C
  repo.push_back(needs(pkg("A", "1.0.0"), "C", range("1.0.0", "2.0.0")));
  repo.push_back(needs(pkg("B", "1.0.0"), "C", range("1.0.0", "2.0.0")));       // and B cannot have one
  repo.push_back(pkg("C", "1.7.0"));
  repo.push_back(pkg("C", "2.1.0"));

  std::vector<Dep> roots;
  roots.push_back(root("A", at_least("0.0.0")));
  roots.push_back(root("B", at_least("0.0.0")));

  int steps = 0;
  const Resolution r = resolve(roots, repo, &steps);
  std::cout << "  the newest A is 2.0.0 and it cannot live with B: chosen A " << chosen_version(r, "A") << ", C "
            << chosen_version(r, "C") << " after " << steps << " steps\n\n";
  RC_REQUIRE(r.ok);
  RC_CHECK_EQ(chosen_version(r, "A"), std::string("1.0.0"));
  RC_CHECK_EQ(chosen_version(r, "B"), std::string("1.0.0"));
  RC_CHECK_EQ(chosen_version(r, "C"), std::string("1.7.0"));
}

RC_TEST("a package that does not exist, and two packages that need each other, both end") {
  std::vector<Package> repo;
  std::vector<Dep> roots;
  roots.push_back(root("Z", at_least("1.0.0")));
  const Resolution missing = resolve(roots, repo);
  RC_CHECK(!missing.ok);
  RC_CHECK_EQ(missing.problem, std::string("Z: no version is available at all"));

  // A needs B and B needs A: each is chosen once, so the loop does not go round.
  std::vector<Package> cycle;
  cycle.push_back(needs(pkg("A", "1.0.0"), "B", at_least("1.0.0")));
  cycle.push_back(needs(pkg("B", "1.0.0"), "A", at_least("1.0.0")));
  std::vector<Dep> start;
  start.push_back(root("A", at_least("1.0.0")));
  int steps = 0;
  const Resolution r = resolve(start, cycle, &steps);
  std::cout << "  A needs B and B needs A: " << (r.ok ? "resolved" : "failed") << " in " << steps << " steps\n\n";
  RC_REQUIRE(r.ok);
  RC_CHECK_EQ(r.chosen.size(), static_cast<std::size_t>(2));
  RC_CHECK(steps < 10);
}
