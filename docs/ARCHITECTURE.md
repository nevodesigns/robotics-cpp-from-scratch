# Architecture

Why this repository is shaped the way it is. Every decision here is load
bearing: change one and the rest need rework, which is what makes it an
architecture rather than a folder layout.

## The problem being solved

Python curricula get `python3 main.py` for free on every machine. C++ robotics
does not. A beginner meets the wrong compiler, a missing CMake, a Qt installer
demanding an account, a ROS 2 that exists on exactly one Ubuntu version, a
permission denied on a serial port, and a template error four hundred lines
long. People quit at the toolchain, not at the concepts.

Four failure modes this curriculum is designed against:

1. **Fragmentation.** C++ learned in one place, Qt in another, ROS in a third,
   control theory in a fourth. Nothing lines up, and the learner can write a
   class but cannot make a motor turn.
2. **Silent rot.** A tutorial written for Qt 5 still ranks first in search
   results. Nothing in it compiles, and nobody notices because nothing is built
   by machine.
3. **The Python escape hatch.** Nearly every ROS 2 tutorial reaches for `rclpy`
   because it demonstrates faster. The learner who wanted one language now needs
   two and never learns how the C++ side works.
4. **Hardware gatekeeping.** "Now connect your LiDAR." Half the audience stops
   at the exact moment it gets interesting.

The headline metric is therefore not the lesson count. It is the number of
lessons that compile and pass their tests, unattended, on every toolchain they
claim.

## The ten decisions

**D1. One language, including the tooling.** C++ is the only language a learner
reads or writes. `rcpp`, the auditor and doctor and verifier, is itself C++, so
it doubles as reference grade code a learner can read on day one. The cost is a
bootstrap: CI compiles the tool before it can check anything. That build takes
seconds and is the first proof the toolchain works.

**D2. The curriculum is one CMake super project.** Every lesson is a real build
target. Lessons cannot silently rot, because rot is a red build. This is the
largest structural difference from a markdown and Python curriculum, and it
turns compile time into an engineering problem to be managed.

**D3. Every lesson ships a failing test suite.** The learner is not asked to
follow along. `rcpp verify` runs their implementation against the same tests CI
runs against the reference. Green means done. Test design is the hardest
authoring work and it is where the pedagogy actually lives.

**D4. Platform support is declared per lesson and enforced.** `lesson.json`
lists the toolchains a lesson claims and CI builds exactly that set. A lesson
claiming Windows that does not build on Windows fails the change. Honest limits,
visible on the page, before the learner spends an evening.

**D5. C++17 baseline, with the later standards built by hand and bridged.**
Every supported toolchain implements C++17 completely, so both Ubuntu releases
compile identical code with no version gates. The facilities that arrived later,
`std::span` in C++20 and `std::expected` in C++23, are provided by
`rc/core/compat.hpp` as `rc::span` and `rc::expected`, which alias the standard
types where they exist.

The reason is pedagogical rather than defensive. A span really is a pointer and
a count, and a learner who has written those sixty lines understands `std::span`
completely on the day they meet it. That understanding is only available while
the facility is still missing, so starting at the newest standard would throw it
away. Rule L017 keeps lessons on the baseline so the device does not erode, and
rule L018 keeps discarded style out, because a learner who meets `NULL` in a
lesson will carry it for years.

Moving to C++20 later is one line in `CMakeLists.txt` and no lesson edits.

**D6. Qt baseline is what apt gives you.** Lessons target the Qt 6.2 API so
`apt install qt6-base-dev` works on both supported Ubuntu releases with no
account wall. CI additionally builds every Qt lesson against pinned Qt 6.8 LTS,
so the upgrade lane is proven rather than assumed.

**D7. Hardware tiers, and every driver has a fake.** Tier 0 software only, 1
simulated, 2 cheap hardware, 3 a real robot. Every tier 2 or 3 lesson must ship
a fallback, which is enforced by rule L014. CI can then run hardware lessons
headless and deterministically, and no learner is ever hard blocked.

**D8. Build it, then use it.** PID by hand before any control library.
Quaternions before Eigen. A publish and subscribe broker before `rclcpp`. The
framework arrives after the learner has written the smaller version, so it stops
being magic. This roughly doubles authoring effort per topic and is the reason
the curriculum is worth existing.

**D9. Safety is a phase, not a footnote.** Risk assessment, emergency stops,
watchdogs, fail safe state machines, and the standards landscape. The safety
primitives appear in phase 14 and every capstone is required by its own tests to
use them.

**D10. If the auditor does not check it, it is not a rule.** Documented rules and
enforced rules are the same set. This exists because the alternative is
observable in the wild: curricula whose contributor guide mandates quizzes,
objectives and tests, while the checker verifies almost none of it, end up with
hundreds of lessons missing all three and a green badge throughout.

## Curriculum map

Twenty one phases, roughly 320 lessons at the full build. Two ordering rules:
something moves in the first hour, and every phase ends with something that
runs, shows or moves.

| Phase | Lessons | Ends with |
|---|---|---|
| 00 Toolchain and First Motion | 14 | A robot driving in a Qt window |
| 01 C++ Core I, values and flow | 18 | A telemetry log parser |
| 02 C++ Core II, memory and ownership | 16 | A leak free device handle |
| 03 C++ Core III, classes and templates | 18 | A generic ring buffer |
| 04 Build systems and tooling | 14 | Your own installable package |
| 05 Testing, fakes and verification | 12 | A fake device with replay |
| 06 Robot maths, by hand then Eigen | 16 | A live rotation viewer |
| 07 Concurrency and real time | 16 | A measured latency histogram |
| 08 Systems input and output | 14 | Talking to a real microcontroller |
| 09 Qt Core | 14 | Understanding what moc generates |
| 10 Qt Widgets | 14 | A live telemetry plotter |
| 11 Qt Quick and QML | 12 | A touchscreen operator panel |
| 12 Qt for robot systems | 12 | A shippable control station |
| 13 Kinematics and dynamics | 16 | A six axis arm hitting a pose |
| 14 Control | 14 | A tuned loop on real hardware |
| 15 Sensors, drivers and vision | 16 | A calibrated sensor stack |
| 16 Estimation and navigation | 16 | A rover navigating a map |
| 17 Middleware, then ROS 2 | 20 | A lifecycle managed node graph |
| 18 Simulation and hardware in the loop | 12 | One binary, sim and bench |
| 19 Safety and field engineering | 14 | A defensible risk assessment |
| 20 Deployment and cross compilation | 12 | A robot that boots into your code |
| 21 Capstones | 10 | Something to show an employer |

Three ordering choices worth defending:

- **Qt arrives at phase 09, not at the end.** A GUI is the cheapest way to see
  what your code is doing. Once the learner has a plotter, every later phase gets
  a visual debugger for free. Putting Qt last wastes it.
- **Testing comes before maths.** Every later lesson ships a failing suite as its
  primary interface, so testing cannot be an advanced topic.
- **ROS 2 sits late and stays optional.** Everything before it transfers to the
  large part of industrial robotics that never touches ROS, and the learner meets
  DDS concepts having already built the idea by hand.

## Milestones

| Milestone | Scope | Done when |
|---|---|---|
| M0 | The machinery plus twelve lessons spanning phases 00, 01, 09 and 14 | A stranger on Windows and a stranger on Ubuntu 22.04 both go from clone to a passing PID test without asking a question |
| M1 | Phases 00 to 05 complete, atlas to about 120 entries | Someone who has never programmed can write, build, test and debug C++ on their own machine |
| M2 | Phases 09 to 12, first capstone, Qt WebAssembly demos | The repository is independently useful to Qt developers who do not care about robotics |
| M3 | Phases 06 to 08 and 13 to 16, the deterministic simulator | A rover navigates a map using the learner's own estimator and planner |
| M4 | Phase 17 dual distro, 18, 19 and 20 | The same binary runs in simulation, on the bench, and on a Jetson |
| M5 | Capstones and tutor skills | Graduates have something an employer recognises |

**On lesson counts.** Shipping ninety lessons at full contract, every one
building on every toolchain with passing tests, beats five hundred with a green
badge and no tests, for the only audience that matters here. The count is not
the product.

## Known risks

| Risk | Mitigation designed in |
|---|---|
| Authoring effort, four to eight hours per lesson | Depth over count. The contract makes outside contributions reviewable |
| ROS 2 distro churn, Humble ends May 2027 | The `rc::ros` adapter seam and containerised CI. Migration is a tag change |
| Qt licensing obligations on shipped binaries | Taught in phase 12, not hidden |
| CI minutes across six toolchains | Change detection, ccache, full matrix nightly only |
| Hardware cost gating learners | Tiering and mandatory fakes, one standard low cost kit |
| Pressure to add Python for one demo | D1 is enforced by rule L015, not by good intentions |
