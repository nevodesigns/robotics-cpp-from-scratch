# Up in Order, Down in Reverse: Bringing a Graph of Nodes Up and Rolling It Back

> A robot is not started. It is assembled, one part at a time, and if a part is missing it is taken apart again in the order it went together.

**Type:** Build
**Time:** about 140 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 17-18, 03-03, 02-04

## The Problem

Each node in a robot's graph now has a life cycle (lesson 17-18). Something has to drive it, for all of them: the driver must be up before the filter that reads it, the filter before the planner, the planner before the controller. Starting them all at once and hoping, which is what a shell script does, gives a robot that works when the machine is fast and fails when it is not, and when one node does not come up, leaves the others running without it.

ROS 2 calls the part that does this **launch**. It has to answer three questions the individual nodes cannot: in what order, what if one fails to come up, and what does a clean stop look like.

## The Concept

### The order

Each node says what it needs. The start order puts every node after everything it needs. There may be many valid orders, and the choice should not depend on anything random: of the nodes that could go next, the one listed first goes first. Listed as planner, controller, filter, driver, logger, viewer, with the needs of each:

```
driver filter planner controller logger viewer
```

The listing was in no helpful order and the result is right. See `E-ROS-0007`.

### A list that cannot be started

Three things make a list impossible, and each is named, with no order returned:

| problem | reason given |
|---|---|
| a name listed twice | duplicate node: driver |
| a need not in the list | extra needs nowhere, which is not listed |
| needs that go round | the needs of the nodes go round in a circle |

A circle of needs, a needing b needing c needing a, has no start order, and finding that out by trying would be a robot that hangs at start up. An empty list is not a problem, and gives an empty order without an error. See `E-ROS-0007`.

### Two passes

The bring up does not start each node completely before the next. It **configures every node in order, and only then activates every node in order**:

```
driver:configure filter:configure planner:configure controller:configure logger:configure viewer:configure
driver:activate filter:activate planner:activate controller:activate logger:activate viewer:activate
```

Configuring opens devices and reads settings, and can fail. Activating makes the node produce and consume, and it is better that nothing is producing until it is known that everything that will consume it exists. If configuring the controller fails, no node has yet published a single message. See `E-ROS-0009`.

The clean shut down is the same order backwards, each node deactivated and then cleaned up, the newest first:

```
viewer:deactivate viewer:cleanup logger:deactivate logger:cleanup controller:deactivate ... driver:cleanup
```

A node is never taken down while something that needs it is still running. See `E-ROS-0008`.

### A required node fails

A node is **required** if the graph is not worth running without it. When the planner will not configure, the bring up stops at once, and everything already brought up is taken back, newest first:

```
driver:configure filter:configure planner:configure filter:cleanup driver:cleanup
```

The filter and the driver, which did configure, are cleaned up in reverse. Nothing was activated, and the controller, which was never reached, is where it started: unconfigured. The report says `ok` is false and `failed` is `planner`. See `E-ROS-0008`.

If instead the controller will not **activate**, the planner, filter and driver are already running, and are stopped as well: each is deactivated and cleaned up, in reverse, so that nothing is left active. See `E-ROS-0008`.

### An optional node fails

An **optional** node is a convenience: a logger, a viewer. Its failure is not worth the robot. When the logger will not configure, the graph comes up without it, and anything that needs it is left out too:

```
running: driver filter controller, left out: logger plotter
```

The plotter needed the logger and was never tried. The report says which nodes are running and which were left out, so that someone can see that the robot is up but degraded, and a supervisor can alert on it. A required node that needs an optional one that failed stops the bring up, since its need can never be met. See `E-ROS-0009`.

## Build It

`start_order`, `bring_up`, `take_down` and `roll_back` in `exercise/solution.hpp`. The specification and report types, `add`, `shut_down` and `state_of` are given.

```
rcpp verify 17-19
```

Nothing is timed. The log of every transition asked of every node is the thing the tests read.

## Use It

`rc/ros/launch.hpp` opens with `rc::ros`.

```cpp
Launcher l;
l.add("driver", {}, true).on(Transition::configure, open_camera);
l.add("planner", {"driver"}, true);
const LaunchReport r = l.bring_up();
if (!r.ok) report(r.failed, r.error);
```

**What this does not do.** It runs each callback to completion, one node at a time, where a real launch waits on a timeout and can bring independent nodes up together. It does not restart a node that failed later, which a supervisor has to. It decides about failure by whether the node is required and not by what failed, and there are failures that should be retried a few times before giving up (lesson 17-09's retries and timeouts are the tool). And it knows needs between nodes and not between a node and a topic: a node that needs a message, rather than another node, does not
appear in the graph.

## What Breaks First

- **Nodes started all at once.** Start up order decided by timing. See `E-ROS-0007`.
- **A required node fails and the rest are left running.** A robot half up, with nothing to say so. See `E-ROS-0008`.
- **Activating before everything is configured.** Output sent to consumers that do not exist yet. See `E-ROS-0009`.

## Ship It

`rc/ros/launch.hpp` holds `NodeSpec`, `start_order`, `LaunchReport` and `Launcher`.

A graph of nodes can now be brought up in order, taken down in reverse, and left out or rolled back by a rule.
