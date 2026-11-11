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
    std::vector<SupervisorEvent> events;
    std::vector<std::string> stalled_now;
    for (Watch& w : watches_)
      if (!w.dead && w.monitor.check(now) == 1) stalled_now.push_back(w.node);

    for (Watch& w : watches_) {
      bool is_stalled = false;
      for (const std::string& n : stalled_now)
        if (n == w.node) is_stalled = true;
      if (!is_stalled) continue;

      bool starved = false;
      for (const std::string& up : w.upstream) {
        for (const std::string& n : stalled_now)
          if (n == up) starved = true;
        for (const Watch& other : watches_)
          if (other.node == up && other.dead) starved = true;
      }
      if (starved) {
        events.push_back(event(now, SupervisorEvent::Kind::starved, w.node));
        w.monitor.on_message(now);
        continue;
      }

      events.push_back(event(now, SupervisorEvent::Kind::stalled, w.node));
      if (w.restarts >= max_restarts_) {
        events.push_back(event(now, SupervisorEvent::Kind::gave_up, w.node));
        w.dead = true;
        if (w.required) gave_up_ = true;
        continue;
      }
      ++w.restarts;
      const bool up = restart(*w.life);
      events.push_back(event(now, up ? SupervisorEvent::Kind::restarted : SupervisorEvent::Kind::restart_failed, w.node));
      w.monitor.on_message(now);
    }
    return events;
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
    if (n.state() == LifeState::active) n.trigger(Transition::deactivate);
    if (n.state() == LifeState::inactive) n.trigger(Transition::cleanup);
    if (n.state() == LifeState::unconfigured) n.trigger(Transition::configure);
    if (n.state() == LifeState::inactive) n.trigger(Transition::activate);
    return n.state() == LifeState::active;
  }

  int max_restarts_ = 0;
  bool gave_up_ = false;
  std::vector<Watch> watches_;
};

#endif  // LESSON_SOLUTION_HPP
