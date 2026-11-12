# Run Here, Ship There: Which Binaries Run During the Build, Which Go to the Robot, and the Bug a Native Build Hides

> If it only fails when you cross compile, you will find out the day you first cross compile.

**Type:** Build
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 20-02, 20-01, 04-01, 03-03

## The Problem

A native build has one machine, and every mistake about **which machine** is invisible: a library built "for the target" is built for the machine you are sitting at, a code generator that runs during the build runs. Cross compilation has two machines, and the build graph is now a graph of **who builds what for whom** and **who runs what**. The mistakes are the same few, and each one is an error the first time the project is cross compiled, usually the week a robot is due, after a year of working only natively.

## The Concept

### Two machines, three rules

The **host** does the building; the **target** is what the result is for. Each step of the build is built for one of them. There are three rules:

1. **Anything the build runs itself** (a code generator, a test) must be built for the host, or for a target the host could run. A message compiler built for the robot cannot be run on the laptop.
2. **A target binary links only target libraries.** A library built for the host, linked into a robot program, is the wrong architecture.
3. **The bundle that goes to the robot holds only target binaries.** A host tool in the bundle is dead weight at best.

`can_run` answers whether a binary built for one target runs on a host. On an x86_64 Linux host:

| built for | runs |
|---|---|
| x86_64-linux-gnu | runs |
| i686-pc-linux-gnu | runs (a 32 bit binary on a 64 bit system) |
| aarch64-linux-gnu | does not run |
| arm-linux-gnueabihf | does not run |
| x86_64-w64-mingw32 | does not run |

A 64 bit binary does not run on a 32 bit machine, and a triple that cannot be read runs nowhere.

### The graph that works natively

The robot's build as first written: a message generator `msg_gen` (executed by the build) and the unit tests (also executed by the build), both built for the target, since that is what the toolchain says every executable is; a message library `libmsgs` that somebody built on the laptop and checked in; the program `robot_node`, linking it; and a host tool `calibrate`, installed with it. Built for the host itself: **0 problems**. Every one of the four mistakes works, because the target **is** the host. That is what makes them dangerous. See `E-DEPLOY-0007`.

### The same graph, for the robot

Built on x86_64 for aarch64, the same graph had **4 problems**, in the order the checks run:

- `msg_gen: built for the target but run during the build`
- `unit_tests: built for the target but run during the build`
- `robot_node: links libmsgs, built for the other machine`
- `calibrate: host binary in the bundle`

Each is a one line change in the build file, and none of them is a **compile** error: the first two fail when the build tries to run an aarch64 program on an x86_64 machine, the third fails at the link, and the fourth is not an error at all, but a robot with a tool on it that cannot run. See `E-DEPLOY-0008`.

### The fix

The generator built for the host, the library built for the target, the unit tests **not** run by the build (they run on the robot or in an emulator, which is a matter for a later lesson), and only `robot_node` in the bundle: **0 problems**. The cost is real and should be seen: some tests no longer run in the build, and a library is built twice if both machines need it. A build that compiles everything twice is the right answer to a project that has two machines. See `E-DEPLOY-0009`.

A step named in a link or an install that is not a step gave an issue on that name, and an unreadable triple one issue and nothing else, and neither crashed.

## Build It

`can_run` and `check` in `exercise/solution.hpp`. The types are given.

```
rcpp verify 20-03
```

## Use It

`rc/deploy/build_split.hpp` joins `rc::deploy`.

```cpp
const auto issues = rc::deploy::check(steps, edges, install, rc::deploy::parse_triple("x86_64-linux-gnu"), rc::deploy::parse_triple("aarch64-linux-gnu"));
for (const auto& i : issues) std::cerr << i.step << ": " << i.message << "\n";
```

**What this does not do.** It is given the graph; it does not read a CMake project and find it. A real project's graph comes from `cmake --graphviz` or from the build system's own files, and turning that into steps is a job of its own. It treats each step as built wholly for one machine, which is true of a plain executable and false of a test that runs under an emulator, a header only library or a Python script. It does not look at **what a library is built from**, only at who links whom. And the rules cover the usual structure and not a project that builds a compiler. The real check is a cross build in the continuous integration, which this repository does not have, and a native build that passes tells you nothing about any of this.

## What Breaks First

- **A mistake that only a cross build can show.** See `E-DEPLOY-0007`.
- **Something the build runs, built for the robot.** See `E-DEPLOY-0008`.
- **A host binary in the bundle, or a host library in the robot.** See `E-DEPLOY-0009`.

## Ship It

`rc/deploy/build_split.hpp` holds `Where`, `Step`, `Edge`, `Issue`, `can_run` and `check`.
