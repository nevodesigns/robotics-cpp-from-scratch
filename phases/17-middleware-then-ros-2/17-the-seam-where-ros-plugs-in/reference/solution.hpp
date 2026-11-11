#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

#include <rc/bus/names.hpp>
#include <rc/bus/sim_time.hpp>
#include <rc/bus/topic_bus.hpp>

using rc::bus::SimClock;
using rc::bus::SimTimer;
using rc::bus::TopicBus;
using rc::bus::resolve_name;

// What a component is allowed to ask of its node. Component code takes a Node& and nothing else: it never names a ROS 2 type, a bus or a clock,
// so the same code runs in a test with no ROS 2 installed, and a ROS 2 adapter is one class that implements this.
class Node {
 public:
  virtual ~Node() {}
  // A function that publishes to `topic`, resolved against this node's namespace and name.
  virtual std::function<void(const std::string&)> create_publisher(const std::string& topic) = 0;
  virtual void create_subscription(const std::string& topic, std::function<void(const std::string&)> handler) = 0;
  // Call `callback` once for every `period` seconds of this node's clock.
  virtual void create_timer(double period, std::function<void()> callback) = 0;
  virtual double now() const = 0;
  virtual std::string full_name(const std::string& name) const = 0;
};

class InProcessNode;

// Everything the in-process nodes share: one bus, one clock, and the list of nodes that are alive.
class World {
 public:
  TopicBus bus;
  SimClock clock;

  // Move the clock on by `dt` seconds and let every living node's timers fire.
  void advance(double dt);

  std::size_t node_count() const { return nodes_.size(); }

 private:
  friend class InProcessNode;
  std::vector<InProcessNode*> nodes_;
};

// A node that lives in a World. When it is destroyed it takes back everything it asked for: its subscriptions stop and its timers stop, and
// nothing in the world holds anything of its.
class InProcessNode : public Node {
 public:
  InProcessNode(World& world, const std::string& ns, const std::string& name) : world_(world), ns_(ns), name_(name) { world_.nodes_.push_back(this); }

  ~InProcessNode() override {
    for (TopicBus::Token t : tokens_) world_.bus.unsubscribe(t);
    for (std::size_t i = 0; i < world_.nodes_.size(); ++i)
      if (world_.nodes_[i] == this) {
        world_.nodes_.erase(world_.nodes_.begin() + static_cast<std::ptrdiff_t>(i));
        break;
      }
  }

  InProcessNode(const InProcessNode&) = delete;
  InProcessNode& operator=(const InProcessNode&) = delete;

  std::string full_name(const std::string& name) const override { return resolve_name(name, ns_, name_); }

  std::function<void(const std::string&)> create_publisher(const std::string& topic) override {
    const std::string full = full_name(topic);
    World* world = &world_;
    return [world, full](const std::string& payload) {
      if (!full.empty()) world->bus.publish(full, payload);
    };
  }

  void create_subscription(const std::string& topic, std::function<void(const std::string&)> handler) override {
    const std::string full = full_name(topic);
    if (full.empty()) return;
    tokens_.push_back(world_.bus.subscribe(full, [handler](const std::string&, const std::string& payload) { handler(payload); }));
  }

  void create_timer(double period, std::function<void()> callback) override {
    timers_.push_back(Timer{SimTimer(period, 1), callback});
    timers_.back().timer.poll(world_.clock);               // arm it now, so its first period runs from the moment it was made
  }

  double now() const override { return world_.clock.now(); }

  // Called by the world after the clock moves.
  void fire_timers() {
    for (Timer& t : timers_) {
      const int n = t.timer.poll(world_.clock);
      for (int i = 0; i < n; ++i) t.callback();
    }
  }

 private:
  struct Timer {
    SimTimer timer;
    std::function<void()> callback;
  };
  World& world_;
  std::string ns_;
  std::string name_;
  std::vector<TopicBus::Token> tokens_;
  std::vector<Timer> timers_;
};

inline void World::advance(double dt) {
  clock.on_clock(clock.now() + dt);
  const std::vector<InProcessNode*> alive = nodes_;      // a timer's callback may create or destroy a node
  for (InProcessNode* n : alive)
    for (InProcessNode* m : nodes_)
      if (m == n) {
        n->fire_timers();
        break;
      }
}

#endif  // LESSON_SOLUTION_HPP
