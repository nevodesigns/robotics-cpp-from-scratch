# A Release That Reaches the Robot: Target, Manifest, Version, Fit, Install and Trial in One Pipeline, Cut at Every Step

> The robot should be told one thing: here is a release. Everything else is what the robot checks, in order, and what it does when the answer is no.

**Type:** Capstone
**Time:** about 160 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 20-11, 20-10, 20-08, 20-07, 20-06, 20-05, 20-04, 20-01

## The Problem

Eleven lessons built the pieces: a target (20-01), a toolchain (20-02), what to build for which machine (20-03), a bundle with a manifest (20-04), an install into the other slot (20-05), a trial boot with rollback (20-06), a staged rollout (20-07), services (20-08), a log that cannot fill the disk (20-09), a clock that may be wrong (20-10) and a check that it fits (20-11). A deployment is not any one of them. It is **the order they are used in**, and the answer to the question that comes up in every incident: **what stops a bad release from reaching the robot, and what happens if one does?**

This lesson puts the pieces end to end, and then does to the whole what was done to the install in lesson 20-05: cut it at every point, offer it everything wrong, and see that the robot ends in a state that somebody could be sent to.

## The Concept

### A version, and a floor

A version is three whole numbers, `major.minor.patch`. `parse_version` accepts exactly that and refuses `1.4`, `1.4.2.1`, `1.4.x`, `1..2`, `v1.4.2`, a space, a minus sign, and ten digits. `compare` goes by major, then minor, then patch: **1.10.0 is newer than 1.9.0**, and as **text** it sorts before it (`"1.10.0" < "1.9.0"`), which is the oldest bug in version handling. `decide` is checked **in order**:

| offered, on a robot running 1.4.2 with a floor of 1.3.0 | answer |
|---|---|
| 1.5.0 | install |
| 1.4.2 | refuse, the same |
| 1.4.0 | refuse, older |
| 1.4.0, rollback authorised | install |
| 1.2.0, rollback authorised | **refuse, below the floor** |
| `oops`, or an empty installed version | refuse, invalid |

A **rollback** is a version going backwards, which a signed release of last year with a known fault is as good as: a robot that takes any older release that is offered can be made to run one with a hole in it. So it is refused unless somebody has deliberately authorised it, and below the **floor** (the lowest release without a fault that must not come back) it is refused whatever is authorised. See `E-DEPLOY-0034`.

### Six stages, in order

`deploy` takes a release to a robot, stage by stage, **stopping at the first that fails**, and **writing nothing to the disk before the install stage**:

1. **target**: both triples read, and the same architecture and system.
2. **manifest**: it parses, names the same version as the release, and the files match it.
3. **version**: `decide` says install.
4. **fit**: the release is not bigger than two slots allow.
5. **install**: plan, write the other slot, commit, move the pointer (20-05).
6. **trial**: three tries, a soak and a deadline (20-06).

A good release passed all six (`target > manifest > version > fit > install > trial`), the robot booted release **1.5.0 from slot 1**, and release 1.4.2 was **still valid in slot 0**. See `E-DEPLOY-0034`.

### What each refusal looks like

Eight releases that should not be taken, each offered to the same robot:

| release | refused at |
|---|---|
| built for 32 bit ARM | target (built for arm-linux-gnueabihf, robot is aarch64-linux-gnu) |
| manifest cut short | manifest (no end line) |
| a file altered | manifest (files differ) |
| manifest of another version | manifest (wrong version) |
| older than installed | version |
| below the floor, rollback authorised | version |
| the same version | version |
| 6400 MB, over the room for two slots | fit (too big) |

Each left the robot running 1.4.2, not bricked, with the boot pointer where it was and **slot 1 still empty**: nothing was written. That is the structure to copy: **do every check that can be done without changing anything first**, in the order of cost, and only then write. A rollback somebody authorised, above the floor, installed.

### A power cut at every step

The install is ten steps (eight chunks, the hash, the pointer). Power lost after **each** of 0 to 9 steps, and then the install run again:

| | result |
|---|---|
| cut at each of the 10 points | **10 of 10** boot 1.4.2, **0 bricked** |
| run again from the start | **10 of 10** end on 1.5.0, confirmed |

Lesson 20-05's result, kept through the whole pipeline: the cut is reported as `install` failed with `power lost after k steps`, with what the robot boots, and the same release offered again goes through. A pipeline that has to be **restarted by hand** after a power cut is one that fails in the field, where the person who knows how is not.

### What the trial catches, and what it does not

A release that crashes 2 s after it starts: `trial` failed with `rolled back`, the robot running **1.4.2**, **up**, both slots valid, boot pointer back on slot 0. And a release that is healthy for 90 s and then crashes, every boot: it is healthy at 20 s, the soak of 30 s confirms it at **50 s**, and it is **confirmed before it crashes**. The result was `running 1.5.0, confirmed yes, up NO`: a robot in a crash loop on a release it has committed to. This is the limit of lesson 20-06, stated as a result and not a hope: a trial can only catch what shows within the soak, and the soak is a number somebody chose. See `E-DEPLOY-0035`.

### Across a fleet

A release that crashes on **300 of 1000** robots (a hardware variant), with the robots in a fixed shuffle. Given to everyone, 300 go through a trial and roll back: **300 robots out of service for the length of a trial**, and a service call each. Staged with the policy of lesson 20-07, the rollout **halted at wave 0 after 10 robots**. Of those ten: 9 were on 1.5.0, 1 was back on 1.4.2, **0 bricked**, and the one that failed is exactly the one the rollout counted as failed. The pieces agree: the per-robot trial finds the fault on the robot that has it, and the rollout stops it spreading. See `E-DEPLOY-0036`.

## Build It

`parse_version`, `compare`, `decide` and `deploy` in `exercise/solution.hpp`. The types are given, and the work of the other lessons is in the headers.

```
rcpp verify 20-12
```

## Use It

`rc/deploy/release.hpp` is the last header of `rc::deploy`.

```cpp
rc::deploy::RobotState robot = read_state();
const rc::deploy::Outcome o = rc::deploy::deploy(robot, offered_release, -1, /*rollback_authorised=*/false);
if (!o.failed_stage.empty()) report(o.failed_stage, o.stages.back().detail);
```

and, as the test of every change to the pipeline, the loops of this lesson: each refusal, a cut at each step, and a fleet.

**What this does not do.** It is the order and the decisions, run against a **simulated** disk and a **simulated** boot. The real work is in the parts that are only named: the cross compile with a toolchain (20-02), the unpacking of files, the signature on the manifest, the boot loader that keeps the counter, the flash that writes atomically, and **testing all of it on real hardware with real power cuts**. It has no cross compiler in this repository's continuous integration, so a release here is made of strings. It does not authenticate **who sent** the release, which is the signature of lesson 20-04's closing section and the largest hole left. It does not migrate data, handle two robots that talk to each other at different versions, restart services in the order of lesson 20-08, or touch the clock of lesson 20-10, the log of 20-09 or the fit of 20-11 beyond the size of the release. And it does nothing about a release that is **bad and committed**: the answer is the next release, sent by the same pipeline, with a floor that was raised.

## What Breaks First

- **A downgrade that the robot accepts.** See `E-DEPLOY-0034`.
- **A trial shorter than the failure.** See `E-DEPLOY-0035`.
- **A bad release given to everyone.** See `E-DEPLOY-0036`.

## Ship It

`rc/deploy/release.hpp` holds `Version`, `parse_version`, `compare`, `VersionDecision`, `decide`, `Release`, `RobotState`, `StageResult`, `Outcome` and `deploy`.

This ends phase 20: a target that is read and not guessed, a toolchain that is generated and checked, a bundle that checks itself, an install that cannot brick, a trial that rolls back, a rollout in waves, services that start in order, a log that cannot fill the disk, a clock that is known to be wrong, a check that it fits, and a pipeline that does all of it in order.
