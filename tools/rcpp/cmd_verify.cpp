// rcpp verify
//
// The command that answers "am I done?". It builds the learner's exercise and
// runs the same test suite that continuous integration runs against the
// reference implementation. Green means finished. Nothing else does.

#include <cstdlib>
#include <iostream>

#include "catalog.hpp"
#include "progress.hpp"
#include "commands.hpp"
#include "util.hpp"

namespace rcpp {
namespace {

int shell(const std::string& command) {
  std::cout << style::dim("  $ " + command) << "\n";
  const int status = std::system(command.c_str());
#if defined(_WIN32)
  return status;
#else
  return (status == -1) ? -1 : (status >> 8) & 0xFF;
#endif
}

std::string quote(const fs::path& path) { return "\"" + path.string() + "\""; }

}  // namespace

int cmd_verify(const Args& args) {
  const auto root = find_repo_root();
  if (!root) {
    std::cerr << "rcpp verify: run this from inside the repository\n";
    return 2;
  }

  const Catalog catalog = load_catalog(*root);
  const bool reference = has_flag(args, "--reference");
  const bool all = has_flag(args, "--all");
  const fs::path build_dir = *root / flag_value(args, "--build-dir", "build/verify");

  std::string wanted;
  for (const std::string& a : args)
    if (!starts_with(a, "--")) { wanted = a; break; }

  if (!all && wanted.empty()) {
    std::cerr << "usage: rcpp verify <lesson-id> [--reference] [--build-dir DIR]\n"
                 "       rcpp verify --all\n";
    return 2;
  }

  const Lesson* lesson = nullptr;
  if (!all) {
    lesson = catalog.find(wanted);
    if (!lesson) {
      std::cerr << "rcpp verify: no lesson matches " << wanted << "\n"
                << "  try: rcpp list\n";
      return 2;
    }
  }

  std::cout << "\n" << style::bold(all ? "Verifying every lesson" : "Verifying " + lesson->id) << "\n\n";

  const std::string configure = "cmake -S " + quote(*root) + " -B " + quote(build_dir) +
                                " -DCMAKE_BUILD_TYPE=Debug" +
                                (on_path("ninja") ? " -G Ninja" : "");
  if (!fs::exists(build_dir / "CMakeCache.txt")) {
    if (shell(configure) != 0) {
      std::cerr << style::fail("\nConfiguration failed.") << " Paste the error into rcpp explain.\n";
      return 1;
    }
  }

  const std::string suffix = reference ? "_reference" : "_exercise";
  const std::string target = all ? "" : (" --target " + lesson->target_name() + suffix);
  if (shell("cmake --build " + quote(build_dir) + target) != 0) {
    // A lesson needing Qt on a machine without it is not a failed attempt, it
    // is a lesson that was never built, and the build system says so in a way
    // nobody can act on: unknown target. Saying which it is costs one check.
    if (lesson != nullptr && !lesson->qt_modules.empty() && !qt6_present()) {
      std::cerr << "\n" << style::warn("This lesson needs Qt, which is not installed here.")
                << "\n\n"
                << "  It was skipped when the build was configured, which is why the\n"
                << "  target does not exist rather than failing to compile.\n\n"
                << "  " << style::bold("rcpp doctor") << " prints the exact command for this machine.\n"
                << "  Or skip it: every lesson that needs Qt has a counterpart that does not,\n"
                << "  and nothing later depends on having done this one.\n\n";
      return 1;
    }

    std::cerr << style::fail("\nIt did not build yet.")
              << " That is normal at the start of a lesson.\n"
              << "  Paste the first error into: " << style::bold("rcpp explain") << "\n";
    return 1;
  }

  const std::string filter = all ? "" : (" -R \"^" + lesson->id + "\\." + (reference ? "reference" : "exercise") + "$\"");
  const int test_status = shell("ctest --test-dir " + quote(build_dir) + filter + " --output-on-failure");

  if (test_status == 0) {
    std::cout << style::pass("\nPassed.")
              << " Your implementation satisfies the same tests the reference does.\n";

    // Progress is only ever recorded here, after the real suite passed against
    // the learner's own code. There is no command that marks a lesson done by
    // assertion, because the tests are what decide.
    if (!all && !reference && lesson != nullptr) {
      if (record_pass(*root, lesson->id)) {
        std::cout << "  " << style::dim("recorded, run rcpp next for what this unlocks") << "\n";
      }
    }
    std::cout << "\n";
    return 0;
  }
  std::cout << style::fail("\nNot passing yet.") << " Read the first failing check above, it names the exact expectation.\n\n";
  return 1;
}

}  // namespace rcpp
