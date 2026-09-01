#include "atlas.hpp"

#include <algorithm>
#include <cstring>
#include <cstddef>
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

namespace {

// GCC quotes names with typographic marks in a UTF-8 locale: the bytes are
// U+2018 and U+2019, not the ASCII apostrophe. Patterns in the atlas are
// written with ASCII quotes, because that is what a person types, so text
// arriving from a real build has to be brought to the same alphabet or none of
// the seventeen patterns containing a quote can ever match.
//
// This was found by piping a real build into rcpp explain, which is the use the
// help text advertises, and getting nothing back.
std::string normalise_quotes(const std::string& text) {
  static const struct { const char* from; char to; } kMarks[] = {
      {"\xE2\x80\x98", '\''},   // U+2018 left single quotation mark
      {"\xE2\x80\x99", '\''},   // U+2019 right single quotation mark
      {"\xE2\x80\x9C", '"'},    // U+201C left double quotation mark
      {"\xE2\x80\x9D", '"'},    // U+201D right double quotation mark
  };

  std::string out;
  out.reserve(text.size());
  for (std::size_t i = 0; i < text.size();) {
    bool replaced = false;
    for (const auto& mark : kMarks) {
      const std::size_t width = std::strlen(mark.from);
      if (text.compare(i, width, mark.from) == 0) {
        out += mark.to;
        i += width;
        replaced = true;
        break;
      }
    }
    if (!replaced) out += text[i++];
  }
  return out;
}

}  // namespace

const AtlasEntry* Atlas::find(const std::string& id) const {
  for (const AtlasEntry& e : entries)
    if (e.id == id) return &e;
  return nullptr;
}

std::vector<const AtlasEntry*> Atlas::match(const std::string& raw_error_text) const {
  const std::string error_text = normalise_quotes(raw_error_text);

  // Ranked by how much of the error text each entry actually accounted for.
  // Some patterns are necessarily broad: "expected .* to be within" matches
  // every near comparison in the curriculum. Unranked, an entry that matched a
  // whole sentence is printed after one that matched four generic words,
  // because the order was alphabetical by filename. The learner reads the
  // first entry, so the first entry has to be the specific one.
  struct Hit {
    const AtlasEntry* entry;
    std::size_t matched;
    std::size_t order;
  };
  std::vector<Hit> hits;

  for (std::size_t i = 0; i < entries.size(); ++i) {
    const AtlasEntry& e = entries[i];
    std::size_t longest = 0;
    bool any = false;

    // Every pattern is tried, not just the first that matches, because the
    // longest match is what decides the rank.
    for (const std::string& pattern : e.patterns) {
      try {
        const std::regex re(pattern, std::regex::ECMAScript | std::regex::icase);
        std::smatch found;
        if (std::regex_search(error_text, found, re)) {
          any = true;
          const std::size_t length = static_cast<std::size_t>(found.length(0));
          if (length > longest) longest = length;
        }
      } catch (const std::regex_error&) {
        // A malformed pattern is reported by audit, not by explain.
      }
    }

    if (any) hits.push_back(Hit{&e, longest, i});
  }

  std::sort(hits.begin(), hits.end(), [](const Hit& a, const Hit& b) {
    if (a.matched != b.matched) return a.matched > b.matched;
    return a.order < b.order;   // file order breaks a tie, so output is stable
  });

  std::vector<const AtlasEntry*> ranked;
  ranked.reserve(hits.size());
  for (const Hit& hit : hits) ranked.push_back(hit.entry);
  return ranked;
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
