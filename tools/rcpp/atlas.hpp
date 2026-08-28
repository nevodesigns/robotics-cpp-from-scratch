// tools/rcpp/atlas.hpp
//
// The Failure Atlas. One file per catalogued failure, so that a learner who
// pastes a wall of compiler output gets the cause, the fix, and the lesson that
// explains why it happened.

#ifndef RCPP_ATLAS_HPP
#define RCPP_ATLAS_HPP

#include <string>
#include <vector>

#include "util.hpp"

namespace rcpp {

struct AtlasEntry {
  std::string id;         // E-QT-0012
  std::string title;
  std::string teaches;    // lesson id
  std::vector<std::string> patterns;   // regular expressions, matched case insensitively
  std::vector<std::string> platforms;  // linux, windows, all
  std::string symptom;
  std::string cause;
  std::string fix;
  fs::path path;
};

struct Atlas {
  std::vector<AtlasEntry> entries;
  std::vector<std::string> load_errors;

  const AtlasEntry* find(const std::string& id) const;
  std::vector<const AtlasEntry*> match(const std::string& error_text) const;
};

Atlas load_atlas(const fs::path& repo_root);

}  // namespace rcpp

#endif  // RCPP_ATLAS_HPP
