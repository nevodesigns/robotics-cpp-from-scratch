// rcpp
//
// One binary carries this repository: it checks the learner's machine, enforces
// the lesson contract, verifies a learner's work against the real test suite,
// and explains errors. It is written in C++ for the same reason the lessons are:
// this curriculum teaches one language, and that includes its own tools.

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

#include "commands.hpp"
#include "util.hpp"

namespace rcpp {

bool has_flag(const Args& args, const std::string& flag) {
  return std::find(args.begin(), args.end(), flag) != args.end();
}

std::string flag_value(const Args& args, const std::string& flag, const std::string& fallback) {
  for (std::size_t i = 0; i < args.size(); ++i) {
    if (args[i] == flag && i + 1 < args.size()) return args[i + 1];
    if (starts_with(args[i], flag + "=")) return args[i].substr(flag.size() + 1);
  }
  return fallback;
}

namespace {

int usage() {
  std::cout << R"(rcpp, the tool that carries Robotics C++ From Scratch

usage: rcpp <command> [options]

  doctor          check this machine and print the exact fix for anything missing
  start           show which lesson to open first
  next            show what is unlocked now, from what you have passed
  status          show how far you have got, phase by phase
  list            list every phase and lesson found on disk
  verify <id>     build your exercise and run the lesson's real test suite
  explain [text]  look an error up in the Failure Atlas, or pipe it in on stdin
  audit           enforce the lesson contract, this is what CI runs
  catalog         write the whole curriculum as JSON
  readme          check, or fix, the generated counts in README.md

examples

  rcpp doctor
  rcpp verify 00-01
  cmake --build build 2>&1 | rcpp explain
  rcpp audit --json
  rcpp readme --check
)";
  return 0;
}

}  // namespace
}  // namespace rcpp

int main(int argc, char** argv) {
  using namespace rcpp;
  if (argc < 2) return usage();

  const std::string command = argv[1];
  Args args(argv + 2, argv + argc);

  if (command == "-h" || command == "--help" || command == "help") return usage();
  if (command == "doctor") return cmd_doctor(args);
  if (command == "start") return cmd_start(args);
  if (command == "list") return cmd_list(args);
  if (command == "verify") return cmd_verify(args);
  if (command == "explain") return cmd_explain(args);
  if (command == "audit") return cmd_audit(args);
  if (command == "catalog") return cmd_catalog(args);
  if (command == "readme") return cmd_readme(args);
  if (command == "next") return cmd_next(args);
  if (command == "status") return cmd_status(args);

  std::cerr << "rcpp: unknown command: " << command << "\n\n";
  usage();
  return 2;
}
