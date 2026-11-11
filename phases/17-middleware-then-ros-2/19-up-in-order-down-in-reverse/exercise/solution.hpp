#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cstddef>
#include <deque>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <rc/ros/lifecycle.hpp>

using rc::ros::CallbackResult;
using rc::ros::LifeState;
using rc::ros::LifecycleNode;
using rc::ros::Transition;

// What one node of the graph needs from the others.
struct NodeSpec {
  std::string name;
  std::vector<std::string> requires_nodes;   // nodes that must be up before this one is configured
  bool required = true;                      // whether the graph is worth running without it
};

// The order to start nodes in: every node after all that it needs. Of those that could go next, the one listed first goes first, so
// the order does not depend on anything but the list. An empty order, with the reason in `error`, if a name is repeated, a node needs
// one that is not listed, or the needs go round in a circle.
inline std::vector<std::string> start_order(const std::vector<NodeSpec>& specs, std::string& error) {
  // TODO: Clear `error`. A name listed twice: error "duplicate node: NAME" and an empty result. A node whose need is not in the list: error "X needs Y, which is not listed" and an empty result.
  // Then repeatedly pick, from the list in its listed order, the FIRST node not yet placed whose needs have all been placed already, and place it.
  // If a round places nothing, the needs go round in a circle: error "the needs of the nodes go round in a circle" and an empty result.
  (void)specs;
  (void)error;
  return std::vector<std::string>();
}

struct LaunchReport {
  bool ok = false;
  std::string failed;                 // the required node that stopped the bring up, if one did
  std::string error;                  // a problem with the list itself
  std::vector<std::string> running;   // nodes that are active
  std::vector<std::string> skipped;   // optional nodes left out, and those that needed them
  std::vector<std::string> log;       // every transition asked of every node, as "name:transition", in order
};

// Brings a graph of managed nodes up and down.
class Launcher {
 public:
  // Add a node. The reference it returns is where to set its callbacks.
  LifecycleNode& add(const std::string& name, const std::vector<std::string>& needs, bool required) {
    NodeSpec s;
    s.name = name;
    s.requires_nodes = needs;
    s.required = required;
    specs_.push_back(s);
    nodes_.emplace_back();
    return nodes_.back();
  }

  // Configure every node in order, and only then activate every node in order. A node that needs one that was skipped is skipped.
  // A required node that fails to configure or to activate stops the whole bring up, and every node already brought up is taken down again, newest first.
  LaunchReport bring_up() {
    // TODO: order_ = start_order(specs_, report.error); give up with the error if there is one.
    // Two passes over the order: first configure every node, then activate every node. For each node: skip it if it is already unavailable; it is blocked if any node it needs is unavailable.
    // Otherwise log "name:configure" or "name:activate" in report.log and trigger the transition; it went well if the transition was legal and the node is now inactive (configure) or active (activate).
    // A node that did not go well: if it is required, set report.failed to its name, call roll_back(report), set ok to false and return. If it is optional, mark it unavailable and take_down() whatever is half up.
    // When both passes are done, every node in order goes to report.skipped if it is unavailable and report.running if not; ok is true.
    (void)specs_;
    (void)nodes_;
    (void)order_;
    return LaunchReport();
  }

  // Take every node down: the newest first.
  LaunchReport shut_down() {
    LaunchReport report;
    report.ok = true;
    roll_back(report);
    return report;
  }

  LifeState state_of(const std::string& name) { return node(name).state(); }

 private:
  std::size_t position(const std::string& name) const {
    for (std::size_t i = 0; i < specs_.size(); ++i)
      if (specs_[i].name == name) return i;
    return 0;
  }
  LifecycleNode& node(const std::string& name) { return nodes_[position(name)]; }

  void take_down(const std::string& name, LaunchReport& report) {
    // TODO: if the node is active, log "name:deactivate" and trigger deactivate; then if it is inactive, log "name:cleanup" and trigger cleanup
    (void)name;
    (void)report;
  }

  void roll_back(LaunchReport& report) {
    // TODO: take_down() every node in the order, last first
    (void)report;
  }

  std::vector<NodeSpec> specs_;
  std::deque<LifecycleNode> nodes_;
  std::vector<std::string> order_;
};

#endif  // LESSON_SOLUTION_HPP
