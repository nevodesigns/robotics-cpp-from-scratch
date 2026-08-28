#include "progress.hpp"

#include <sstream>

#include "json.hpp"

namespace rcpp {
namespace {

fs::path progress_path(const fs::path& repo_root) {
  return repo_root / ".rcpp-progress.json";
}

}  // namespace

Progress load_progress(const fs::path& repo_root) {
  Progress progress;
  const auto text = read_file(progress_path(repo_root));
  if (!text) return progress;

  const json::ParseResult parsed = json::parse(*text);
  if (!parsed.ok) return progress;   // a damaged file means no progress, not a crash

  for (const std::string& id : parsed.value.at("passed").string_list())
    progress.passed.insert(id);
  return progress;
}

bool record_pass(const fs::path& repo_root, const std::string& lesson_id) {
  Progress progress = load_progress(repo_root);
  if (!progress.passed.insert(lesson_id).second) return true;   // already recorded

  json::Array passed;
  for (const std::string& id : progress.passed) passed.push_back(json::Value(id));

  json::Object root;
  root.emplace("schema", json::Value(1.0));
  root.emplace("passed", json::Value(std::move(passed)));

  return write_file(progress_path(repo_root), json::Value(std::move(root)).dump() + "\n");
}

}  // namespace rcpp
