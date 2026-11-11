// rc/ros/launch.hpp
//
// Bringing a graph of managed nodes up and down, from lesson 17-19, graduated.
//
// A start order from what each node needs (ties go to the one listed first, so the order depends on nothing but the list), refused with a reason for a repeated name, a missing need or a circle. A bring up that configures every node
// and only then activates every node; a required node that fails stops it and everything already brought up is taken down, newest first; an optional node that fails is left out with whatever needed it, and the rest comes up.
//
// Listed planner, controller, filter, driver, logger, viewer, the order was driver filter planner controller logger viewer. A required planner that would not configure left the log driver:configure filter:configure planner:configure
// filter:cleanup driver:cleanup. A required controller that would not activate rolled back through planner, filter and driver. An optional logger that would not configure left driver, filter and controller running and logger and plotter
// left out.

#ifndef RC_ROS_LAUNCH_HPP
#define RC_ROS_LAUNCH_HPP

#include <cstddef>
#include <deque>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>
#include <rc/ros/lifecycle.hpp>

namespace rc {
namespace ros {

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
  error.clear();
  std::map<std::string, std::size_t> index;
  for (std::size_t i = 0; i < specs.size(); ++i) {
    if (!index.insert({specs[i].name, i}).second) {
      error = "duplicate node: " + specs[i].name;
      return std::vector<std::string>();
    }
  }
  for (const NodeSpec& s : specs)
    for (const std::string& d : s.requires_nodes)
      if (!index.count(d)) {
        error = s.name + " needs " + d + ", which is not listed";
        return std::vector<std::string>();
      }

  std::vector<std::string> order;
  std::set<std::string> done;
  while (order.size() < specs.size()) {
    bool placed = false;
    for (const NodeSpec& s : specs) {
      if (done.count(s.name)) continue;
      bool ready = true;
      for (const std::string& d : s.requires_nodes)
        if (!done.count(d)) ready = false;
      if (!ready) continue;
      order.push_back(s.name);
      done.insert(s.name);
      placed = true;
      break;
    }
    if (!placed) {
      error = "the needs of the nodes go round in a circle";
      return std::vector<std::string>();
    }
  }
  return order;
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
    LaunchReport report;
    order_ = start_order(specs_, report.error);
    if (!report.error.empty()) return report;
    std::set<std::string> unavailable;

    for (int phase = 0; phase < 2; ++phase) {
      for (const std::string& name : order_) {
        const NodeSpec& spec = specs_[position(name)];
        if (unavailable.count(name)) continue;
        bool blocked = false;
        for (const std::string& d : spec.requires_nodes)
          if (unavailable.count(d)) blocked = true;
        const Transition t = phase == 0 ? Transition::configure : Transition::activate;
        const LifeState wanted = phase == 0 ? LifeState::inactive : LifeState::active;
        bool good = !blocked;
        if (good) {
          report.log.push_back(name + ":" + (phase == 0 ? "configure" : "activate"));
          const rc::ros::TransitionResult r = node(name).trigger(t);
          good = r.legal && r.state == wanted;
        }
        if (good) continue;
        if (spec.required) {
          report.failed = name;
          roll_back(report);
          report.ok = false;
          return report;
        }
        unavailable.insert(name);
        // The node may be half up: take it back down.
        take_down(name, report);
      }
    }
    for (const std::string& name : order_) {
      if (unavailable.count(name)) report.skipped.push_back(name);
      else report.running.push_back(name);
    }
    report.ok = true;
    return report;
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
    LifecycleNode& n = node(name);
    if (n.state() == LifeState::active) {
      report.log.push_back(name + ":deactivate");
      n.trigger(Transition::deactivate);
    }
    if (n.state() == LifeState::inactive) {
      report.log.push_back(name + ":cleanup");
      n.trigger(Transition::cleanup);
    }
  }

  void roll_back(LaunchReport& report) {
    for (std::size_t i = order_.size(); i > 0; --i) {
      const std::string& name = order_[i - 1];
      take_down(name, report);
    }
  }

  std::vector<NodeSpec> specs_;
  std::deque<LifecycleNode> nodes_;
  std::vector<std::string> order_;
};

}  // namespace ros
}  // namespace rc

#endif  // RC_ROS_LAUNCH_HPP
