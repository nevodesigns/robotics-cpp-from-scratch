#include <rc/test/rc_test.hpp>

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <system_error>

#include "solution.hpp"

namespace {

namespace fs = std::filesystem;

// Quoted, because a path on any of these platforms can contain a space and the
// command is handed to a shell.
std::string quoted(const fs::path& path) { return "\"" + path.string() + "\""; }

// Runs a command with its output sent to a file, and returns the exit status.
// The output is kept so a failing test can say what the build actually said.
int run(const std::string& command, const fs::path& log) {
  const std::string full = command + " > " + quoted(log) + " 2>&1";
  return std::system(full.c_str());
}

std::string tail_of(const fs::path& log, int lines) {
  std::ifstream input(log);
  std::vector<std::string> all;
  std::string line;
  while (std::getline(input, line)) all.push_back(line);

  std::ostringstream out;
  const std::size_t start = all.size() > static_cast<std::size_t>(lines)
                                ? all.size() - static_cast<std::size_t>(lines)
                                : 0;
  for (std::size_t i = start; i < all.size(); ++i) out << "      " << all[i] << "\n";
  return out.str();
}

// A directory of our own, removed when the test finishes whether it passed or
// not, which is lesson 02-03 applied to something that is not memory.
class Scratch {
 public:
  Scratch() {
    std::error_code ignored;
    for (int attempt = 0; attempt < 100; ++attempt) {
      const fs::path candidate =
          fs::temp_directory_path() / ("rc-04-03-" + std::to_string(attempt));
      fs::remove_all(candidate, ignored);
      if (fs::create_directories(candidate, ignored)) {
        path_ = candidate;
        return;
      }
    }
  }

  ~Scratch() {
    std::error_code ignored;
    if (!path_.empty()) fs::remove_all(path_, ignored);
  }

  Scratch(const Scratch&) = delete;
  Scratch& operator=(const Scratch&) = delete;

  bool ok() const { return !path_.empty(); }
  const fs::path& path() const { return path_; }

 private:
  fs::path path_;
};

fs::path variant_dir() { return fs::path(RC_LESSON_VARIANT_DIR); }

}  // namespace

RC_TEST("the library itself does what a rate limiter does") {
  // The C++ is not what this lesson is about, and it still has to be right, or
  // a consumer that builds would not prove anything.
  RC_CHECK_NEAR(steplib::step_toward(0.95, 1.0, 0.1), 1.0, 1e-9);
  RC_CHECK_NEAR(steplib::step_toward(0.0, 100.0, 0.1), 0.1, 1e-9);
  RC_CHECK_NEAR(steplib::step_toward(0.0, -100.0, 0.1), -0.1, 1e-9);
  RC_CHECK_NEAR(steplib::step_toward(1.0, 1.0, 0.1), 1.0, 1e-9);
}

RC_TEST("the package installs, and a project that is not yours consumes it") {
  // The whole lesson, end to end, using the same cmake that built this test.
  //
  // The sources are deleted after installing and before the consumer is built.
  // A package that only works while its source tree is still present is the
  // most common way to get this wrong and it cannot be noticed on the machine
  // that made it, so this test arranges for it to be noticed here.
  Scratch scratch;
  RC_REQUIRE(scratch.ok());

  const fs::path source = scratch.path() / "source";
  const fs::path install = scratch.path() / "install";
  const fs::path build_package = scratch.path() / "build-package";
  const fs::path build_consumer = scratch.path() / "build-consumer";
  const fs::path consumer = scratch.path() / "consumer";

  std::error_code error;
  fs::copy(variant_dir() / "package", source, fs::copy_options::recursive, error);
  RC_REQUIRE(!error);
  fs::copy(variant_dir() / "consumer", consumer, fs::copy_options::recursive, error);
  RC_REQUIRE(!error);

  const fs::path log = scratch.path() / "log.txt";

  const int configured = run("cmake -S " + quoted(source) + " -B " + quoted(build_package) +
                                 " -DCMAKE_INSTALL_PREFIX=" + quoted(install),
                             log);
  if (configured != 0) std::cout << "\n    configuring the package failed:\n" << tail_of(log, 12);
  RC_REQUIRE_EQ(configured, 0);

  const int installed = run("cmake --build " + quoted(build_package) +
                                " --config Release --target install",
                            log);
  if (installed != 0) std::cout << "\n    installing the package failed:\n" << tail_of(log, 12);
  RC_REQUIRE_EQ(installed, 0);

  // What a package must contain to be findable at all.
  RC_CHECK(fs::exists(install / "include" / "steplib" / "step.hpp"));

  // Now it is on its own.
  fs::remove_all(source, error);
  RC_REQUIRE(!fs::exists(source));

  const int found = run("cmake -S " + quoted(consumer) + " -B " + quoted(build_consumer) +
                            " -DCMAKE_PREFIX_PATH=" + quoted(install),
                        log);
  if (found != 0) std::cout << "\n    the consumer could not find the package:\n" << tail_of(log, 14);
  RC_REQUIRE_EQ(found, 0);

  const int built = run("cmake --build " + quoted(build_consumer) + " --config Release", log);
  if (built != 0) std::cout << "\n    the consumer found the package and could not build:\n"
                            << tail_of(log, 14);
  RC_REQUIRE_EQ(built, 0);

  std::cout << "\n    a package installed, its source deleted, and a separate\n"
               "    project built against what was left.\n";
}
