#include "util.hpp"

#include <array>
#include <cstdio>
#include <fstream>
#include <sstream>

#if defined(_WIN32)
#  include <io.h>
#  define RCPP_POPEN _popen
#  define RCPP_PCLOSE _pclose
#  define RCPP_ISATTY _isatty
#  define RCPP_FILENO _fileno
#else
#  include <unistd.h>
#  define RCPP_POPEN popen
#  define RCPP_PCLOSE pclose
#  define RCPP_ISATTY isatty
#  define RCPP_FILENO fileno
#endif

namespace rcpp {

std::optional<std::string> read_file(const fs::path& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) return std::nullopt;
  std::ostringstream buffer;
  buffer << in.rdbuf();
  return buffer.str();
}

bool write_file(const fs::path& path, const std::string& contents) {
  std::error_code ec;
  fs::create_directories(path.parent_path(), ec);
  std::ofstream out(path, std::ios::binary);
  if (!out) return false;
  out << contents;
  return out.good();
}

CommandResult run(const std::string& command) {
  CommandResult result;
  const std::string full = command + " 2>&1";
  FILE* pipe = RCPP_POPEN(full.c_str(), "r");
  if (!pipe) return result;
  result.ran = true;
  std::array<char, 512> buffer{};
  while (std::fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr)
    result.output += buffer.data();
  const int status = RCPP_PCLOSE(pipe);
#if defined(_WIN32)
  result.exit_code = status;
#else
  result.exit_code = (status == -1) ? -1 : (status >> 8) & 0xFF;
#endif
  return result;
}

bool on_path(const std::string& program) {
#if defined(_WIN32)
  const CommandResult r = run("where " + program);
#else
  const CommandResult r = run("command -v " + program);
#endif
  return r.ran && r.exit_code == 0 && !trim(r.output).empty();
}

std::string first_line(const std::string& text) {
  const std::size_t nl = text.find('\n');
  return trim(nl == std::string::npos ? text : text.substr(0, nl));
}

std::string trim(const std::string& text) {
  const std::size_t begin = text.find_first_not_of(" \t\r\n");
  if (begin == std::string::npos) return "";
  const std::size_t end = text.find_last_not_of(" \t\r\n");
  return text.substr(begin, end - begin + 1);
}

std::vector<std::string> split(const std::string& text, char separator) {
  std::vector<std::string> parts;
  std::string current;
  std::istringstream in(text);
  while (std::getline(in, current, separator)) parts.push_back(current);
  return parts;
}

bool starts_with(const std::string& text, const std::string& prefix) {
  return text.size() >= prefix.size() && text.compare(0, prefix.size(), prefix) == 0;
}

bool contains(const std::string& haystack, const std::string& needle) {
  return haystack.find(needle) != std::string::npos;
}

std::optional<fs::path> find_repo_root() {
  std::error_code ec;
  fs::path here = fs::current_path(ec);
  if (ec) return std::nullopt;
  for (int depth = 0; depth < 12; ++depth) {
    if (fs::exists(here / "CMakeLists.txt") && fs::is_directory(here / "phases"))
      return here;
    if (!here.has_parent_path() || here.parent_path() == here) break;
    here = here.parent_path();
  }
  return std::nullopt;
}

namespace style {

bool colour_enabled() {
  static const bool enabled = RCPP_ISATTY(RCPP_FILENO(stdout)) != 0;
  return enabled;
}

static std::string wrap(const char* code, const std::string& text) {
  if (!colour_enabled()) return text;
  return std::string("\033[") + code + "m" + text + "\033[0m";
}

std::string pass(const std::string& text) { return wrap("32", text); }
std::string fail(const std::string& text) { return wrap("31", text); }
std::string warn(const std::string& text) { return wrap("33", text); }
std::string dim(const std::string& text) { return wrap("90", text); }
std::string bold(const std::string& text) { return wrap("1", text); }

}  // namespace style

}  // namespace rcpp
