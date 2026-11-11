#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cstddef>
#include <map>
#include <string>
#include <vector>

#include <rc/bus/staleness.hpp>
#include <rc/ros/lifecycle.hpp>

using rc::bus::DeadlineMonitor;
using rc::ros::LifeState;
using rc::ros::LifecycleNode;
using rc::ros::Transition;

struct SupervisorEvent {
  enum class Kind { stalled, starved, restarted, restart_failed, gave_up };
  double time = 0.0;
  Kind kind = Kind::stalled;
  std::string node;
};

// Looks after a graph of managed nodes while it runs. Each watched node has a deadline: its output must be seen at least that often.
// One that goes quiet is stalled, and the supervisor takes it down and brings it up again through its life cycle. A node that has been
// restarted `max_restarts` times and stalls again is given up on, and if it was required the supervisor says the graph cannot be trusted.
//
// Silence spreads down a graph: when the sensor stops, the nodes that listen to it stop too, and they look just as stalled. Restarting them
// would be restarting the victims. Each node is told which watched nodes it listens to, and one whose upstream has stalled in the same look
// is `starved` and left alone: it gets a fresh deadline, and is only blamed if it is still silent after its upstream has been dealt with.
class Supervisor {
 public:
  explicit Supervisor(int max_restarts) : max_restarts_(max_restarts < 0 ? 0 : max_restarts) {}

  void watch(const std::string& node, LifecycleNode* life, double deadline, bool required, const std::vector<std::string>& upstream) {
    Watch w{node, life, DeadlineMonitor(deadline), required, 0, false, upstream};
    watches_.push_back(w);
  }

  // The node produced output at `now`.
  void on_output(const std::string& node, double now) {
    for (Watch& w : watches_)
      if (w.node == node && !w.dead) w.monitor.on_message(now);
  }

  // Look at every node at time `now`. Those whose deadline has just been missed are the stalled ones. Of them:
  //   - one whose upstream (a node it was told it listens to) is also stalled, or has been given up on, is reported `starved`, not restarted,
  //     and given a fresh deadline;
  //   - one that has already been restarted the allowed number of times is reported `stalled` and then `gave_up`, and not watched again;
  //     a required one makes gave_up() true;
  //   - any other is reported `stalled` and restarted: `restarted` if it ends up active again, `restart_failed` if not. Either way it is given a
  //     fresh deadline from now, so that a restart that does not work is noticed one deadline later.
  // Returns the events of this tick, in order.
  std::vector<SupervisorEvent> tick(double now) {
    // TODO: First find the stalled nodes: every watch not dead whose monitor says a deadline was just missed (check(now) == 1). Then, for each of them in the order added:
    // - if any node in its `upstream` list is also stalled in this look, or is dead (given up on): it is a victim. Report `starved`, give its monitor a fresh start (on_message(now)) and go on to the next. Do not restart it.
    // - otherwise report `stalled` (use event(now, kind, node)). If it has already been restarted max_restarts_ times: report `gave_up`, mark the watch dead, set gave_up_ if it is required, and go on.
    // - otherwise add one to its restarts, restart() its life cycle node, report `restarted` if that returned true and `restart_failed` if not, and give its monitor a fresh start.
    // Return the events of this call, in order.
    (void)now;
    (void)max_restarts_;
    (void)gave_up_;
    (void)watches_;
    return std::vector<SupervisorEvent>();
  }

  bool gave_up() const { return gave_up_; }

  int restarts(const std::string& node) const {
    for (const Watch& w : watches_)
      if (w.node == node) return w.restarts;
    return 0;
  }

 private:
  struct Watch {
    std::string node;
    LifecycleNode* life = nullptr;
    DeadlineMonitor monitor;
    bool required = true;
    int restarts = 0;
    bool dead = false;
    std::vector<std::string> upstream;
  };

  static SupervisorEvent event(double time, SupervisorEvent::Kind kind, const std::string& node) {
    SupervisorEvent e;
    e.time = time;
    e.kind = kind;
    e.node = node;
    return e;
  }

  // Take the node down as far as it goes and bring it up again. True if it ends active.
  static bool restart(LifecycleNode& n) {
    // TODO: if the node is active, trigger deactivate; then if it is inactive, trigger cleanup; then if it is unconfigured, trigger configure; then if it is inactive, trigger activate.
    // True if it is active at the end
    (void)n;
    return false;
  }

  int max_restarts_ = 0;
  bool gave_up_ = false;
  std::vector<Watch> watches_;
};

#endif  // LESSON_SOLUTION_HPP
