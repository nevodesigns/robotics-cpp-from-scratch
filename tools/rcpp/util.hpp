// tools/rcpp/util.hpp
// Small helpers shared by every rcpp subcommand.

#ifndef RCPP_UTIL_HPP
#define RCPP_UTIL_HPP

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace rcpp {

namespace fs = std::filesystem;

std::optional<std::string> read_file(const fs::path& path);
bool write_file(const fs::path& path, const std::string& contents);

// Runs a command and captures its standard output. Standard error is folded in
// so that version banners printed to stderr are still seen.
struct CommandResult {
  bool ran = false;
  int exit_code = -1;
  std::string output;
};
CommandResult run(const std::string& command);

bool on_path(const std::string& program);

// Whether a Qt 6 development installation can be found at all. Deliberately
// cheap and deliberately not the whole story: doctor asks the fuller question,
// including whether the OpenGL headers Qt6 Widgets needs are present. This is
// the version verify needs, to tell "you have not written it yet" apart from
// "this lesson could not be built on this machine".
bool qt6_present();
std::string first_line(const std::string& text);
std::string trim(const std::string& text);
std::vector<std::string> split(const std::string& text, char separator);
bool starts_with(const std::string& text, const std::string& prefix);
bool contains(const std::string& haystack, const std::string& needle);

// Walks upward from the current directory looking for the repository root,
// which is the directory holding both CMakeLists.txt and phases/.
std::optional<fs::path> find_repo_root();

namespace style {
// Colour is used only when standard output is a terminal, so piping rcpp into
// a file or into CI logs produces clean text.
bool colour_enabled();
std::string pass(const std::string& text);
std::string fail(const std::string& text);
std::string warn(const std::string& text);
std::string dim(const std::string& text);
std::string bold(const std::string& text);
}  // namespace style

}  // namespace rcpp

#endif  // RCPP_UTIL_HPP
