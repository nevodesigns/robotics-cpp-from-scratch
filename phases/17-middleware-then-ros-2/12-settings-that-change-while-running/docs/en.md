# Settings That Change While Running: Typed Parameters, Validation and Changing Several at Once

> A robot's settings change while it is running, from a laptop, by someone in a hurry, with a typo.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 17-09, 03-02, 01-06

## The Problem

Gains, speed limits, the name of a frame, whether a node is enabled: these are not messages, they are the node's **parameters**. They have to be changeable while it runs, since restarting a robot to try a gain of 2.5 instead of 2.0 is not tuning. And a setting changed from outside is input from outside, and has the problems of input: a wrong type, an absurd value, a name that was mistyped, and something that should not be changed at all.

ROS 2 gives every node a parameter server, and service calls (lesson 17-09) to read and write it. What the server does with a write is the subject: whether it checks, and what happens when a write names two settings and one is wrong.

## The Concept

### Declare, then set

A setting is **declared** with a default value, which fixes its **type** for ever, and an optional range and a read only flag. A set of a name that was never declared is refused: `ki: not declared`, and a mistyped name does not create a new setting that the node never reads. See `E-BUS-0034`.

```cpp
p.declare("kp", Value::of(2.0), ranged);       // a real, from 0 to 100
p.declare("serial", Value::of(std::string("A-1001")), fixed);   // read only
```

A `Value` carries its type and **nothing converts**: 1 is not 1.0. The last mistake worth naming is a trap of the C++ overload: a string literal such as `"A-1001"` would pick the `bool` overload, which is why the declaration wraps it in a `std::string`.

### What is refused

| request | reason |
|---|---|
| kp as an integer | kp: wrong type |
| kp = 250 | kp: out of range |
| kp = -1 | kp: out of range |
| serial | serial: read only |
| channels as a real | channels: wrong type |

All five were refused and nothing changed: kp still 2.0, serial still A-1001. The ends of the range are inside it, so 0 and 100 were accepted. A node that took a gain of 250 would be running a loop at a hundred times its design. A node that took 5 as an integer where it reads a real is reading whatever bits are there, and **not converting** is what exposes it. See `E-BUS-0034`.

### A valid change, recorded

A change that takes effect is recorded with what it replaced: `kp` from 2.0 to 3.5. Setting the same value again is allowed and is not a change, so it is not recorded. The record is how a log can say who changed the gain, and what it was. See `E-BUS-0036`.

### Several at once

A client that changes `kp`, `enabled` and `channels` in one call should find all three changed or none. Three changes with the last the wrong type:

```
refused (channels: wrong type); kp is still 2.0, enabled is true
```

Without that, the first two are applied, the third is refused and the node is running with half of a change that nobody intended. The server works on a **copy** of the settings, checks every request against it, and applies them only when all have passed. See `E-BUS-0035`.

### A rule about the whole

Some rules are about settings together: the minimum speed must be below the maximum. The rule is checked against the settings **as they would end up**, not request by request. Moving `min_speed` 0.1 and `max_speed` 1.0 to 2.0 and 3.0:

| how | result |
|---|---|
| min first, then max | refused: min_speed must be below max_speed |
| both together | ok |
| min 5.0 with max 4.0 together | refused: min_speed must be below max_speed |

One at a time, the first step breaks the rule, though the end is fine. Together, the rule sees the end. And a pair that ends wrong is refused whole, with the settings as they were. See `E-BUS-0036`.

## Build It

`set_many` in `exercise/solution.hpp`. The values, descriptors, `declare`, `get`, `set` and the log are given.

```
rcpp verify 17-12
```

Nothing is timed.

## Use It

`rc/bus/params.hpp` joins `rc::bus`.

```cpp
ParameterServer p;
p.declare("kp", Value::of(2.0), ranged);
const SetResult r = p.set_many({{"kp", Value::of(3.0)}, {"ki", Value::of(0.5)}});
if (!r.ok) report(r.reason);
```

**What this does not do.** A parameter in ROS 2 can also be an array, and has a description and an additional constraint, and can be declared as dynamically typed. A node that is running does not know that a value changed unless it is told: the callback that the real parameter server calls on a change is not here. The log is in memory and the settings are not saved: a restart goes back to the defaults, and saving them is a file and a lesson
of its own (lesson 4-12 layered settings from files). And a rule about the whole is one function, so two rules from two parts of the node are the caller's to combine.

## What Breaks First

- **A set that creates what it was not told to.** A typo in a name that is accepted. See `E-BUS-0034`.
- **A change of several, applied halfway.** A node with half of a retune. See `E-BUS-0035`.
- **A rule checked one change at a time.** Moves that are fine together refused, and ones that are bad let through. See `E-BUS-0036`.

## Ship It

`rc/bus/params.hpp` holds `ParamType`, `Value`, `same_value`, `Descriptor`, `SetResult`, `Change` and `ParameterServer`.

A node can now be tuned while it runs without being tuned into a state nobody asked for.
