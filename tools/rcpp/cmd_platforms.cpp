// rcpp platforms
//
// The toolchains this curriculum claims, read from platforms.json.
//
//   rcpp platforms            human readable, with what each lane provides
//   rcpp platforms --matrix   the continuous integration matrix, as JSON
//
// The matrix is generated rather than written by hand for the same reason the
// README counts are: two lists of the same thing drift, and here the drift is
// dangerous rather than merely untidy. A lesson claiming a lane that does not
// exist, or a Qt lesson claiming a lane without Qt, is silently skipped, and a
// skip looks exactly like a pass.

#include <iostream>

#include "commands.hpp"
#include "json.hpp"
#include "util.hpp"

namespace rcpp {

int cmd_platforms(const Args& args) {
  const auto root = find_repo_root();
  if (!root) { std::cerr << "rcpp platforms: run this from inside the repository\n"; return 2; }

  const auto text = read_file(*root / "platforms.json");
  if (!text) { std::cerr << "rcpp platforms: cannot read platforms.json\n"; return 2; }

  const json::ParseResult parsed = json::parse(*text);
  if (!parsed.ok) {
    std::cerr << "rcpp platforms: platforms.json line " << parsed.line << ": " << parsed.error << "\n";
    return 2;
  }

  const json::Array& toolchains = parsed.value.at("toolchains").as_array();

  if (has_flag(args, "--matrix")) {
    json::Array include;
    for (const json::Value& toolchain : toolchains) {
      json::Object entry;
      entry.emplace("name", json::Value(toolchain.at("name").as_string_or("")));
      entry.emplace("os", json::Value(toolchain.at("runner").as_string_or("")));
      entry.emplace("cc", json::Value(toolchain.at("cc").as_string_or("")));
      entry.emplace("cxx", json::Value(toolchain.at("cxx").as_string_or("")));
      entry.emplace("platform_id", json::Value(toolchain.at("id").as_string_or("")));
      entry.emplace("qt", json::Value(toolchain.at("qt").as_bool(false)));
      include.push_back(json::Value(std::move(entry)));
    }
    json::Object matrix;
    matrix.emplace("include", json::Value(std::move(include)));

    // One line, because a workflow output cannot span several.
    std::cout << json::Value(std::move(matrix)).dump(0) << "\n";
    return 0;
  }

  std::cout << "\n" << style::bold("Toolchains") << "\n\n";
  for (const json::Value& toolchain : toolchains) {
    const bool has_qt = toolchain.at("qt").as_bool(false);
    std::cout << "  " << style::pass(toolchain.at("id").as_string_or("")) << "\n";
    std::cout << "      " << style::dim("runner " + toolchain.at("runner").as_string_or("") +
                                        ", Qt " + (has_qt ? "installed" : "not installed"))
              << "\n";
    const std::string note = toolchain.at("note").as_string_or("");
    if (!note.empty()) std::cout << "      " << style::dim(note) << "\n";
    std::cout << "\n";
  }
  std::cout << "  " << toolchains.size() << " toolchain(s). Lessons may only claim these,\n"
            << "  and a Qt lesson may only claim one where Qt is installed.\n\n";
  return 0;
}

}  // namespace rcpp
