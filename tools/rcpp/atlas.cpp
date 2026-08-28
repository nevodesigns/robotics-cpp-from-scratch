#include "atlas.hpp"

#include <algorithm>
#include <regex>
#include <sstream>

namespace rcpp {
namespace {

// An atlas file is a block of "key: value" header lines, then a line holding
// exactly "---", then a markdown body with "## Symptom", "## Cause" and
// "## Fix" sections. Keys may repeat, which is how an entry lists several
// patterns.
void parse_entry(const std::string& text, AtlasEntry& entry) {
  std::istringstream in(text);
  std::string line;
  bool in_header = true;
  std::string section;
  std::ostringstream symptom, cause, fix;

  while (std::getline(in, line)) {
    if (in_header) {
      if (trim(line) == "---") { in_header = false; continue; }
      const std::size_t colon = line.find(':');
      if (colon == std::string::npos) continue;
      const std::string key = trim(line.substr(0, colon));
      const std::string value = trim(line.substr(colon + 1));
      if (key == "id") entry.id = value;
      else if (key == "title") entry.title = value;
      else if (key == "teaches") entry.teaches = value;
      else if (key == "match") entry.patterns.push_back(value);
      else if (key == "platforms") {
        for (const std::string& p : split(value, ',')) entry.platforms.push_back(trim(p));
      }
      continue;
    }

    if (starts_with(line, "## ")) { section = trim(line.substr(3)); continue; }
    if (section == "Symptom") symptom << line << "\n";
    else if (section == "Cause") cause << line << "\n";
    else if (section == "Fix") fix << line << "\n";
  }

  entry.symptom = trim(symptom.str());
  entry.cause = trim(cause.str());
  entry.fix = trim(fix.str());
}

}  // namespace

const AtlasEntry* Atlas::find(const std::string& id) const {
  for (const AtlasEntry& e : entries)
    if (e.id == id) return &e;
  return nullptr;
}

std::vector<const AtlasEntry*> Atlas::match(const std::string& error_text) const {
  std::vector<const AtlasEntry*> hits;
  for (const AtlasEntry& e : entries) {
    for (const std::string& pattern : e.patterns) {
      try {
        const std::regex re(pattern, std::regex::ECMAScript | std::regex::icase);
        if (std::regex_search(error_text, re)) { hits.push_back(&e); break; }
      } catch (const std::regex_error&) {
        // A malformed pattern is reported by audit, not by explain.
      }
    }
  }
  return hits;
}

Atlas load_atlas(const fs::path& repo_root) {
  Atlas atlas;
  const fs::path dir = repo_root / "atlas" / "errors";
  if (!fs::is_directory(dir)) {
    atlas.load_errors.push_back("no atlas/errors directory");
    return atlas;
  }

  std::vector<fs::path> files;
  for (const auto& entry : fs::directory_iterator(dir))
    if (entry.is_regular_file() && entry.path().extension() == ".md")
      files.push_back(entry.path());
  std::sort(files.begin(), files.end());

  for (const fs::path& file : files) {
    const auto text = read_file(file);
    if (!text) { atlas.load_errors.push_back("unreadable: " + file.string()); continue; }
    AtlasEntry entry;
    entry.path = file;
    parse_entry(*text, entry);
    if (entry.id.empty()) {
      atlas.load_errors.push_back("atlas entry has no id: " + file.filename().string());
      continue;
    }
    if (entry.patterns.empty())
      atlas.load_errors.push_back("atlas entry has no match pattern: " + entry.id);
    atlas.entries.push_back(std::move(entry));
  }
  return atlas;
}

}  // namespace rcpp
