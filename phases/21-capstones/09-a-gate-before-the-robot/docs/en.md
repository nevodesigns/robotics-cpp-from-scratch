# A Gate Before the Robot: The Safety Suite as a Release Condition, a Candidate That Looks Fine and Is Not, and the Fault the Gate Cannot See

> Everything in this phase says what to test. This lesson says what a failing test is for: it stops the release.

**Type:** Capstone
**Time:** about 150 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 21-08, 21-07, 21-02, 20-12, 20-06, 19-14

## The Problem

Lesson 21-08 produced a report: which scenarios pass, which hazards are covered. A report is read by a person, and a person under pressure to ship does not always read. The way a safety result is made to matter is that it is a **condition**: a release that fails it **does not go**, and nobody has to remember to look.

This lesson is the join between phase 21 and phase 20. The pipeline of lesson 20-12 takes a release to a robot: target, manifest, version, fit, install, trial. It has no stage that asks **whether the release is safe**, because it cannot: it does not know what the release does. The gate is that stage, put **before** the first.

## The Concept

### A configuration is a release

The thing being shipped is a **policy**: a `DeliveryConfig` (top speed, inflation, whether the limiter and the supervisor are on, how long before it forgets what it has seen). `serialise_config` writes it as text, one setting to a line:

```
top_speed=0.8
robot_radius=0.15
inflate=0.3
lookahead=0.5
use_limiter=1
use_supervisor=1
replan_every=2
forget_after=4
```

and `rover_release` makes a `Release` of it for phase 20: a program, the policy file, a manifest that covers both. A different policy is a **different manifest**: the one with the limiter off has a different hash, since the policy is a file in the release. So the gate and the pipeline are about the same bytes.

### The gate

`run_gate(candidate)` runs the five scenarios of 21-08 with the candidate's configuration, makes the report, and passes it only if `suite_green`: no scenario failed and no hazard has a failing one. `ship_rover` runs it **first**, and only a candidate that passes goes through `rc::deploy::deploy`. There is no flag to skip the gate, and that is a design decision: a gate that can be switched off with an argument will be, on the day it matters.

### A good candidate

The gate: **5 of 5** scenarios passed, with the same report as in 21-08. Then the pipeline: `target manifest version fit install trial`, running **1.5.0**, **confirmed**. Two layers, one after the other, both clear.

### A candidate that looks fine and is not

A candidate with the **limiter and the supervisor taken out**, which is the kind of change that comes in as "simplify the control loop" or "speed up the planner":

| | |
|---|---|
| failing scenarios | **S2, S4, S5** |
| failing hazards | **G1, G2, G4, G8** |
| deployed | **no** |

S1, the empty building, **passed**: the candidate drives perfectly. Only the three scenarios that put something in the way failed, and the report's summary line reads `failing: G1, G2, G4, G8`. The robot's disk, boot pointer and installed version were untouched; the second slot was empty. Nobody had to be told. See `E-CAP-0025`.

### A candidate that fails on a scenario no hazard asked for

A candidate that **never forgets** what it saw, which keeps it waiting at a doorway that has been clear for minutes (the finding of 21-02): failing scenarios **S3**, failing hazards **none**. S3 is evidence for no hazard of the log, and the gate refuses anyway: it is a **functional** scenario that the robot must pass for the same reason, that a robot which is safe because it never moves is not a product. The gate is the whole suite and not only the safety part. See `E-CAP-0026`.

### What the gate cannot see

A candidate that **passes the gate**, shipped in a release that crashes 2 s after it starts on the robot: the gate passed, the release was deployed, and then `trial` failed (`rolled back`), the robot running **1.4.2** again. The gate tests the **policy in a simulation**; the trial tests the **release on a robot**. Neither can do the other's job, and a project needs both: a fault in the build, the image or the hardware is one the simulation does not contain, and a fault in the logic is one that the trial, which asks only whether it started and stayed up, does not find. See `E-CAP-0027`.

### Across a fleet

Twenty robots offered the rover with the limiter and supervisor out. With **no gate**, straight to the pipeline: **20 of 20** ran it (the pipeline checked everything it knows to check, and the release is valid, fits and boots). With the gate: **0**. See `E-CAP-0025`.

## Build It

`serialise_config` and `ship_rover` in `exercise/solution.hpp`. The suite, the gate, the release maker and the hazards are given.

```
rcpp verify 21-09
```

## Use It

`rc/capstone/ship_gate.hpp` joins `rc::capstone`.

```cpp
const rc::deploy::Release release = rover_release("1.5.0", candidate, "aarch64-linux-gnu");
const ShipResult s = ship_rover(robot, candidate, release, false);
if (!s.gate_passed) std::cerr << s.gate_report;
```

in the continuous integration of the project, where a red gate fails the build and the report is the build's output.

## Show It

Show the **three candidates and what happened to each**: the good one shipped, the unsafe one refused with the hazard ids in the message, and the one that fails only on a robot rolled back by the trial. A pipeline with two different kinds of failure caught in two different places is a better demonstration of engineering than one that never fails.

**What this does not do.** The gate is **as good as the suite behind it**: five scenarios, four of eight hazards (21-08 says which four are missing), and a candidate that is unsafe in a way none of them tries goes through. It takes the configuration to be the whole of the change: a release that changes **code** and not the policy is a release the gate runs with the old policy, which is a hole, closed only by running the gate on the built release itself. It is a **simulation**, and a candidate can pass it and fail on a robot. It does not run for each robot of a fleet or each hardware variant, which is where a staged rollout (20-07) comes in. It is a check **before** shipping, and does nothing about a fault found **after**: the next release goes through the same gate, with a regression scenario for the fault added to it, which is the habit that makes the suite grow.

## What Breaks First

- **A release process with no safety condition.** See `E-CAP-0025`.
- **A gate of the safety tests only.** See `E-CAP-0026`.
- **A gate taken for a trial, or a trial for a gate.** See `E-CAP-0027`.

## Ship It

`rc/capstone/ship_gate.hpp` holds `rover_hazards`, `default_suite`, `GateResult`, `run_gate`, `serialise_config`, `rover_release`, `ShipResult` and `ship_rover`.
