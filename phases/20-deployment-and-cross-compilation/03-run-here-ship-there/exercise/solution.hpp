#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <algorithm>
#include <rc/deploy/target.hpp>
#include <string>
#include <vector>

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
  // TODO: false if either is invalid or the os differs; true for the same arch; true if the host is x86_64 and the binary is i686 or i386; else false
  (void)built_for;
  (void)host;
  return false;
}

// What is wrong with a build graph, in this order:
//  - an invalid host or target triple: one Issue on step "triple" ("cannot be read") and nothing else;
//  - each step, in order, that is executed in the build but is built for the target and could not run on the host: "built for the target but run during the build";
//  - each edge, in order: a step that is not in the list (Issue on that name, "not a step"), or two steps that are built for machines that cannot run each other's binaries (Issue on `from`, "links <to>, built for the other machine");
//  - each name in `install`, in order: not a step ("not a step"), or built for the host while the host could not run the target's ("host binary in the bundle").
// Where the host could run what the target runs (a native build), the "other machine" checks do not apply: there is only one machine.
inline std::vector<Issue> check(const std::vector<Step>& steps, const std::vector<Edge>& edges, const std::vector<std::string>& install, const rc::deploy::Target& host, const rc::deploy::Target& target) {
  // TODO: the checks in the comment above, in that order. same_machine is can_run both ways; Where::host and Where::target steps differ only when it is not the same machine
  (void)steps;
  (void)edges;
  (void)install;
  (void)host;
  (void)target;
  return std::vector<Issue>();
}

#endif  // LESSON_SOLUTION_HPP
