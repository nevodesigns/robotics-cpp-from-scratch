#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <vector>

#include <rc/core/compat.hpp>

enum class GraphError {
  UnknownDependency,
  Cycle,
};

struct Target {
  std::string name;
  std::vector<std::string> depends_on;   // other targets
  std::vector<std::string> inputs;       // source files this target reads
  std::string output;                    // the file this target produces
};

// When each file was last written. A file absent from the table does not exist.
using FileTimes = std::map<std::string, long>;

class BuildGraph {
 public:
  void add(Target target) { targets_[target.name] = std::move(target); }

  bool contains(const std::string& name) const { return targets_.count(name) > 0; }
  std::size_t size() const { return targets_.size(); }

  // A topological order: every target appears after everything it depends on.
  //
  // Report UnknownDependency when a target names something that was never
  // added, and Cycle when no valid order exists. Check for the unknown one
  // first, or an absent target is reported as a cycle and sends the reader
  // looking in the wrong place.
  rc::expected<std::vector<std::string>, GraphError> order() const {
    // TODO
    // Repeatedly take any target whose dependencies have all been taken. If a
    // pass takes nothing and targets remain, everything left is waiting on
    // something else that is left, which is a cycle.
    return std::vector<std::string>{};
  }

  // The targets needing work, in build order.
  //
  // A target is stale when any of these hold:
  //   1. its output does not exist in the times table
  //   2. one of its inputs is newer than its output
  //   3. a target it depends on is stale
  //
  // The third is the one people forget, and it is what makes one header change
  // rebuild everything downstream of it.
  std::vector<std::string> stale(const FileTimes& times) const {
    // TODO. Walking in build order makes the third clause easy: by the time a
    // target is considered, everything it depends on has already been decided.
    (void)times;
    return {};
  }

 private:
  std::map<std::string, Target> targets_;
};

#endif  // LESSON_SOLUTION_HPP
