// rcpp targets
//
// The build target names for lessons, derived from the catalog.
//
//   rcpp targets --exercises              every lesson's exercise target
//   rcpp targets --references             every lesson's reference target
//   rcpp targets --exercises --platform X only lessons claiming that toolchain
//
// Continuous integration needs this list to check that every shipped exercise
// still fails. Asking the build system for it is generator specific: the help
// target exists with Makefiles and not with Ninja, which is a difference that
// cost a green build the first time this workflow ran. Deriving the names from
// lesson.json instead works with any generator.

#include <iostream>

#include "catalog.hpp"
#include "commands.hpp"
#include "util.hpp"

namespace rcpp {

int cmd_targets(const Args& args) {
  const auto root = find_repo_root();
  if (!root) { std::cerr << "rcpp targets: run this from inside the repository\n"; return 2; }

  const bool want_exercises = has_flag(args, "--exercises");
  const bool want_references = has_flag(args, "--references");
  if (!want_exercises && !want_references) {
    std::cerr << "usage: rcpp targets --exercises | --references [--platform ID]\n";
    return 2;
  }

  const std::string platform = flag_value(args, "--platform", "");
  const Catalog catalog = load_catalog(*root);

  for (const auto* lesson : catalog.all()) {
    if (!platform.empty()) {
      bool claimed = false;
      for (const std::string& id : lesson->platforms)
        if (id == platform) claimed = true;
      if (!claimed) continue;
    }
    if (want_exercises) std::cout << lesson->target_name() << "_exercise\n";
    if (want_references) std::cout << lesson->target_name() << "_reference\n";
  }
  return 0;
}

}  // namespace rcpp
