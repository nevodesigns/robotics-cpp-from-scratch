# The Seam Where ROS 2 Plugs In: Writing a Node Against an Interface and Testing It Without ROS

> The code that decides what the robot does should not know what carries its messages. If it does, it can only be tested on a robot.

**Type:** Build
**Time:** about 130 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 17-11, 17-13, 17-01, 03-10, 02-04

## The Problem

Everything in this phase so far was the middleware by hand: topics, queues, quality of service, services, actions, parameters, time, frames. The real thing, ROS 2, is a middleware with all of these, and it is a large dependency: it needs a particular operating system release, hundreds of packages, a build tool of its own, and a running network stack to do anything at all.

A planner written directly against `rclcpp` can be tested only with all of that. It is slow to build, a test needs processes talking to each other, and it cannot run on a machine without ROS 2, which includes the continuous integration lanes of this repository. The cure is old: write the planner against an **interface**, and put the middleware behind it.

## The Concept

### What a component may ask of its node

```cpp
class Node {
 public:
  virtual std::function<void(const std::string&)> create_publisher(const std::string& topic) = 0;
  virtual void create_subscription(const std::string& topic, std::function<void(const std::string&)> handler) = 0;
  virtual void create_timer(double period, std::function<void()> callback) = 0;
  virtual double now() const = 0;
  virtual std::string full_name(const std::string& name) const = 0;
};
```

That is the whole contract. A **component** takes a `Node&` and nothing else, and never mentions a bus, a clock or a ROS 2 type. The test's component is twelve lines: it takes numbers on `in`, doubles them onto `out`, and every second publishes how many it has handled on `~/status`. It is written once, and runs against any class that implements `Node`. See `E-ROS-0001`.

### A node in a world

The implementation for tests is an **in-process node**, living in a **world** that holds the bus (lesson 17-01), the clock (17-13) and the list of nodes alive. Its names are resolved by lesson 17-11's rules, against its namespace and its own name:

| written | node `doubler` in `/robot1` |
|---|---|
| `out` | `/robot1/out` |
| `~/status` | `/robot1/doubler/status` |
| `/clock` | `/clock` |

A listener in `/robot1` heard `42` and `8` for the inputs 21 and 4, and a listener on the other robot, in `/robot2`, heard nothing. Two nodes running the same component in two namespaces did not hear each other: robot 1 handled 2 messages and robot 2 handled 0. See `E-ROS-0001`.

### Timers on the world's clock

A timer created on a node fires once per period of the world's clock, and **is armed when it is created**, not at its first poll: otherwise a timer made at 0 and checked first at 0.6 s would have missed the whole of its first period. Over 3.5 s of the clock, with a number sent at 0.5, 1.5, 2.5 and 3.5 s, the status timer fired 3 times and the messages read `handled 1`, `handled 2`, `handled 3`. See `E-ROS-0003`.

### When a node goes away

The hard problem of any interface like this is **lifetime**. A subscription is a function the bus will call, and it captures the component that made it. If the node is destroyed and the subscription stays, the next message calls into something that no longer exists. In a test it is a crash that happens on a later line; on a robot it is a crash after a reconfiguration.

An in-process node takes everything back when it is destroyed: it unsubscribes every token it holds and removes itself from the world. The test destroyed a node that had a subscription and a timer:

```
nodes alive 1, subscribers to its topic 0, calls into it afterwards 0
```

Before it died, a message and half a second of clock had called into it twice. After, ten messages and five seconds called into it never. See `E-ROS-0002`.

### Churn

A node that is made and destroyed 200 times, each with a subscription to the same topic, left the world with 1 node and 1 subscriber (the keeper), and the keeper still heard every message: 200 in the loop and 1 more after. A publisher on an invalid name does nothing, and a subscription on one does nothing, rather than throw: the name was checked when it was created, and the check says no. See `E-ROS-0002`.

### The other side of the seam

A ROS 2 adapter is one class that implements the same interface over `rclcpp`: `create_publisher` makes an `rclcpp` publisher of a string message and returns a function that publishes with it, `create_subscription` makes a subscription, `create_timer` makes a wall or simulated time timer, and `now` reads the node's clock. It would look like this, and **it is not built or tested by this repository's continuous integration**,
which has no ROS 2 lane. It is a sketch, to show where the dependency would sit:

```cpp
class RclcppNode : public Node {
  std::shared_ptr<rclcpp::Node> n_;
  // create_publisher: auto p = n_->create_publisher<std_msgs::msg::String>(topic, qos);
  //                   return [p](const std::string& s) { std_msgs::msg::String m; m.data = s; p->publish(m); };
  // now:              return n_->now().seconds();
};
```

Everything above it, the component and its tests, is unchanged. That is the point, and it is also the limit of what this lesson claims: that the seam is the right shape is checked here, and that an adapter satisfies it has to be checked wherever ROS 2 is installed.

## Build It

The destructor, `create_publisher`, `create_subscription` and `create_timer` of `InProcessNode` in `exercise/solution.hpp`. The interface, the world, `full_name`, `now` and `fire_timers` are given.

```
rcpp verify 17-17
```

The clock is passed by hand, so the figures above are the same on every run.

## Use It

`rc/ros/node.hpp` opens `rc::ros`.

```cpp
World world;
InProcessNode node(world, "/robot1", "doubler");
Doubler doubler(node);          // the component, written against Node&
world.advance(0.1);
```

**What this does not do.** It carries strings, where a ROS 2 topic carries typed messages: lesson 17-03's frames are how a typed message becomes bytes. It has no quality of service and no discovery, which lessons 17-07 and 17-04 describe and which a ROS 2 adapter gets from DDS. It is single threaded and a component given a real executor needs the care of lesson 17-14. And its timers poll the world's clock where a ROS 2 timer is woken by the executor.

## What Breaks First

- **A component that names a middleware.** It can be tested only where the middleware is. See `E-ROS-0001`.
- **A node that leaves callbacks behind.** A message calls into a component that is gone. See `E-ROS-0002`.
- **A timer armed at its first poll.** The first period is lost. See `E-ROS-0003`.

## Ship It

`rc/ros/node.hpp` holds `Node`, `World` and `InProcessNode`.

Component code can now be written, tested and shipped without a middleware, and run on one by adding a class.
