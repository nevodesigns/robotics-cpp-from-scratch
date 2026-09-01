// rc/core/build_graph.hpp
//
// The build graph from lesson 04-01, graduated.
//
// What every build system is underneath: targets, what each depends on, and an
// order to visit them in that never visits a thing before what it needs. The
// cycle detection is the part worth having written once, because the error a
// real build system gives you for a cycle makes sense only if you know what it
// was looking for.

#ifndef RC_CORE_BUILD_GRAPH_HPP
#define RC_CORE_BUILD_GRAPH_HPP

#include <map>
#include <set>
#include <string>
#include <vector>
#include <rc/core/compat.hpp>

namespace rc {
namespace core {

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
  rc::expected<std::vector<std::string>, GraphError> order() const {
    // Every named dependency must exist. Reporting this before sorting gives a
    // useful error rather than a cycle complaint about a target that is simply
    // absent, which sends people looking in the wrong place.
    for (const auto& entry : targets_) {
      for (const std::string& needed : entry.second.depends_on) {
        if (!contains(needed)) return rc::unexpected(GraphError::UnknownDependency);
      }
    }

    std::vector<std::string> ordered;
    std::set<std::string> done;

    // Repeatedly take any target whose dependencies are all satisfied. Sorted
    // iteration over a std::map makes the result deterministic, which matters
    // more than it looks: a build order that varies between runs makes a
    // failure hard to reproduce.
    while (ordered.size() < targets_.size()) {
      bool progressed = false;

      for (const auto& entry : targets_) {
        const std::string& name = entry.first;
        if (done.count(name) > 0) continue;

        bool ready = true;
        for (const std::string& needed : entry.second.depends_on) {
          if (done.count(needed) == 0) ready = false;
        }
        if (!ready) continue;

        ordered.push_back(name);
        done.insert(name);
        progressed = true;
      }

      // Nothing became ready and work remains, so every remaining target is
      // waiting on another remaining target. That is a cycle.
      if (!progressed) return rc::unexpected(GraphError::Cycle);
    }
    return ordered;
  }

  // The targets needing work, in build order.
  std::vector<std::string> stale(const FileTimes& times) const {
    std::vector<std::string> needs_work;

    const auto ordering = order();
    if (!ordering.has_value()) return needs_work;

    std::set<std::string> already_stale;

    // Walking in build order is what makes the third clause cheap: by the time
    // a target is considered, every target it depends on has been decided.
    for (const std::string& name : ordering.value()) {
      const Target& target = targets_.at(name);

      bool is_stale = false;

      // Clause one: no output yet.
      const auto output_time = times.find(target.output);
      if (target.output.empty() || output_time == times.end()) {
        is_stale = true;
      } else {
        // Clause two: an input is newer than the output.
        for (const std::string& input : target.inputs) {
          const auto input_time = times.find(input);
          if (input_time != times.end() && input_time->second > output_time->second) is_stale = true;
        }
      }

      // Clause three, the one people forget: a dependency is stale, so this is
      // too. Without it a header change rebuilds one object and nothing else,
      // and the program is quietly built from mismatched pieces.
      for (const std::string& needed : target.depends_on) {
        if (already_stale.count(needed) > 0) is_stale = true;
      }

      if (is_stale) {
        needs_work.push_back(name);
        already_stale.insert(name);
      }
    }
    return needs_work;
  }

 private:
  std::map<std::string, Target> targets_;
};

}  // namespace core
}  // namespace rc

#endif  // RC_CORE_BUILD_GRAPH_HPP
