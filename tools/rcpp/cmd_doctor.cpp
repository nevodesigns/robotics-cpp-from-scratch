// rcpp doctor
//
// The first command a learner runs, and the one that decides whether they stay.
// It reports what is installed, what is missing, and the exact command that
// fixes each gap on this operating system. It never guesses: every check either
// found a real program or it did not.

#include <iostream>
#include <sstream>
#include <vector>

#include "commands.hpp"
#include "json.hpp"
#include "util.hpp"

namespace rcpp {
namespace {

enum class Level { Required, RequiredLater, Optional };

struct Check {
  std::string id;
  std::string label;
  Level level = Level::Required;
  bool ok = false;
  std::string detail;   // what was found
  std::string fix;      // what to run when it was not
};

struct Host {
  std::string os;          // linux | windows | macos
  std::string distro;      // ubuntu
  std::string version_id;  // 22.04
  bool wsl = false;
  std::string pretty;
};

Host detect_host() {
  Host host;
#if defined(_WIN32)
  host.os = "windows";
  host.pretty = "Windows";
#elif defined(__APPLE__)
  host.os = "macos";
  host.pretty = "macOS";
#else
  host.os = "linux";
  host.pretty = "Linux";
  if (const auto release = read_file("/etc/os-release")) {
    for (const std::string& line : split(*release, '\n')) {
      const std::size_t eq = line.find('=');
      if (eq == std::string::npos) continue;
      std::string key = trim(line.substr(0, eq));
      std::string value = trim(line.substr(eq + 1));
      if (!value.empty() && value.front() == '"') value = value.substr(1, value.size() - 2);
      if (key == "ID") host.distro = value;
      if (key == "VERSION_ID") host.version_id = value;
      if (key == "PRETTY_NAME") host.pretty = value;
    }
  }
  if (const auto proc = read_file("/proc/version"))
    host.wsl = contains(*proc, "microsoft") || contains(*proc, "Microsoft");
#endif
  return host;
}

std::string apt_install(const std::string& packages) {
  return "sudo apt update && sudo apt install -y " + packages;
}

Check probe_program(const std::string& id, const std::string& label,
                    const std::string& program, const std::string& version_flag,
                    Level level, const std::string& fix) {
  Check check;
  check.id = id;
  check.label = label;
  check.level = level;
  check.fix = fix;
  if (!on_path(program)) {
    check.detail = "not found on PATH";
    return check;
  }
  const CommandResult result = run(program + " " + version_flag);
  check.ok = true;
  check.detail = first_line(result.output);
  return check;
}

// Ubuntu 22.04 carries GCC 11, which has neither std::format nor std::expected.
// That is supported and expected: rc/core/compat.hpp fills both gaps. The check
// reports the fact so the learner is never surprised by it later.
Check probe_cxx_features(const Host& host) {
  Check check;
  check.id = "cxx-library";
  check.label = "C++20 library features";
  check.level = Level::Optional;
  check.ok = true;
  if (host.distro == "ubuntu" && starts_with(host.version_id, "22.04")) {
    check.detail = "GCC 11 has no std::format or std::expected, rc::format and rc::expected cover it";
  } else {
    check.detail = "assumed complete, lesson 04-08 explains the shim either way";
  }
  return check;
}

Check probe_qt(const Host& host) {
  Check check;
  check.id = "qt6";
  check.label = "Qt 6 development files";
  check.level = Level::RequiredLater;
  if (host.os == "linux")
    check.fix = apt_install("qt6-base-dev qt6-declarative-dev");
  else if (host.os == "windows")
    check.fix = "install Qt 6 with the official open source installer, then add it to CMAKE_PREFIX_PATH";
  else
    check.fix = "brew install qt6";

  if (on_path("qmake6")) {
    const CommandResult version = run("qmake6 -query QT_VERSION");
    check.ok = true;
    check.detail = "Qt " + trim(version.output);
    return check;
  }
  if (fs::exists("/usr/lib/x86_64-linux-gnu/cmake/Qt6")) {
    check.ok = true;
    check.detail = "found the Qt6 CMake package";
    return check;
  }
  check.detail = "not found, needed from phase 09 onward";
  return check;
}

Check probe_ros2(const Host& host) {
  Check check;
  check.id = "ros2";
  check.label = "ROS 2 (optional module, phase 17)";
  check.level = Level::Optional;
  if (host.os != "linux") {
    check.detail = "not available on this operating system, use WSL2 for phase 17";
    check.fix = "wsl --install -d Ubuntu-24.04";
    return check;
  }
  const std::string distro = starts_with(host.version_id, "22.04") ? "humble" : "jazzy";
  check.fix = "follow docs.ros.org for ROS 2 " + distro + ", then source /opt/ros/" + distro + "/setup.bash";
  if (fs::is_directory("/opt/ros/humble")) { check.ok = true; check.detail = "ROS 2 Humble is installed"; return check; }
  if (fs::is_directory("/opt/ros/jazzy"))  { check.ok = true; check.detail = "ROS 2 Jazzy is installed";  return check; }
  check.detail = "not installed, phase 17 is optional so this does not block you";
  return check;
}

Check probe_serial_group(const Host& host) {
  Check check;
  check.id = "serial-access";
  check.label = "permission to open serial devices";
  check.level = Level::RequiredLater;
  if (host.os != "linux") {
    check.ok = true;
    check.detail = "not applicable on this operating system";
    return check;
  }
  const CommandResult groups = run("id -nG");
  check.fix = "sudo usermod -aG dialout $USER, then log out and back in";
  if (contains(groups.output, "dialout")) {
    check.ok = true;
    check.detail = "you are in the dialout group";
  } else {
    check.detail = "you are not in the dialout group, needed from phase 08 onward";
  }
  return check;
}

std::string level_tag(Level level) {
  switch (level) {
    case Level::Required: return "required now";
    case Level::RequiredLater: return "required later";
    case Level::Optional: return "optional";
  }
  return "";
}

}  // namespace

int cmd_doctor(const Args& args) {
  const bool as_json = has_flag(args, "--json");
  const Host host = detect_host();

  std::vector<Check> checks;
  if (host.os == "linux") {
    checks.push_back(probe_program("compiler", "a C++20 compiler", "g++", "--version",
                                   Level::Required, apt_install("build-essential")));
    checks.push_back(probe_program("cmake", "CMake 3.22 or newer", "cmake", "--version",
                                   Level::Required, apt_install("cmake")));
    checks.push_back(probe_program("ninja", "Ninja build tool", "ninja", "--version",
                                   Level::Required, apt_install("ninja-build")));
    checks.push_back(probe_program("git", "Git", "git", "--version",
                                   Level::Required, apt_install("git")));
    checks.push_back(probe_program("debugger", "a debugger", "gdb", "--version",
                                   Level::RequiredLater, apt_install("gdb")));
  } else if (host.os == "windows") {
    checks.push_back(probe_program("compiler", "a C++20 compiler", "cl", "",
                                   Level::Required,
                                   "install Visual Studio 2022 with Desktop development with C++, then use the Developer Command Prompt"));
    checks.push_back(probe_program("cmake", "CMake 3.22 or newer", "cmake", "--version",
                                   Level::Required, "install CMake, or use the copy bundled with Visual Studio"));
    checks.push_back(probe_program("git", "Git", "git", "--version",
                                   Level::Required, "install Git for Windows"));
  } else {
    checks.push_back(probe_program("compiler", "a C++20 compiler", "clang++", "--version",
                                   Level::Required, "xcode-select --install"));
    checks.push_back(probe_program("cmake", "CMake 3.22 or newer", "cmake", "--version",
                                   Level::Required, "brew install cmake"));
    checks.push_back(probe_program("git", "Git", "git", "--version",
                                   Level::Required, "brew install git"));
  }
  checks.push_back(probe_cxx_features(host));
  checks.push_back(probe_qt(host));
  checks.push_back(probe_serial_group(host));
  checks.push_back(probe_ros2(host));

  int required_total = 0, required_passed = 0;
  for (const Check& c : checks) {
    if (c.level != Level::Required) continue;
    ++required_total;
    if (c.ok) ++required_passed;
  }
  const bool ready = required_passed == required_total;

  if (as_json) {
    json::Object root;
    root.emplace("os", json::Value(host.os));
    root.emplace("pretty", json::Value(host.pretty));
    root.emplace("distro", json::Value(host.distro));
    root.emplace("version_id", json::Value(host.version_id));
    root.emplace("wsl", json::Value(host.wsl));
    root.emplace("ready", json::Value(ready));
    json::Array items;
    for (const Check& c : checks) {
      json::Object item;
      item.emplace("id", json::Value(c.id));
      item.emplace("label", json::Value(c.label));
      item.emplace("level", json::Value(level_tag(c.level)));
      item.emplace("ok", json::Value(c.ok));
      item.emplace("detail", json::Value(c.detail));
      if (!c.ok) item.emplace("fix", json::Value(c.fix));
      items.push_back(json::Value(std::move(item)));
    }
    root.emplace("checks", json::Value(std::move(items)));
    std::cout << json::Value(std::move(root)).dump() << "\n";
    return ready ? 0 : 1;
  }

  std::cout << "\n" << style::bold("Machine check") << "\n";
  std::cout << style::dim("  " + host.pretty + (host.wsl ? " (running under WSL2)" : "")) << "\n\n";

  for (const Check& c : checks) {
    const std::string mark = c.ok ? style::pass("  ok  ") : style::fail(" miss ");
    std::cout << mark << " " << c.label << style::dim("  [" + level_tag(c.level) + "]") << "\n";
    std::cout << "        " << style::dim(c.detail) << "\n";
    if (!c.ok && !c.fix.empty())
      std::cout << "        " << style::warn("fix: ") << c.fix << "\n";
  }

  std::cout << "\n" << required_passed << "/" << required_total << " checks required right now passed\n";
  if (ready) {
    std::cout << style::pass("\nYou are ready to start.") << "\n";
    std::cout << "  Next: " << style::bold("rcpp start") << "\n\n";
  } else {
    std::cout << style::fail("\nNot ready yet.") << " Run each fix above, then run rcpp doctor again.\n\n";
  }
  return ready ? 0 : 1;
}

}  // namespace rcpp
