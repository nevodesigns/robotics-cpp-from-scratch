# What a Name Means: Namespaces, Private Names and Remapping

> The same node, run on two robots, should be two nodes. A name is how.

**Type:** Build
**Time:** about 110 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 17-01, 01-05

## The Problem

Up to now a topic's name was the string it was published under, and equal strings met. That was enough for one robot. Two robots on one network, each with a lidar driver publishing `scan`, would put 20 000 scans a second on the one topic, and each robot's planner would plan from both. The lidar driver's code is the same and the name it publishes under must differ.

The answer in ROS 2, and in most middleware, is that a name written in a node is not the name on the bus: it is **resolved**, against the node's namespace and its own name, and can be **remapped** from outside without editing the node.

## The Concept

### What a name may be

A topic name is checked before it is used. The rules, from ROS 2: letters, digits, underscores and slashes; no empty part, so no `a//b` and no trailing slash; no part starting with a digit; and a tilde only at the very start, as `~` or `~/`. The test fed it 9 good names (`scan`, `/scan`, `robot1/scan`, `~/status`, `_hidden` and others) and 16 bad ones (`scan/`, `a//b`, `1scan`, `a b`, `a-b`, `~x`, `a/~/b`, `{node}/x`, and more),
and all 9 were accepted and all 16 rejected. A name with a space in it that is accepted and published to is a topic nobody can type into a command line. Substitutions in curly braces are a real ROS 2 feature and are not handled here, so braces are rejected. See `E-BUS-0031`.

### Three kinds

A name means different things written three ways. For a node `lidar_driver` in namespace `/robot1`:

| written | means |
|---|---|
| `scan` | `/robot1/scan` |
| `/scan` | `/scan` |
| `front/scan` | `/robot1/front/scan` |
| `~/status` | `/robot1/lidar_driver/status` |
| `~` | `/robot1/lidar_driver` |

A **relative** name is put in the node's namespace. An **absolute** name, with a slash in front, is itself and ignores the namespace. A **private** name, with a tilde, is inside a namespace named for the node: it is where a node puts what is its own, such as its status and its parameters. The namespace is first made to look the same however it was written: `robot1/`, `/robot1` and `/robot1/` are one namespace, the empty one is the root `/`,
and the root contributes nothing, so a name in it is `/scan` and not `//scan`. See `E-BUS-0032`.

### Why it matters

Two robots, one node:

```
relative: /robot1/scan and /robot2/scan  (apart)
absolute: /scan and /scan  (the same: one topic, two publishers)
```

A node that writes `scan` can be run on any number of robots by giving each its namespace. A node that writes `/scan` has hard coded the answer, and two of them share the one topic. The fix is to write relative names in a node, and to leave absolute ones for things that are truly global. See `E-BUS-0032`.

### Remapping

A node written to publish `scan` can be told, from outside, that for this run `scan` is `/filtered/scan`:

| resolved | rule | result |
|---|---|---|
| /robot1/scan | scan -> /filtered/scan | /filtered/scan |
| /robot1/lidar/status | ~/status -> /diagnostics/lidar | /diagnostics/lidar |
| /robot1/pose | none | /robot1/pose |

A rule is read the same way as any name, and applies to the **resolved** name. The same rule in a node in `/robot2` is a rule about `/robot2/scan` and does not apply to `/robot1/scan`. A remap is how a recorded bag is played into a node that wants a different name, and how a robot's nodes are wired together without touching their code. See `E-BUS-0033`.

## Build It

`valid_topic_name`, `normal_namespace`, `resolve_name` and `remap_name` in `exercise/solution.hpp`. The `Remap` type is given.

```
rcpp verify 17-11
```

Nothing is timed or random.

## Use It

`rc/bus/names.hpp` joins `rc::bus`.

```cpp
const std::string topic = rc::bus::resolve_name("scan", "/robot1", "lidar_driver");   // /robot1/scan
const std::string wired = rc::bus::remap_name(topic, rules, "/robot1", "lidar_driver");
```

**What this does not do.** It handles topic names and not the others ROS 2 has: node names and namespaces have rules of their own (they cannot contain slashes in the name, for one), and service and action names follow the topic rules plus more. It does not do substitutions in braces. It does not check a name's length, which DDS limits.
It matches a remap rule on the whole name and has no wildcards, and a real remap can also change a node's name or namespace.

## What Breaks First

- **A name nobody checked.** A topic with a space or a hyphen in it. See `E-BUS-0031`.
- **An absolute name in a node.** Two robots sharing one topic. See `E-BUS-0032`.
- **A remap written against the wrong name.** It applies to nothing, and silently. See `E-BUS-0033`.

## Ship It

`rc/bus/names.hpp` holds `valid_topic_name`, `normal_namespace`, `resolve_name`, `Remap` and `remap_name`.

A node can now be written once and run as many times as there are robots.
