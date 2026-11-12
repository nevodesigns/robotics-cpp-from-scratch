// rc/deploy/build_split.hpp
//
// A check of a build graph for what is built for the wrong machine, from lesson 20-03, graduated.
//
// can_run says whether a binary built for a target runs on a host (same architecture and system, or 32 bit x86 on 64 bit x86). check takes the steps (each built for the host or the target, and whether the build itself runs it), the links between them and the install set, and names: a target binary executed during the build, a library linked from the other machine, a name that is not a step, and a host binary in the bundle. Where the host can run what the target runs, there is one machine and the machine checks do not apply.
//
// A graph with a generator and tests built for the target, a library built on the laptop and a host tool in the bundle had 0 problems built for the host itself and 4 built on x86_64 for aarch64; corrected it had 0.

#ifndef RC_DEPLOY_BUILD_SPLIT_HPP
#define RC_DEPLOY_BUILD_SPLIT_HPP

#include <algorithm>
#include <rc/deploy/target.hpp>
#include <string>
#include <vector>

namespace rc {
namespace deploy {

// A build has two machines: the HOST that does the building and the TARGET that the result is for. Each piece is built for one of them.
enum class Where { host, target };

struct Step {
  std::string name;
  Where built_for = Where::target;
  bool executed_in_build = false;     // run by the build itself: a code generator, a test
};

struct Edge {
  std::string from;                   // links
  std::string to;                     // this library
};

struct Issue {
  std::string step;
  std::string message;
};

// Could a binary built for `built_for` run on `host`? Only for the same architecture and operating system, and a 32 bit x86 binary on a 64 bit x86 host of the same system. Invalid triples run nowhere.
inline bool can_run(const rc::deploy::Target& built_for, const rc::deploy::Target& host) {
  if (!built_for.valid || !host.valid) return false;
  if (built_for.os != host.os) return false;
  if (built_for.arch == host.arch) return true;
  return host.arch == "x86_64" && (built_for.arch == "i686" || built_for.arch == "i386");
}

// What is wrong with a build graph, in this order:
//  - an invalid host or target triple: one Issue on step "triple" ("cannot be read") and nothing else;
//  - each step, in order, that is executed in the build but is built for the target and could not run on the host: "built for the target but run during the build";
//  - each edge, in order: a step that is not in the list (Issue on that name, "not a step"), or two steps that are built for machines that cannot run each other's binaries (Issue on `from`, "links <to>, built for the other machine");
//  - each name in `install`, in order: not a step ("not a step"), or built for the host while the host could not run the target's ("host binary in the bundle").
// Where the host could run what the target runs (a native build), the "other machine" checks do not apply: there is only one machine.
inline std::vector<Issue> check(const std::vector<Step>& steps, const std::vector<Edge>& edges, const std::vector<std::string>& install, const rc::deploy::Target& host, const rc::deploy::Target& target) {
  std::vector<Issue> out;
  if (!host.valid || !target.valid) {
    out.push_back({"triple", "cannot be read"});
    return out;
  }
  const bool same_machine = can_run(target, host) && can_run(host, target);
  auto find = [&steps](const std::string& n) -> const Step* {
    for (const Step& s : steps)
      if (s.name == n) return &s;
    return nullptr;
  };
  auto differ = [&](const Step& a, const Step& b) { return !same_machine && a.built_for != b.built_for; };
  for (const Step& s : steps)
    if (s.executed_in_build && s.built_for == Where::target && !can_run(target, host)) out.push_back({s.name, "built for the target but run during the build"});
  for (const Edge& e : edges) {
    const Step* a = find(e.from);
    const Step* b = find(e.to);
    if (a == nullptr) out.push_back({e.from, "not a step"});
    else if (b == nullptr) out.push_back({e.to, "not a step"});
    else if (differ(*a, *b)) out.push_back({e.from, "links " + e.to + ", built for the other machine"});
  }
  for (const std::string& n : install) {
    const Step* s = find(n);
    if (s == nullptr) out.push_back({n, "not a step"});
    else if (s->built_for == Where::host && !same_machine) out.push_back({n, "host binary in the bundle"});
  }
  return out;
}

}  // namespace deploy
}  // namespace rc

#endif  // RC_DEPLOY_BUILD_SPLIT_HPP
