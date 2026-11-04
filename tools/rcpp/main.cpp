// rcpp
//
// One binary carries this repository: it checks the learner's machine, enforces
// the lesson contract, verifies a learner's work against the real test suite,
// and explains errors. It is written in C++ for the same reason the lessons are:
// this curriculum teaches one language, and that includes its own tools.

#include <iostream>
#include <string>
#include <vector>

#include "commands.hpp"
#include "util.hpp"

namespace rcpp {

bool read_option(const Args& args, const char* name, std::string& into, const char* command) {
  const rc::core::Option found = args.option(name);
  if (!found.present) return true;
  if (!found.has_value) {
    std::cerr << "rcpp " << command << ": " << name << " needs a value\n";
    return false;
  }
  into.assign(found.value.begin(), found.value.end());
  return true;
}

bool read_count(const Args& args, const char* name, int& into, const char* command) {
  // option() first, so "given with nothing after it" is reported as the
  // missing value it is rather than as an unparseable number. number()
  // folds those two together, which is right for its own signature and not
  // specific enough for a message a user has to act on.
  const rc::core::Option given = args.option(name);
  if (given.present && !given.has_value) {
    std::cerr << "rcpp " << command << ": " << name << " needs a value\n";
    return false;
  }

  const Args::Number found = args.number(name);
  if (!found.present) return true;
  if (!found.ok) {
    std::cerr << "rcpp " << command << ": " << name << " needs a whole number\n";
    return false;
  }
  const double value = found.value;
  if (value < 1.0 || value != static_cast<double>(static_cast<int>(value))) {
    std::cerr << "rcpp " << command << ": " << name
              << " must be a whole number of at least 1\n";
    return false;
  }
  into = static_cast<int>(value);
  return true;
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
  platforms       the toolchains claimed, and the CI matrix generated from them
  targets         build target names for lessons, for any generator
  readme          check, or fix, the generated counts in README.md

examples

  rcpp doctor
  rcpp verify 00-01
  cmake --build build 2>&1 | rcpp explain
  rcpp audit --json
  rcpp next --blocked
  rcpp explain E-RT-0001
  rcpp readme --check
)";
  return 0;
}

}  // namespace
}  // namespace rcpp

int main(int argc, char** argv) {
  using namespace rcpp;
  if (argc < 2) return usage();

  // argv + 1 so that program() is the subcommand and the arguments start
  // after it. Arguments stops at argc and at a null entry, whichever comes
  // first, so a short argv cannot walk off the end.
  const std::string command = argv[1];
  const Args args(argc - 1, argv + 1);

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
  if (command == "platforms") return cmd_platforms(args);
  if (command == "targets") return cmd_targets(args);

  std::cerr << "rcpp: unknown command: " << command << "\n\n";
  usage();
  return 2;
}
