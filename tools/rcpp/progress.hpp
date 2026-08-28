// tools/rcpp/progress.hpp
//
// What the learner has actually passed.
//
// Progress is recorded only by rcpp verify, and only when the real test suite
// passed against the learner's own code. There is no way to mark a lesson done
// by asserting it, which is the same principle the rest of the repository runs
// on: the tests decide.
//
// The file is local and gitignored. It belongs to the person learning, not to
// the repository.

#ifndef RCPP_PROGRESS_HPP
#define RCPP_PROGRESS_HPP

#include <set>
#include <string>

#include "util.hpp"

namespace rcpp {

struct Progress {
  std::set<std::string> passed;

  bool has(const std::string& lesson_id) const { return passed.count(lesson_id) > 0; }
};

Progress load_progress(const fs::path& repo_root);
bool record_pass(const fs::path& repo_root, const std::string& lesson_id);

}  // namespace rcpp

#endif  // RCPP_PROGRESS_HPP
