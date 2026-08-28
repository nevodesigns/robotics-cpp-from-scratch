// rcpp explain
//
// Paste a compiler, linker, CMake or Qt error and get the cause, the fix, and
// the lesson that explains why it happened. This is the command that keeps a
// beginner in the curriculum at eleven at night.

#include <iostream>
#include <sstream>

#include "atlas.hpp"
#include "catalog.hpp"
#include "commands.hpp"
#include "util.hpp"

namespace rcpp {

int cmd_explain(const Args& args) {
  const auto root = find_repo_root();
  if (!root) {
    std::cerr << "rcpp explain: run this from inside the repository\n";
    return 2;
  }

  std::string text;
  const std::string file = flag_value(args, "--file", "");
  if (!file.empty()) {
    const auto contents = read_file(file);
    if (!contents) { std::cerr << "rcpp explain: cannot read " << file << "\n"; return 2; }
    text = *contents;
  } else {
    std::ostringstream joined;
    for (const std::string& a : args)
      if (!starts_with(a, "--")) joined << a << " ";
    text = trim(joined.str());
    if (text.empty()) {
      std::cout << style::dim("Paste the error, then press Ctrl+D:\n");
      std::ostringstream buffer;
      buffer << std::cin.rdbuf();
      text = buffer.str();
    }
  }

  if (trim(text).empty()) {
    std::cerr << "rcpp explain: nothing to look up\n";
    return 2;
  }

  const Atlas atlas = load_atlas(*root);
  const auto hits = atlas.match(text);

  if (hits.empty()) {
    std::cout << "\n" << style::warn("No catalogued match.") << "\n\n"
              << "  The Failure Atlas holds " << atlas.entries.size() << " entries and does not\n"
              << "  recognise this one yet. Two things worth doing:\n\n"
              << "    1. Read the first error, not the last. C++ errors cascade,\n"
              << "       and everything after the first is usually noise.\n"
              << "    2. Open an issue with the exact text. Entries come from real\n"
              << "       errors people hit, which is the only way this stays useful.\n\n";
    return 1;
  }

  const Catalog catalog = load_catalog(*root);
  for (const AtlasEntry* entry : hits) {
    std::cout << "\n" << style::bold(entry->id + "  " + entry->title) << "\n\n";
    if (!entry->symptom.empty()) std::cout << style::dim("  What you see\n") << "  " << entry->symptom << "\n\n";
    if (!entry->cause.empty())   std::cout << style::dim("  Why it happens\n") << "  " << entry->cause << "\n\n";
    if (!entry->fix.empty())     std::cout << style::warn("  How to fix it\n") << "  " << entry->fix << "\n\n";
    if (!entry->teaches.empty()) {
      const Lesson* lesson = catalog.find(entry->teaches);
      std::cout << style::dim("  Taught in\n") << "  " << entry->teaches;
      if (lesson) std::cout << "  (" << lesson->rel_path << ")";
      std::cout << "\n\n";
    }
  }
  return 0;
}

}  // namespace rcpp
