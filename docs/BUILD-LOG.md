# Build Log

The complete engineering record of this repository: where everything is, the
exact commands that verify it, what has been built and in what order, every
measurement that changed a decision, and every mistake that was made and how
it was caught.

[PROGRESS.md](PROGRESS.md) is the short recovery document and holds the
current counts. This is the long one. Where the two disagree, `rcpp audit` and
the generated block in `README.md` are authoritative, because they are read
from disk rather than typed.

Last verified 5 November 2026, after the documentation pass described under
"Every mistake" below.

## Where everything is

| | |
|---|---|
| working copy | `/home/nwokolo/robotics-cpp-from-scratch` |
| remote | `https://github.com/nevodesigns/robotics-cpp-from-scratch` |
| branch | `main`, and nothing else exists |

```
phases/<NN-phase-name>/<NN-lesson-name>/
    lesson.json          id, title, minutes, platforms, requires, artifact, breaks_first
    CMakeLists.txt       one line: rc_add_lesson()
    docs/en.md           the lesson, six required sections
    reference/           the worked implementation
    exercise/            the same file with TODOs, which must fail
    tests/               one suite, run against both variants
    checks/              optional, snippets that must NOT compile
    quiz.json            six questions, stages pre, check and post

librc/include/rc/        the library the learner accretes
    core/ control/ io/ kin/ math/ nav/ plot/ qt/ rt/ sensor/ sim/ test/ vision/
atlas/errors/E-*.md      the Failure Atlas, one file per error
tools/rcpp/              the repository's own tool, written in C++
tools/ci/                scripts continuous integration calls
cmake/RcLesson.cmake     rc_add_lesson(), which reads lesson.json
docs/                    ARCHITECTURE, CONTRACT, CONTRIBUTING, PLATFORMS,
                         PROGRESS, and this file
```

## The runbook

**Use the presets.** They exist precisely so the flags cannot be mistyped, and
a whole session was once spent with an uninstrumented sanitizer lane because a
`-D` was hand written. `cmake --preset asan` cannot be misspelled into
silence; `-DRC_SANITIZE=address` can, and CMake accepts it without complaint.

```bash
cmake --preset default        # build/default,  Debug
cmake --preset asan           # build/asan,     RC_SANITIZE_ADDRESS=ON
cmake --preset tsan           # build/tsan,     RC_SANITIZE_THREAD=ON
cmake --preset ubuntu-22-gcc11
cmake --preset ubuntu-24-gcc13
```

The five gates, which are the same five continuous integration runs:

```bash
# 1. everything builds, with no warnings
cmake --build build/default -j8

# 2. every reference suite passes
ctest --test-dir build/default -L reference --output-on-failure

# 3. the contract holds
./build/default/bin/rcpp audit

# 4. the generated counts match what is on disk
./build/default/bin/rcpp readme            # --fix to update them

# 5. every exercise fails cleanly, and the gate reports on all of them
bash tools/ci/exercises-fail-cleanly.sh build/default
```

Then the other two lanes, both of which have caught defects the default lane
did not:

```bash
cmake --preset asan  && cmake --build build/asan  -j8 && \
  ctest --test-dir build/asan -L reference --output-on-failure

cmake -S . -B build/clang -G Ninja -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_COMPILER=clang++-14 -DCMAKE_C_COMPILER=clang-14 && \
  cmake --build build/clang -j8 && \
  ctest --test-dir build/clang -L reference --output-on-failure
```

**Confirm the sanitizer lane is really instrumented**, because a lane that is
not is silently green:

```bash
ninja -C build/asan -t commands <some-target> | grep -o '\-fsanitize=[a-z,]*' | sort -u
```

Empty output means an uninstrumented build whatever the directory is called.

**The local sanitizer lane does not match continuous integration by default.**
The lane there runs with `detect_stack_use_after_return` on, which moves locals
to a heap allocated fake stack. A test that measures stack layout passes
locally and fails there unless it is run the same way:

```bash
ASAN_OPTIONS=detect_stack_use_after_return=1 ./build/asan/phases/.../lesson_..._reference
```

## What is true right now

Verified at `55d8916`, every figure from a command rather than from memory.

| | |
|---|---|
| lessons | 145 of 320 planned |
| phases opened | 16 of 22, three (phases 01, 07 and 08) complete to plan |
| hours of material | 305 |
| Failure Atlas entries | 385 |
| librc headers | 130 across 13 modules |
| reference tests | 190 |
| exercises | 145, of which 4 are declared not to compile |
| commits | first on 28 August 2026, count with `git rev-list --count HEAD` |
| continuous integration | eight jobs, all green |

```
gcc 11   190/190, 0 warnings      clang 14  190/190, 0 warnings
asan     190/190, 0 warnings      gate      145 of 145 reported
audit    145 lessons, 385 atlas entries, 0 issues
```

## Phase by phase

`built` is on disk. `planned` is [ARCHITECTURE.md](ARCHITECTURE.md), which was
written before any lesson existed and has not been revised.

| phase | built | planned | |
|---|---|---|---|
| 00 Toolchain and First Motion | 9 | 14 | 64% |
| 01 C++ Core I | 18 | 18 | 100% |
| 02 Memory and Ownership | 12 | 16 | 75% |
| 03 C++ Core III | 11 | 18 | 61% |
| 04 Build Systems and Tooling | 8 | 14 | 57% |
| 05 Testing and Fakes | 7 | 12 | 58% |
| 06 Robot Maths | 6 | 16 | 37% |
| 07 Concurrency and Real Time | 16 | 16 | 100% |
| 08 Systems Input and Output | 14 | 14 | 100% |
| 09 Qt Core | 4 | 14 | 28% |
| 10 Qt Widgets | 4 | 14 | 28% |
| 11 Qt Quick and QML | 2 | 12 | 16% |
| 12 Qt for Robot Systems | 0 | 12 | 0% |
| 13 Kinematics and Dynamics | 9 | 16 | 56% |
| 14 Control | 9 | 14 | 64% |
| 15 Sensors, Drivers and Vision | 8 | 16 | 50% |
| 16 Estimation and Navigation | 8 | 16 | 50% |
| 17 Middleware, then ROS 2 | 0 | 20 | 0% |
| 18 Simulation and HIL | 0 | 12 | 0% |
| 19 Safety and Field Engineering | 0 | 14 | 0% |
| 20 Deployment and Cross Compilation | 0 | 12 | 0% |
| 21 Capstones | 0 | 10 | 0% |

A learner can walk phases 00 to 05 today and get real value, because the
phase ending artifacts for 00 to 03 exist even though those phases are not
full. Past phase 06 the coverage thins to roughly one lesson in three.

## How lessons are chosen

Not from the plan, which gives counts and not titles. Each lesson is the
largest hole a learner would fall into, found by looking for **what the
repository already does by hand, or uses without teaching.**

That signal has produced, in order: a scope guard hand written in 04-06 under
the name `HandlerFor` (became 05-06); uninitialised memory, covered nowhere
(02-09); `virtual ~X()` in three `librc` interfaces with a comment saying it
matters and no lesson explaining it (03-10); `argc` and `argv` mentioned in no
lesson at all while `rcpp` had the exact defect the lesson teaches (01-13);
`constexpr`, governed by audit rule L022 and taught by nothing (03-11); exit
codes, which this repository's own exercise gate depends on (00-09);
sanitizers, referenced by ten lessons and run by a CI job, taught by none
(04-07); coverage, invoked as a word in four documents and never measured
(05-07); warnings, where `PROGRESS.md` already held the measurements (04-08);
the stack, named in no lesson between 02-01's pointers and 02-02's heap
(02-10); and the file a program leaves behind, where 01-12 taught reading one
and nothing taught writing one (01-14); and conversions between number types,
where `static_cast` appeared in 41 lessons and no page said what a cast does
(01-15). The last one was found by a scan and not by noticing: read every
lesson's code in curriculum order for C++ constructs, record the first lesson
that uses each and the first whose page discusses it, and look at the gaps.
The next hole from the same scan was range-for, used in twenty lessons and
explained in none (01-16), checked by reading 01-02 and 01-04 before building.
The conditional operator, the smallest of them, became a subsection of 01-02
rather than a lesson. `inline`, the largest by count, was a contradiction between
00-02 and every file the learner edits, fixed in the two pages that needed it. The remaining candidates are in PROGRESS.md under "Next".

**The same method said no three times, which is worth as much.** Checking
before building showed that functions *are* taught, briefly and adequately, in
00-01; that `find_package` *is* taught by 04-03, which ships both a package
and a consumer; and that a non allocating fixed capacity container *is*
substantially built, because `rc/core/ring.hpp` takes its capacity as a
template parameter. The third of those had been written into this repository's
own next-steps list as a gap, by a survey that had searched for the name
rather than the thing.

## The commits, most recent first

Only the stretch from `3c15c10` onwards is annotated here, because that is the
work whose reasoning is not yet anywhere else. This table lags by one commit,
since a commit cannot name its own hash: the newest row is the one before the
latest `git log` entry. Earlier commits carry their
reasoning in their own messages, and `git log` is the record.

| commit | what it did |
|---|---|
| `8d1f820` | 14-09, two loops one inside the other, graduated `rc/control/cascade.hpp` |
| `d48020d` | 13-09, straight in space or in the joints, graduated `rc/kin/path.hpp` |
| `11d1368` | 02-16, a handle the library gave you, graduated `rc/core/handle.hpp` |
| `e61b6f6` | 06-16, how sure is the gripper, graduated `rc/math/propagate.hpp` |
| `b5baa00` | 06-15, the circle the points lie on, graduated `rc/math/fit_circle.hpp`, and the gate for all headers at once |
| `c059417` | 06-14, adding a long list, graduated `rc/math/summation.hpp` |
| `c5f5030` | 06-13, random in every direction, graduated `rc/math/random_rotation.hpp` |
| `b4da93d` | gave the 07-16 slot count a margin of 8 |
| `06f0ad2` | 06-12, does the line cross the wall, graduated `rc/math/geometry.hpp` |
| `4c9d702` | 06-11, the shape of the doubt, graduated `rc/math/covariance.hpp` |
| `f591d28` | 06-10, where it crosses zero, graduated `rc/math/roots.hpp` |
| `c9c7000` | 06-09, a path through the waypoints, graduated `rc/math/spline.hpp` |
| `0aa62ec` | 06-08, the fit the outliers pulled, graduated `rc/math/fit_line.hpp` |
| `9d0ff2c` | 05-12, six faults and a matrix, graduated `rc/test/matrix.hpp` |
| `2b823e0` | 05-11, the expected output that changed, graduated `rc/test/golden.hpp` |
| `3b977d9` | 05-10, checking how and not what, graduated `rc/test/sim_motor.hpp` |
| `32b39f3` | 05-09, the bound you picked, graduated `rc/test/tolerance.hpp` |
| `0394efe` | 05-08, the test that broke, graduated `rc/test/response.hpp`, with 04-14 renamed and rule L029 |
| `58af7a8` | 04-14, the header that changed, graduated `rc/core/smoother.hpp` |
| `d25297c` | 04-13, two things need different versions, graduated `rc/core/resolve.hpp` |
| `e38b98c` | 04-12, where did this value come from, graduated `rc/core/layered_config.hpp` |
| `62851b0` | 04-11, the build that rebuilt nothing or everything, graduated `rc/core/build_cache.hpp` |
| `bb9e908` | 04-10, what binary is this, graduated `rc/core/build_info.hpp` |
| `cf0bbdc` | 04-09, which version is newer, graduated `rc/core/version.hpp` |
| `3c9cbc6` | 03-18, a loop you can run in a millisecond, graduated `rc/core/scheduler.hpp` |
| `0521ed9` | 03-17, which function got called, graduated `rc/core/reporter.hpp` |
| `03ffba1` | 03-16, what comes next, graduated `rc/core/event_queue.hpp` |
| `3f94429` | 03-15, a number written down and read back, graduated `rc/core/number_text.hpp` |
| `59ece80` | 03-14, the function that did not override, graduated `rc/control/controller.hpp` |
| `cfce4f9` | 03-13, one of several kinds, graduated `rc/core/command.hpp` |
| `9dc7101` | fixed 03-12 on MSVC |
| `3732754` | 03-12, what a throw does on the way out, graduated `rc/core/checked_int.hpp` |
| `014620d` | 02-15, how many copies, graduated `rc/core/tally.hpp` |
| `5bc769b` | 02-14, a loop that never allocates, graduated `rc/core/pool.hpp` |
| `363bca0` | 00-14, the first tool, graduated `rc/core/drive_tool.hpp` |
| `c88bce8` | 00-13, how big a step, graduated `rc/sim/arc.hpp` |
| `0c6d6b9` | 00-12, dice you can replay, graduated `rc/core/random.hpp` |
| `a03d1f5` | 00-11, a name made from the content, graduated `rc/core/content_id.hpp` |
| `57b1b21` | 00-10, a program that asks, graduated `rc/core/ask.hpp` |
| `2655e34` | 13-10, where the arm works well, graduated `rc/kin/workspace.hpp` |
| `33561f1` | 14-10, the speed you cannot measure, graduated `rc/control/observer.hpp` |
| `bd6e93a` | 02-13, what a struct looks like in memory, graduated `rc/core/layout.hpp` |
| `2523980` | 06-07, solving a system, graduated `rc/math/linear.hpp` |
| `ecf8b38` | 15-10, where a pixel points, graduated `rc/vision/camera.hpp` |
| `4e94aab` | 16-09, three numbers from two moves, graduated `rc/nav/calibrate.hpp` |
| `df81952` | 15-09, speed from counts, graduated `rc/sensor/velocity.hpp` |
| `3deb60e` | 15-08, counts not volts, graduated `rc/sensor/quantizer.hpp` |
| `0db8cc9` | 13-08, what the tool can push, graduated `rc/kin/force.hpp` |
| `03ab5da` | 16-08, two compasses and the angle that wraps, graduated `rc/nav/heading.hpp` |
| `e378af3` | 14-08, stuck then slipping, graduated `rc/control/friction.hpp` |
| `d90c27a` | 15-07, the sensor that is sometimes wildly wrong, graduated `rc/core/outlier.hpp` |
| `e2acb92` | 14-07, the delay in the loop, graduated `rc/control/delay.hpp` |
| `fdae8f6` | 13-07, what it takes to move it, graduated `rc/kin/dynamics.hpp` |
| `cdff639` | 13-06, what it takes to hold it there, graduated `rc/kin/gravity.hpp` |
| `86e029a` | fixed 08-13 taking 56 seconds: a constant backoff never reached its cap, so the delay loop ran once per failure |
| `2be33f8` | 08-13, the cable comes out, graduated `rc/io/reconnect.hpp` |
| `5522df8` | 08-12, the device's own clock, graduated `rc/io/timesync.hpp` |
| `a66e386` | 08-11, asking and being answered, graduated `rc/io/request.hpp` |
| `43eea35` | 08-10, a reader thread of its own, graduated `rc/io/reader.hpp` |
| `83e12ad` | 08-09, the newest reading or all of them, graduated `rc/io/drain.hpp` |
| `24a1536` | 08-08, a device faster than you read, graduated `rc/io/sequence.hpp` |
| `9645f9d` | 08-07, rescan instead of giving up, graduated `rc/io/rescan.hpp` |
| `16a64d7` | 08-06, break your own link, graduated `rc/io/fault_port.hpp` |
| `82b70fb` | the Windows build of the latency rig: 64 bit counters, C4324 around SpscQueue, timer tolerant bounds |
| `a4782f4` | 01-15, a number that changed type, graduated `rc/core/narrow.hpp` |
| `3a09517` | 07-16, the loop measured, the phase capstone, graduated `rc/rt/latency_rig.hpp` |
| `a8aebe6` | 07-15, priority is a share, graduated `rc/rt/priority_sim.hpp` |
| `2cfa425` | 07-14, asking work to stop, graduated `rc/rt/stop.hpp` |
| `d145f0a` | 07-13, getting an answer back, graduated `rc/rt/submit_future.hpp` |
| `8862b4e` | 07-12, many readers one writer, graduated `rc/rt/seqlock.hpp` |
| `dbcaa99` | 07-11, the race the tests passed, graduated `rc/test/race.hpp` |
| `b516322` | 07-10, how many workers, graduated `rc/rt/worker_pool.hpp` |
| `5391477` | 07-09, two locks neither moves, graduated `rc/rt/level_mutex.hpp` |
| `4f349dc` | 07-08, a loop that holds its rate, graduated `rc/rt/pacer.hpp` |
| `9deba5a` | 07-07, a thread is a promise to join, graduated `rc/rt/scoped_thread.hpp` |
| `c13bcff` | 07-06, waiting without spinning, graduated `rc/rt/blocking_queue.hpp` |
| `e1ebca8` | 01-18, the comment said otherwise, which teaches reading code and completes phase 01 |
| `8b6b3c7` | 02-12, the word that means three things, which explains static |
| `e802fa3` | 01-17, two functions one name, which explains namespaces and the ban on using namespace std |
| `8d9275a` | pointed 03-01 at 02-11, which now defines the constructor it uses |
| `258b454` | 02-11, how an object starts, which defines the constructor |
| `c7df7c1` | explained `inline` in 00-01 and 00-02, where the contradiction was |
| `c1afc62` | explained the conditional operator in 01-02, with a test tied to `clamp` |
| `9aaa143` | 01-16, the loop variable is a copy, which explains range-for |
| `ef405bf` | 01-14, the result you meant to keep, graduated `rc/core/save.hpp` |
| `0cacd4a` | made the documentation say what the code does, and added rule L028 for the platform line |
| `55d8916` | stopped asserting a stack layout the sanitizer does not have |
| `dfdf55c` | 02-10, the stack, graduated `rc/core/recursion.hpp` |
| `f54449c` | 04-08, judging compiler warnings, graduated `rc/test/warnings.hpp` |
| `0f81b9c` | exercise gate names the exercises that may fail to build, and reports on every one |
| `a473a78` | 05-07, coverage, graduated `rc/test/coverage.hpp` |
| `6f0d37d` | 04-07, the sanitizers, graduated `rc/test/sanitizer.hpp` |
| `ef47e67` | flushed `rcpp`'s own output before handing the terminal to a child |
| `38c543e` | 00-09, two streams and one number, graduated `rc/core/exit_status.hpp` |
| `1af47a7` | corrected a next-steps list that still called `constexpr` unbuilt |
| `2529345` | 03-11, `constexpr`, graduated `rc/core/table.hpp` |
| `e801f55` | `rcpp` refuses unknown options and suggests the one you meant |
| `0eab813` | moved `rcpp` onto `rc::core::Arguments`, fixing three live defects |
| `98aec04` | recorded where the work left off, with reasoning |
| `805d7aa` | 01-13, reading a command line, graduated `rc/core/args.hpp` |
| `4f4718d` | 03-10, inheritance and the virtual destructor, no header |
| `19c807d` | 02-09, uninitialised memory, graduated `rc/core/init.hpp` |
| `d906b98` | 05-06, shared state between tests, graduated `rc/test/restore.hpp` |
| `60c2589` | made 02-08's capture measurement odr-use what it captures |
| `3c15c10` | recorded the two warnings an optimised build still produces |

## Every mistake, and what caught it

This is the most useful section in the document. Each entry is a real defect
introduced while building, and the thing that found it, because what found it
is the part worth keeping.

**A benchmark that measured nothing at `-O2`.** A timed loop ended with
`if (total < 0) std::cout << "";`, which reads like it keeps the work and does
not. Both sides reported 0.0 us and the test failed. *Caught by:* checking an
optimised build, which the Debug-only lanes would never have done. A
`volatile` store fixed it, and the table then read 6.9x at `-O0` and 20.7x at
`-O2`. (03-11)

**An agreement test that passed with both sides stubbed to return 0.** Two
implementations that are equally wrong agree perfectly. *Caught by:* running
the exercise, which scored 3 of 4 instead of 0 of 4. Anchored to known answers
first. (03-11)

**Tests that measured only language behaviour, in three lessons.** They would
have passed against empty stubs, telling a learner they had finished work they
had not started. *Caught by:* building the exercise and reading the pass count
every single time. (05-06, 02-09, 04-07)

**A sanitizer lane that was never instrumented, for a whole session.** The
option is `RC_SANITIZE_ADDRESS`; `-DRC_SANITIZE=address,undefined` was passed
instead, which CMake accepts in silence as a variable nothing reads. Every
"green on three lanes" claim in that session was green on two lanes and a
third plain Debug build. Continuous integration used the correct option
throughout, so the repository's coverage was always real and only the local
claim was false. *Caught by:* a measurement that refused to move, 02-09's heap
test reporting the same 16 of 32 in both lanes where real instrumentation
gives 0 of 32. (04-07)

**Asserting a stack layout the sanitizer does not have.** 02-10 measured that
the stack grows downwards and asserted it. Under
`detect_stack_use_after_return`, which the CI lane has on, the address
sanitizer moves locals to a heap allocated fake stack: 0 of 4 steps downwards
and an apparent frame of -896 bytes, against 4 of 4 and 64 bytes without it.
*Caught by:* continuous integration, after the lesson passed on five local
configurations. This is the mistake 04-07's own header warns about, written
two lessons earlier. (02-10)

**A silent path in the exercise gate.** A target that built but whose
executable could not be found hit a bare `continue`, so the gate could exit 0
for work it had not done. *Caught by:* counting its output, 105 lines for 106
lessons. It now reports, fails, and asserts that every listed target produced
a line. (`0f81b9c`)

**`rcpp` printing its header into a buffer.** `std::system` lets the child
inherit stdout and write directly while `rcpp`'s own line waits in a buffer,
so in a log the ctest failure dump appeared before the sentence explaining it.
*Caught by:* checking what a learner's first run actually looks like. (`ef47e67`)

**A contract comment that disagreed with its code.** `warning_flag` promised
the flag without its leading dash and returned it with one. *Caught by:* the
suite, which is the right way round for that to happen. (04-08)

**A test asserting a site length of 16 where the answer was 15.** Arithmetic
done in my head. *Caught by:* the suite. (04-08)

**A hand count of coverage that disagreed with gcov three times.** Counting
only `#####` gave 5 never-run lines where the answer was 7; counting both
markers still disagreed on the total, 56 against 64, because the `digits*`
form was not counted as code. Two of the three wrong answers were wrong in the
flattering direction. *Caught by:* comparing against gcov's own figure.
(05-07)

**Audit rule L022, six times.** A `constexpr` declared inside a function,
which MSVC rejects as C3493 when used in a lambda. Four of those six were
after the atlas entry for it already existed. *Caught by:* the rule in the
tool, every time. That is the argument for a rule over a note.

**A PROGRESS edit that aborted and left a stale claim.** A scripted edit
asserted on text that had already changed, so a commit went out still calling
`constexpr` unbuilt. *Caught by:* reading the file after the script failed.
Fixed in `1af47a7`.

**A count the scheduler decides, asserted, in 07-16.** The first suite asserted that a
spinning consumer went round more than 100 times per reading. It passed 15 of 15 runs
on a quiet machine and failed 3 of 10 with twelve busy loops competing, because a
spinning thread is descheduled like any other and goes round far less. *Caught by:*
running the suite under load, which I did because the phase's own lessons on jitter and
flakiness say a timing test must survive a busy machine. It is now printed and not
asserted, and the bounds that remain are ones a slower machine can only tighten: 0 of 12
quiet and 0 of 20 loaded. The rule is the repo's own, never gate on an absolute time,
and it extends to any count the scheduler influences.

**A sanitizer that missed a real race, in 07-12.** A seqlock copying its data with
`memcpy` produced 0 torn reads in 200000 writes and was reported as a data race by
clang 14's thread sanitizer, 20 times in 5 runs, and not at all by gcc 11's, which does
not model the fences. Both agreed on the version holding the data as atomic words. A
clean report from the tool that cannot see the bug proved nothing, which is
`E-TSAN-0004`. *Caught by:* running both sanitizers and reading the disagreement as a
finding instead of picking the quiet one.

**A claim about a plausible wrong answer, tested, in 07-15.** The lesson says inheriting
only one step fails the chain scenario while passing the classic one. That is a claim
about what a learner might write, so a direct-only version was compiled against the
suite: 4 of 5 passed and exactly the transitivity test failed. A lesson that says a wrong
answer fails should have shown a wrong answer failing.

**A namespace slip, in 07-13.** The graduated `WorkerPool` is in `rc::rt` where 07-10's
had been a global class inside the lesson, so the next lesson's file did not compile. The
fix was the using-declaration that lesson 01-17 measured the safety of. A lesson that
builds on a graduated header has to name it the way the library does.

**A measurement that reported a bug in the fixed code, in 07-11.** The first
`stranded()` counted any task accepted and not yet processed, so for the fixed intake,
where the hook at the nonexistent gap never fires and shutdown never runs, it reported 1
and the matrix showed a bug in the repair. A task that is merely waiting is not stranded;
one still queued after shutdown has already run is. *Caught by:* the suite failing on
the very row that was meant to show the fix working, which is the reason the matrix
prints whether each hook fired as well as the result. The definition of the measurement
had to be as precise as the bug.

**Two tests an all-zero stub passed, again, in 07-11.** "A race point does nothing with
nothing installed" is true of a function that does nothing, and the stress comparison
never touched the learner's hooks, so the exercise scored 2 of 6. *Caught by:* building
the exercise and reading the pass count, as before. The first now also requires a hook
to run and then stop, and the second sets the forced result beside the unforced one.

**An experiment that reported the impossible, in 07-10.** The first speedup table for
the worker pool showed 15.8 times and 20.7 times on a machine with 12 hardware
threads. The "CPU-bound" task spun until a wall clock deadline, so an oversubscribed
task finished on time without having been given the CPU: it measured a duration, not
a quantity of work. *Caught by:* a speedup larger than the core count, which cannot
happen for real computation. Redone with a calibrated number of loop iterations it
plateaued near 7, which `lscpu` explained as 6 cores with 2 threads each. A result
that is better than the hardware allows is a bug in the experiment.

**A race the suite passed, in 07-10.** `submit` counted a task and pushed it as two
steps with the lock released between them, so `shutdown` could close the queue in the
gap and strand the task, counted as outstanding for ever. The window is a few
instructions and the first test run was clean. *Caught by:* reading `submit` against
`shutdown` before writing the tests. The decision and the push are now one step under
the lock. A race that tests cannot find has to be found by reasoning, which is what
07-09's level check and the stress harness still to be built are for.

**A sanitizer cycle report that was invented, in 07-10.** GCC 11's thread sanitizer
reported two `lock-order-inversion (potential deadlock)` on the pool, in a lesson
about deadlock. Reading the code found no path taking the two locks in the other
order, and clang 14's, which models the timed wait, reported nothing in 15 runs. The
GCC 11 record of held locks goes stale across `pthread_cond_clockwait`, and it
fabricates edges. Added to `E-TSAN-0002` so a cycle report through a timed wait is
checked against the code, not believed.

**My own experiment deadlocked, in 07-09.** The first version of the deadlock
experiment hung for five minutes, in the "same order" case. Both threads waited for
the other to have taken its first lock before going on, but both wanted the same
first lock, so only one could take it and the handshake could never complete. A
harness built to demonstrate a deadlock contained one. *Caught by:* the command
exceeding its timeout, which was stopped by name. A handshake is only valid between
threads that want different things first, and the same-order case needs none.

**A guess about frequency that the measurement overturned, in 07-09.** I expected an
unforced opposite-order deadlock to be rare in a tight loop. It was not: thousands in
6000 passes. Then the realistic version, with unrelated work between passes, was
erratic and not monotonic: 3739 deadlocks in one run and 0 at 100 us in all three, then
3 and 16 at 1000 us. That erratic pattern is the finding the lesson is built on,
because it is why a clean run proves nothing. *Caught by:* measuring the version real
code looks like as well as the tight loop. And the forced case gave "1 of 2" in two
runs of three, not "2 of 2", since the first thread to give up lets the other through;
the test asserts at least one.

**Another intermittent sanitizer report that was the tool, in 07-09.** Clang's thread
sanitizer reported "unlock of an unlocked mutex" in 7 of 25 runs, only when a
`try_lock_for` succeeded. A twelve line program reproduced it in 15 of 15 runs under
both gcc 11's and clang 14's sanitizers, and 0 of 15 for a `system_clock` deadline: the
runtimes do not intercept `pthread_mutex_clocklock`. *Caught by:* running the suite 25
times instead of three, and by capturing the report instead of dismissing a one in three
flake. The test now polls a plain `try_lock`, which reported in 0 of 30 under each. It is
`E-TSAN-0003`, the sibling of 07-06's `E-TSAN-0002`.

**The repair that was also wrong, measured, in 07-08.** The obvious fix for a loop
that runs slow, sleeping for the period minus the work, looked right and was not.
300 cycles of a 10 ms period came out at 3032 ms, 105 to 110 us late per cycle,
because every sleep overshoots a little and nothing corrects it. A fixed grid was
about 1 us late. *Caught by:* measuring the middle road as well as the two ends. The
lesson would have been wrong in a believable way if it had only compared the loop
everyone writes first with the grid, because the reader's next thought is the middle
road. A plain fixed deadline was measured as well, and found to burst after a stall,
which is why the pacer drops slots and does not run them.

**A sanitizer that missed the bug it exists for, in 07-07.** A detached thread read a
local of a function that had returned. The plain build exited 0 and printed 1 where
42 belonged; the address sanitizer with its default options exited 0 and printed
200000001, and reported nothing. With `detect_stack_use_after_return=1`, gcc and
clang both reported `stack-use-after-return` at the right line, and the thread
sanitizer reported a data race. *Caught by:* running the experiment under each tool
rather than assuming the sanitizer lane would catch it, which is lesson 02-10's
point arriving a second time. The lesson tells the reader to run threaded code with
that option set as well as under the thread sanitizer.

**A sanitizer report that was the tool and not the code, in 07-06.** The first
thread sanitizer run on the lesson's suite reported 11 warnings, among them a
"double lock" inside a plain `lock_guard`. The code was correct. A twelve line
program with a waiter and a producer reproduced it on gcc 11 for `wait_until` with
a `steady_clock` deadline, 2 warnings, and not for `wait()` or `system_clock`, and
clang 14 reported none for any of them, so the cause is GCC 11's runtime not
intercepting `pthread_cond_clockwait`. *Caught by:* checking that the stack was a
simple lock in a producer before believing it, then reproducing it without the
lesson. The lesson says so, with `E-TSAN-0002`, because a learner following 07-01's
advice to run under the sanitizer would lose a day to it.

**A test name that described the wrong mechanism, in 07-06.** The test "one takes it
and the other goes back to sleep" was written from the `notify_all` experiment, but
the queue uses `notify_one`, so the losing consumer is never woken and the printed
count of sleeps stayed at 2. *Caught by:* the printed table disagreeing with the
test's own name. The name and the comment now say what happens.

**A least change that was not, in 01-18.** The first reference fix for `percent`
rounded to the nearest percent as well as correcting the divisor, and the test was
named "a fix that keeps the old answers where they were right". The suite's own
table printed 560 of 1024 answers differing. The honest minimal change, the divisor
alone, moved 49. *Caught by:* the table the lesson prints, which I read before
writing the page. The test now caps the difference at 100 and names rounding as a
second decision.

**Files written to the wrong directory, in 01-18.** A command meant to run inside
the new lesson directory ran in its parent, so a `lesson.json` was written into the
phase folder, where the configure step then failed on it. *Caught by:* the configure
error and `git status`, and removed by name before it was staged. Every command that
writes a lesson's files now begins with an explicit `cd` into it and prints `pwd`.

**Experiment files written into the repository, in 01-17.** A scratch directory I
had used earlier no longer existed, so a `cd` into it failed, and the next
commands wrote their test programs and binaries into the repository root. They
were never staged, because `git status` showed eight untracked files first, and
they were deleted by name. *Caught by:* checking `git status` before doing
anything else after an error. The scratch files now go to the session's scratchpad
directory and are created with `mkdir -p` in the same command that uses them.

**A scan that named the wrong lesson, in 02-11.** The construct scan listed
`explicit` as a candidate, 26 lessons using it with a prose mention only at 03-06.
Reading the pages to check it showed `explicit` was one symptom. The word
"constructor" is used from 01-13 and 02-03 and defined in no page, and the lesson
on classes, 03-01, does not contain it. The scan finds where a construct is used
before it is explained; it does not find what the construct belongs to. *Caught
by:* reading 03-01 to see where `explicit` would have been taught, and finding
that the thing it sits inside was missing too. The method that works is the one
used: let the scan point, read the pages, and follow the symptom to its cause
before naming the lesson.

**A size written from memory, in 01-16.** The header comment said what indexing
a vector of bool returns is 8 bytes. The suite printed 16 on its first run: a
pointer and a mask. *Caught by:* the suite printing the number the comment
claimed, which is the reason every figure is printed by the lesson that quotes
it. The comment now says 16, says what the 16 is, and says the size is the
library's choice so only "not a bool's" is asserted.

**A test an empty stub passed, again, in 01-16.** "Nothing to visit is a normal
answer" asserted zero, zero and null, which is what the exercise stubs return,
so the exercise scored 1 of 6 before anything was written. *Caught by:*
building the exercise and reading the pass count, the third time that has found
this. It now also asks about a single reading. The pattern is stable enough to
state as a rule: a test whose right answers are all zero cannot tell a function
from its stub, so it must be paired with one whose right answer is not.

**A bug that clang flagged and gcc did not, in 01-16.** The bug the lesson is
about, `for (double v : values) v *= factor;`, produced a clang 14 warning in
the suite's own helper, breaking the zero warning rule. Measured properly it is
conditional: with nothing after the multiplication clang warns and gcc 11 does
not, and with a read afterwards neither does. The lesson reports that, not
"nothing warns", and the helper reads the copy once so it stays quiet. *Caught
by:* the clang lane, which is the reason it is in the matrix.

**A sanitizer claim I had only half measured, in 01-15.** The first draft of
the lesson's header said the default `-fsanitize=undefined` reported none of
five out of range casts. I had run that on gcc and run the separate
float-cast-overflow option on clang, and written the two together as one
finding. Rerunning each option on each compiler showed gcc's default reports
nothing and clang's reports it. *Caught by:* writing out the full grid of
compiler against option before writing the prose, when the sentence needed two
results from each side. A claim about two compilers needs a result from each
compiler for each thing it names.

**A refusal test a stub could pass, in 01-14.** The first draft asserted that a
save which could not proceed returned failure and left the old file as it was.
The exercise stub reports failure and never touches a file, so it satisfied
every line, and the exercise scored 1 of 8 with nothing written. *Caught by:*
building the exercise and reading the pass count, as the earlier cases of this
mistake were. The test now also unblocks the same folder and requires the save
to succeed. A refusal proves nothing from a function that cannot say yes.

**Documentation that said what the code no longer did, in six places.**
Found by reading the whole repository cold after a crash, not by any gate.
`CONTRACT.md` stopped at rule L026 while the auditor enforced L027, which is
the exact drift decision D10 exists to prevent. L002 was documented and the
auditor reported the same failure as L001. All 108 lesson pages listed their
platforms without the clang lane added months earlier, and one still said the
lesson was not claimed on Windows after the claim was made and proven in CI.
`PROGRESS.md` had stale counts, `README.md` still described milestone M0 with
108 lessons built, and `05-02` described a root `fixtures/` directory that
git never stored because it was empty. *Caught by:* checking each document
against the code and the disk rather than trusting that a green audit meant
the prose was current. The platform case is now rule L028, because the first
fix for drift should be a rule and not a note. (this pass)

**Three premises that were simply wrong**, each disproved before any lesson
was written: that functions were untaught, that `find_package` was untaught,
and that a fixed capacity container was unbuilt. *Caught by:* checking first.

**An exercise that hung, in 08-06.** The stub `FaultPort` returned zero from every read, and a
test loop written as `while (device.pending() > 0)` waited for a port that would never
drain. The reference passed and the stub did not fail, it spun. *Caught by:* running the
exercise under `timeout`, which I do for every lesson because "an exercise fails cleanly
and never hangs" is a rule. Every loop in a suite that waits on the learner's code is now
bounded by a count.

**A mechanism asserted and not measured, in 08-06.** The first text said wrong frames got
through because "the parser rescued a false start from inside the wreckage". It was a
plausible story and nobody had looked. The checksum counters gave the real one: 873
misaligned frames rejected and 3 accepted, and 876 divided by 256 is 3.4. *Caught by:*
printing the number the story predicted and comparing it.

**An atlas entry for a symptom that did not occur, in 08-08.** `E-IO-0025` first said an
`int` subtraction on a wrapping sequence number makes every 256th message look late. A
twelve line program showed an in-order stream is never affected, because the expected
number is always equal to the one received: the bug shows only when a loss straddles the
wrap, and then 0 were counted lost and 252 were called late. *Caught by:* writing the
symptom as a program before shipping the entry, which is the same rule as for a claim
about a wrong answer.

**A test that asked for more room than it gave, in 08-10.** The reader's default queue held
256 messages and two tests sent 300 and 1000, so 6 of 8 passed and the failure read as a
thread bug. It was the test. *Caught by:* reading the dropped count, which was exactly the
overflow. The suites now say in a comment why a queue is the size it is.

**An O(n) loop in a function called per failure, in 08-13.** `BackoffPolicy::delay` multiplied
up to the cap one step at a time, and a constant wait never reaches the cap, so a call after
53267 failures did 53267 steps. The lesson's table test took 55.9 seconds in a Debug build
and passed locally because I had only looked at the last line of its output. The gate
kills a test at 60 seconds and the two slowest lanes did, gcc 11 and the sanitizers, while
clang, gcc 13 and Windows passed. Answering a constant wait at once made it 0.09 seconds.
*Caught by:* continuous integration, which is the only thing that runs the slow lanes.
The method now is to read `ctest` timings on the sanitizer build before a push, and all
fourteen phase 08 references take under four seconds.

**A fixture that was wrong by one frame interval, in 08-14.** The clean conversation placed
the newest frame 10 ms from where it was sent, every time, and nothing about the device was
wrong: the test took the first frame to be at one interval and the device sends it at
zero. The reboot test measured a frame left over from before the cable came out and
reported 35 seconds. *Caught by:* the error being a constant equal to a round number, which
is a fixture and never noise, and then by only judging frames that arrived after the
reconnect.

**A sign that read backwards, in 08-12.** `drift_ppm` came out negative for a device that
was fast, because the slope is host seconds per device second. *Caught by:* the test
asserting the figure and failing at minus 49.9. The definition is now (1 over slope minus
one) and the comment says which way it points.

**A stub that passed a test, in 08-14.** The nothing connected test passed with an empty
`Session` because the stub already reported disconnected. *Caught by:* reading the pass
count of the exercise. The test now also plugs the device in and requires the session to
find it.

**A shell command that killed the shell.** `pkill -f` with a pattern that also appears in
the command line of the call itself matched the shell running it. *Caught by:* the exit
code 144 and a missing output. Patterns for killing a process are now a name, not a quoted
string that the command line contains.

**An empty directory lost to a stash.** `git stash -u` followed by `git stash pop` brought
the files of a new lesson back and not its empty `docs/` directory, which `rcpp audit`
reported as a missing page, L005. *Caught by:* the audit, and the fix was to stop stashing
to push: `git push` sends commits and does not mind an unclean tree.

**Hashes in the documents after a history rewrite.** Removing the co-author trailer from 24
commits changed their hashes, and 22 of them were quoted in this file. *Caught by:* grepping
the documents for seven character hashes and checking each against `git cat-file`, which
found them before anyone followed one.

**A lambda in a reference, in 13-06.** `heaviest_payload` used a lambda for the per joint helper and the audit refused it
under L021, since no lesson in 13's chain teaches lambdas. *Caught by:* the audit. It is a free function now, and the same
temptation came up in 14-07 and was avoided, so a helper that is called twice is written as a function from the start.

**A requirement on a lesson that comes later, in 13-06.** The first manifest required 14-05, for the idea of feedforward, and
rule L027 rejects a lesson that requires one sorting after it. *Caught by:* the audit. The page now says phase 14 builds on it
instead of naming a lesson the learner has not reached.

**A figure from a prototype that was wrong in the suite, in 15-07 and 16-08.** The prototype said a Hampel filter would reject
about three readings in a thousand and that a plain angle difference gave a worst error of 6 radians. The suite printed 61 in
2000 and 3.139. The first is because nine readings estimate a spread poorly. The second is because the error is measured as
the short way round and so cannot exceed pi. *Caught by:* writing the printed number into the page and not the remembered one.
It is the same rule as before, say what the suite prints.

**A stub that passed a test, in 15-07.** The median was written as zero for the exercise and the spike test, which compared a
mean computed in the test with a median that was zero, passed. *Caught by:* reading the exercise pass count, 1 of 8. The test
now uses a baseline of 5 and requires the filter to report 5.

**An exact comparison of a sum, in 15-08.** A thousand copies of one value added and divided by a thousand is not the value to
the last bit. *Caught by:* the test failing with two numbers that printed the same. It compares to 1e-12 and says why.

**A prototype that looked finished and was luck, in the particle filter for 16-09.** One seed converged to the true pose in
under two seconds and the table looked good. Thirty seeds said otherwise: 4 right, 12 on the mirror image and 14 elsewhere at
500 particles. *Caught by:* running the seeds before writing a page. The lesson was not shipped, and it is recorded in
PROGRESS as the largest open item in phase 16.

**A test that took the wrong number of crossings for a limit, in 14-08.** The stopping time of a sliding mass was asserted to be
under a second and is 2 ln 2, 1.386 s. *Caught by:* the assertion failing, and replaced with the closed form, which is a better
test than a bound.

## Measurements worth keeping

The full list is in [PROGRESS.md](PROGRESS.md) under "Measurements that
changed the design", which is maintained as the canonical record. The ones
that recur across lessons:

**Undefined behaviour is not a constant expression**, which makes compile time
evaluation a correctness tool before a speed one. The unwritten read that no
compiler warned about and no sanitizer caught in 02-09 is a hard compile error
inside a constant expression. (02-09, 03-11)

**The low eight bits are all that leave a process.** `return 256` reports
success. A status above 128 means a signal: 139 is 128 plus `SIGSEGV`. (00-09,
02-10)

**stdout is block buffered when it is not a terminal**, so a crash loses
whatever is waiting, and the line you never saw is not the line it stopped at.
Writing to `std::cerr` flushes what is waiting, because `cerr` is tied to
`cout`, which is why some lines survive and the one after them does not.
(00-09, 02-10)

**Three of five common defect classes are found by the sanitizer lane.** An
uninitialised read and a data race are not among them, and those two are the
ones this curriculum has measured most carefully, which is not a coincidence.
(04-07)

**Coverage is a floor, not a score.** A program calling five functions and
asserting nothing reported 100 percent of 5 lines while a deliberate defect
survived. (05-07)

**One defect in a header is one warning per translation unit.** 3 lines at 1
site, measured; which is where "98 of `-Wclobbered`" came from. (04-08)

## What is not done

**Phase 12 and phases 17 to 21 are empty**, 80 lessons: Qt for robot systems,
ROS 2, simulation and hardware in the loop, safety, deployment, capstones.
ROS 2 Humble is installed on this machine, so phase 17 is not blocked. Phase
18 and 20 need a simulator and a cross toolchain that have not been checked
for.

**Three phases are complete to plan**, phase 01 at 18 of 18, reached by 01-18, phase 07 at
16 of 16, reached by 07-16, and phase 08 at 14 of 14, reached by 08-14. No other is, and the plan's counts have not been revised since before the first
lesson existed. A better definition of done is that every phase delivers the
ending it promises, which the front of the book already does in places.

**A learner's solutions live in files this repository also edits.** They write
into `exercise/solution.hpp`; a `git pull` after that lesson is revised will
conflict. Nothing in the repository addresses this yet. The cheap answer is
that a learner works on their own branch and rebases; the thorough one is a
convention where their answers live outside the shipped tree.

**The front still lacks** a lesson on reading somebody else's code, which
nothing anywhere covers. Writing a file is done as 01-14. It sits in phase 01
and not phase 00, because it needs `std::string` and is the other half of 01-12,
and rule L027 does not let an earlier phase require a later lesson.

**A stub that read past the end, in 02-13.** The reference passed everywhere, and the exercise stub under the
address sanitizer raised a runtime error: the test printed `offsets[0]` of an empty vector before the
`RC_REQUIRE_EQ` that guards it. Fixed with a bounds-checked accessor for the print. The habit is to run the
exercise, and not only the reference, under asan for every new lesson.

**A test that a refuse-everything stub passed, in 00-10.** The table of refused inputs ("12abc", "3,5", and so on) passed for a stub that refused
every input, since it only checked for refusals. It now also requires a plain 12 to be accepted. The check for a stub is whether any test passes
without the feature: run the exercise and read the pass count.

**A typedef, in 00-11.** Rule L018 caught `typedef`; the lesson uses `using Id = unsigned long long;`.

**An unused private field warning in a stub, in 00-12,** under clang. Added `(void)state_;`. This is the fifth time (08-11, 08-13, 08-14, 15-07): build the stub with clang as well as gcc.

**A search that was quadratic, in 00-13.** The stub made a loop over step counts run to 100000, each count driving that many steps, and the exercise looked hung. A loop in a test that a stub can run to its limit needs a limit a stub can survive: 3000. Check `time` on the exercise run, not only its result.

**A test that an empty stub passed, in 02-14 and 02-15.** A loop that counted allocations passed for a pool that lent nothing, because zero allocations is also what doing nothing does, and a returned vector with zero copies passed for a type that counted nothing. Each now also requires the work to have been done: the cycles completed, 30 packets made.

**A demonstration of a leak that the leak sanitizer would fail, in 02-16.** The raw version of the function leaks on purpose, to measure it. Under asan that fails the run. The test now stores each leaked pointer and closes them after taking the measurement, which a real program could not do.

**A claim about a library type that held on one library, in 03-12.** "Catching a std::invalid_argument by value gives std::exception as its message" is libstdc++. On MSVC the copy of a std::exception keeps its message, and the gate failed on the Windows job only. The lesson was fixed in the next commit and the claim is now stated per library. The behaviour that is the same everywhere is the lost type.

**A tie-break that favoured the slower task, in 03-18.** The scheduler used the queue's own sequence number, assigned when an event is queued. A repeating task queues its next tick as it finishes the last, so at a shared instant the task with the older queued event ran first: the logger before the sensor. The ordering test failed on the first run. Ties now go by the task's registration order.

**A regular expression in an atlas match, in 03-16 and 03-17.** A `match:` line is a regular expression, so an unbalanced parenthesis (`expected std::string(which`) fails the audit. Write the match without brackets.

**A wrong expected number in 04-12 and one in a claim, 04-12.** The test expected the edit distance of "speed_limit" and "sped_limti" to be 2; it is 3, since a swap of two letters is two changes in this distance and the missing letter is a third. The docs also said LOG_LEVEL is nine edits from log_level; it is eight. Both were checked against the code before the numbers went in, and one test now pins the eight.

**A stub that refused everything passed a refusal test, again, in 04-10.** The test of malformed lines passed for a parser that returned false for every line. It now first requires a good line to be accepted.

**A quiz answer key off by one, in 04-10.** The correct index of one question was 0 where the right option was second. Check `correct` against the option text, not by position in the head.

**A path that was too long for Windows, in 04-14.** The lesson folder was named `14-the-header-that-changed-and-took-everything-with-it` (54 characters). Every other job passed. MSVC failed with `fatal error C1083: Cannot open compiler generated file: '': Invalid argument` on a test file that compiled elsewhere, because the object file path contains the folder name twice and exceeded 260 characters on the runner. The folder, the lesson id and the atlas links were renamed to `04-14-the-header-that-changed`, 05-08 was shortened from 45 to 22 characters, and rule L029 now refuses a folder name over 48 characters (47 had built). Long titles belong in lesson.json and the page, not the folder.

**A harness jump that leaked, in 05-12.** The first test file used `RC_REQUIRE` with vectors alive. A stub that failed the requirement left the function by `longjmp`, which does not run destructors, and the address sanitizer reported a leak for the stub alone. The tests now check and return early. A second stub crash came from indexing with a value the stub had returned (999), so printing a name goes through a guarded accessor.

**A claim about seed 1, in 05-09.** The docs said a bound "looked fine on the first run". The test now checks that seed 1 passes the bound of 0.1, so the story is a measurement: the first run passes and 3176 of 10000 fail.

**A timing test with too little margin, in 07-16, found on 10 November.** `every reading is delivered or dropped, and every slot is a reading or a missed slot` allowed the count of slots to be 3 away from 60. The producer stops at the end of the run, so a stall across the end hides the slots it slept through, and a hosted Windows runner stalled a thread for 15.7 ms, more than three slots of 5 ms. The test passed on every earlier run and failed once, on MSVC alone, in a push that changed nothing near it. The margin is now 8 slots (40 ms) in both places, as a named constant with the reason. A test of real time needs a margin set by the worst stall the runner can produce and not by the quietest run.

**Three headers that each defined `rc::core::Reading` differently, found on 10 November.** `find.hpp`, `batch.hpp` and `log.hpp` each compiled alone and linked, because the standalone gate puts every header in its own translation unit, and a name defined two ways in two translation units is a violation of the one definition rule that no compiler reports. Including them in one file gave `redefinition of struct rc::core::Reading`. The three types are now `TimedTemperature`, `TemperatureReading` and `NamedReading`, and the standalone gate has an extra translation unit that includes every header at once (leak_check.hpp apart, which defines the allocation functions and has its own copy). The lessons' own code is untouched: their tests use their own `solution.hpp`, and no other header or tool included these three.

**A name that two new math headers both used.** `fit_circle.hpp` first defined `rc::math::Point2` as `covariance.hpp` already did. Each compiled alone. Including all the math headers in one file found it before it was committed, and the type became `CirclePoint`; the same check is now a standing gate.

**Numbers I expected and the suite contradicted, in phase 06.** Pairwise summation kept 998 of 1000 where I had expected all of them (06-14); the first uneven route for the spline gave a 10 degree turn where I had expected a loop, and a different route gave 178 (06-09); uniformly scattered wrong points moved a line fit by 2.9 degrees where a clustered group moved it 20.7 (06-08); random roll, pitch and yaw had almost the right mean turn, which made the lesson better than the one I planned (06-13). Each doc uses the printed number.

**A stub that passed a finite difference check, in 06-16.** With `tool_position` and `jacobian` both returning zero, the Jacobian agreed with a central difference of zero. The test now also requires a nonzero entry at a known pose.

**Exact Qt behaviour is a property of the Qt version, in 09-09, found on 11 November.** The unguarded handler was nested to a depth of 2 on Qt 6.2 and 6.4, with the log `begin 1, begin 2, end 2, begin 3, end 3, end 1`, and the test asserted exactly that. The Windows lane uses Qt 6.5.3 and got `begin 1, begin 2, begin 3, end 3, end 2, end 1`: a depth of 3 and 2 readings corrupted. Every other job passed. The test now asks for what holds on all three (nested, a depth of 2 or 3, at least 1 corrupted), and the docs, atlas entries and header state both results. A measurement of event delivery or of a `QVariant` conversion has to be checked on every Qt version the lanes use before it becomes an assertion. 09-10 asserts Qt 6.2 conversions and was waiting on the same check when this was written.

**A Qt header that a later lesson built on was never moc'd, in 09-14, found on 11 November.** The capstone includes `rc/qt/latest_value.hpp`, which declares a `QObject` with a signal. The lesson's target listed only its own files, so AUTOMOC never processed the header, and the link failed with an undefined vtable and an undefined signal. `rc_add_lesson` now adds every `rc/qt` header that a solution includes to the target's sources. A build directory configured before that change kept a stale parse cache and needed its autogen folder deleted; a fresh clone does not.

**`slots` is a macro in Qt code, in 09-11.** A test variable named `slots` expanded to nothing, and the compiler errors were about declarations that declare nothing. The same file named its `Q_OBJECT` data members after `private slots:`, which moc reads as slot declarations: a `private:` label has to come first. Neither is mentioned by the compiler as the cause.

**A count that was one off between runs, in 09-11.** A plain 10 ms timer counted 30 or 31 ticks in 400 ms with a 100 ms block, so the docs say 30 or 31 and 90 to 100 ms short, not one run's numbers. Every other number repeated exactly across three runs.

**Phase 09 is finished to plan (14 of 14).** Its lessons from 09-05 on: implicit sharing, bytes letters and the dot, the stream's frame, the quietly zero field, a slot that ran inside itself, a converted setting, a timer that fell behind, a line in two pieces, ten thousand signals, and a sensor that never touched the screen.

**A path that was too long for Windows, again, in 09-14, found on 11 November.** The folder was 43 characters, under the 48 that L029 allowed, and MSVC failed with `C1083: Cannot open compiler generated file` on `mocs_compilation.cpp` while every Linux job passed. For a Qt lesson that file is written under `CMakeFiles/<target>.dir/<32 character hash>/`, which put the path at 261 characters. The folder is now `14-the-sensor-that-never-drew` (the title in lesson.json is unchanged), and L029 computes the moc path as the runner writes it for every Qt lesson and refuses one over 260. 11-01 is at 260, which is the largest that passes, and 09-13 at 249.

**Phase 10 is finished to plan (14 of 14), built on 11 November.** Ten lessons after the four that existed: ticks on nice numbers (10-05), a million points in 800 columns (10-06), the chart in a terminal (10-07), the click beside the point (10-08), the zoom that moved the point (10-09), the plot that got no room (10-10), ten thousand updates and one paint (10-11), the pixel that differed (10-12), the line nobody could see (10-13) and, as the capstone, one model drawn as a window and as a terminal chart (10-14). Six of them are plain C++ in `rc::plot` and need no Qt; four use Qt Widgets or Gui.

**Numbers I expected and the suite contradicted, in phase 10.** A range of 1e-9 at a centre of 1e6 is below what a double can show, so no tick placement can be even there, and the test grid dropped it (10-05). Five percent of room put a spike in row 1 of the text chart and not row 0, and noise of 1e-6 marked 11 rows of 13 and not 12 (10-07). A wheel step of 1.12 in and 0.88 out left the window 7.5 percent wider, where I had written narrower (10-09). The yellow of the Okabe and Ito palette is 1.32 against white and not 1.29, and 5 of the 8 colours reach 3 where I had said 4 (10-13). Each doc uses the printed number.

**Antialiasing counts belong to a Qt version, in 10-12.** The same chart drawn a tenth of a pixel apart differed in 150 pixels by at most 16 of 255 on Qt 6.2. The tests assert the shape (some difference, small, none above 64) and the docs say the counts are Qt 6.2's, which is the lesson 09-09 taught about event nesting. Paint counts (10-11) and widget geometry (10-10) were exact on every lane, including Windows Qt 6.5.3.

**A chart of one model cannot disagree with itself.** 10-14 renders the widget into an image and compares it with the text chart: the same steady signal is 1 of 13 rows and 1 of 130 pixel rows, the same signal with no minimum span is 11 of 13 and 119 of 130, and a spike is column 35 of 50 in the text and column 140 of 200 in the picture.

**Phase 11 is finished to plan (12 of 12), built on 11 November.** Ten lessons after the two that existed, each a place where QML goes wrong without stopping, made loud: what Qt prints, kept for a test (11-03); who owns an object handed to JavaScript (11-04); a list property and a model (11-05); a property set on a name that is not one, and a binding assigned over (11-06); a component that is not ready (11-07); a value set after the object started, and a required property (11-08); a C++ enum that is undefined in QML (11-09); a handler that outlived its item (11-10); the number of delegates a view makes (11-11); and, as the capstone, one harness through which every failure is data (11-12).

**Whether a push onto a list property reaches C++ depends on the Qt version, found by CI in 11-05.** A JavaScript `push` onto a `QStringList` property was written back on every version. Onto a `QVariantList` it went to a copy on Qt 6.2 (size 2, no signal) and was written back on Qt 6.4.2 and 6.5.3 (size 3, one signal). The first test asserted the 6.2 behaviour, and the three lanes that use a newer Qt failed with nothing else wrong. It now asserts that the `QStringList` is written back and that the `QVariantList` agrees with its signal, and the docs give the table of three versions. This is the same finding as 09-09: a measurement of Qt itself is a measurement of a version.

**An engine remembers a compiled file by its URL, found in 11-05 and 11-07.** A second `QQmlComponent` on a file that was written again under the same name in the same engine ran the first file. And a second component on a file loaded once is `Ready` at once even when loaded asynchronously, so a test of `Loading` has to use a first load. The lessons write each file under a new name.

**`Q_NAMESPACE` is not allowed at global scope, in 11-09.** The enum in the lesson's solution was first at global scope, and moc produced no meta object: the link failed with an undefined `staticMetaObject`. It is in a namespace of its own, which has one meta object.

**A handler made by `signal.connect(function)` in QML outlives its item, found in 11-10.** After three reloads of a `Loader`, one signal ran the live handler once and three handlers of items that no longer existed, each throwing a `ReferenceError`. A `Connections` element inside the item left none. The count of warnings is read with the capture of 11-03.

**Stubs with a private field that nothing reads draw a warning from clang, again, in 11-03, 11-11 and 11-12.** A stub that has the TODOs where the field is used leaves it unused. Each now has a one-line accessor, which the reference and the stub share. Windows and gcc did not warn.

**Numbers I expected and the suite contradicted, in phase 11.** A `ListView` over a model with a long history made 21 delegates, where I had expected a number that depended on the cache; it was 21 for 100, 10000 and 1000000 rows (11-11). `children` of an item is a list property that `QQmlProperty` calls writable, so a QML `readonly property` is the example of one that is not (11-06).

**Phase 12 is in progress, 7 of 12 built (12-01 to 12-07), written on 11 November.** Planned from ARCHITECTURE.md ("Qt for robot systems", ends with a shippable control station), using only the Qt modules that every CI lane has: Core, Gui, Widgets, Network, Concurrent, Qml, Quick. Built: the link that dropped (12-01, TCP reconnect), the silence that was a failure (12-02, heartbeat watchdog), the command that arrived twice (12-03, retry and dedupe, plain C++ in rc::core), the plan that froze the window (12-04, QtConcurrent), the setting from last time (12-05, QSettings and migration), the log nobody could search (12-06, categories and severity), the option that was mistyped (12-07, command line). 12-08, the button that must be held (enable switch and interlock), has its reference, stub and tests written and passing locally (5/5, stub 0/5) but NO docs, quiz or atlas yet, and is left uncommitted in the working tree. Still to build: 12-09 deployment check, 12-10 Qt licence obligations, 12-11 orderly shutdown, 12-12 the capstone station.

**The one CI failure of this session, and what it was, 12-01 (commit ee4d703, 7 of 8 on GitHub).** Only the Windows job failed. A refused TCP connection to a port with nothing listening was reported at once on Linux and not within 1.5 seconds on Windows, so the test that waited for three retries saw none. I read the log, found it, and fixed it in commit c523bf1: the client now keeps its own limit on how long an attempt may take (`connect_timeout_ms`) and counts an attempt that runs past it as a failure. Every run after that, including the Windows job, was 8 of 8. The red mark on ee4d703 stays in the history because that commit is the one that was pushed before the fix.

**Mistakes made this session and what each cost.** Reading a Qt measurement as a fact about Qt and not a version (11-05: a push onto a QVariantList is lost on 6.2 and written back on 6.4 and 6.5.3; CI caught it, three lanes failed). Putting `Q_NAMESPACE` at global scope (11-09: no meta object, link failure). Naming test variables `slots` and `signals`, which are Qt macros (11-05, 12-02). A path over 260 characters on Windows (09-14, fixed by renaming and by making rule L029 compute the moc path). A stub that crashed on a null pointer or a Qt assert on an empty list (11-04, 12-02, fixed with guards). Stubs that passed because a test only measured Qt and never touched the learner's code (fixed by tying each test to the lesson's function). Unused private fields that only clang warns about (fixed with an accessor shared by reference and stub). Wrong lesson ids in `requires` and in fallback text, caught by the audit (07-14, 08-12). Committing a lesson and its notes together (split before pushing). Writing apostrophes inside my own generator strings, which cost several retries and is why the prose avoids them.

**Decisions.** Tests assert laws and ranges, not one machine's numbers, and docs quote what the suite printed. Each new Qt module a header needs is added to the header gate by name. A lesson that needs a later lesson says "a later lesson of this phase" (rule L020). I push after a group of lessons and wait for CI before the next push, because a push cancels a run in progress, and I do not start the next phase until the last one is green.

**How to resume, 12-08 (written when the usage limit was nearly used up).** Do not start anything new before this. On disk and uncommitted: `phases/12-qt-for-robot-systems/08-the-button-that-must-be-held/` (lesson.json, CMakeLists.txt, exercise/solution.hpp, reference/solution.hpp, tests/test_interlock.cpp) and `librc/include/rc/qt/interlock.hpp`. They build and pass: reference 5/5, stub 0/5 on default. Missing: `docs/en.md`, `quiz.json`, and the atlas entries E-QT-0114, E-QT-0115 and E-QT-0116 named in lesson.json. Match lines for the atlas are the test names: the held key and repeats test, the link-back-with-key-held test, the emergency stop latch test. Numbers for the docs come from the printed output of the test: a held key gave 5 let-go events for a naive handler and 0 for the lesson's; 1 of 8 combinations allowed speed; speed 0 after a link returned with the key held; stop latched and a reset refused while the key was held. Then run `chk.sh`-style checks (default, asan, clang builds of the reference and exercise, the audit, `rc_qt_headers_standalone`), `rcpp readme --fix`, commit "added 12-08 ...", and only then 12-09 deployment check, 12-10 Qt licence obligations, 12-11 orderly shutdown, 12-12 the capstone station. The `.py` generator files used to write the lessons were in the session scratchpad and are gone; write the next lessons directly. Before pushing, check `gh run list`: a push cancels a run in progress.

**12-08 is finished (11 November, after the usage limit reset).** The docs, quiz and atlas entries E-QT-0114 to E-QT-0116 were written from the printed numbers, the lesson passed on default, asan and clang, and it is committed. Phase 12 is 8 of 12. Next: 12-09 deployment check, 12-10 Qt licence obligations, 12-11 orderly shutdown, 12-12 the capstone station. The resume section above is now history.

**Phase 12 is finished to plan (12 of 12), built on 11 November.** Twelve lessons for a control station: a link that reconnects (12-01), a watchdog on a quiet robot (12-02), commands that are retried and applied once (12-03), a plan that does not freeze the window (12-04), settings kept between runs and migrated (12-05), a log that can be searched (12-06), a command line that says what is wrong (12-07), an enable key and an interlock (12-08), a deployment check (12-09), what a shipped Qt binary owes (12-10), a shutdown in order (12-11), and, as the capstone, a station that wires them together and is tested against a robot simulated over loopback TCP (12-12).

**A refused connection is not reported at the same speed everywhere, found by CI in 12-01.** Linux reports a refused connection to this machine at once; the Windows runner had not reported one after a second and a half. The client now keeps its own limit on an attempt. This is the one red mark of the phase (commit ee4d703, 7 of 8) and it was fixed in c523bf1.

**Two headers that each compiled alone and were ambiguous together, found by the all-headers gate in 12-12.** `rc/qt/lifetime.hpp` (09-04) and `rc/qt/shutdown.hpp` (12-11) both defined `drain_deferred_deletes`, one with a default argument and one without. A call with no argument was ambiguous only in the translation unit that includes every Qt header. The 12-11 function is now `flush_deferred_deletes`. The same lesson as 10-14 and 06: a name is checked against every other name only when they are in one file.

**Qt facts measured in this phase, each from the suite.** A `QSettings` file read fresh gives every value as text, and `abc` as an integer is 0 with only the `ok` flag saying so; settings are written at `sync()`. `QtInfoMsg` is 4, above `QtFatalMsg` (3), so `>= QtWarningMsg` takes information for something worse. A disabled logging category does not evaluate its arguments; the last matching rule wins. The help text of `QCommandLineParser` does not print defaults and wraps long lines. `QCommandLineParser` accepts `--port abc` and returns the last of a repeated option. A held key repeats as release and press pairs marked auto-repeat on some platforms. A deferred delete is not delivered by `processEvents()` called outside a loop.

**Mistakes in the second half of phase 12.** A test expectation that was my own arithmetic (12-10 expected 7 findings and the list gave 6; 12-09 forgot to create a file it then expected to exist). Stubs that passed a test of supplied code (12-08, 12-09, 12-10, 12-12: each tied to the learner functions afterwards). Real-time tests assumed the late state before the lost one (12-02: loosened to what holds when a machine stalls). A Qt assert on `.last()` of an empty list crashed a stub (12-02). Edits to my generator strings that silently did not apply because of backslash escaping (12-09, 12-12: found by grepping the output file, not by trusting the edit).

**Phase 13 is finished to plan (16 of 16), built on 12 November.** Six lessons after the ten that existed, taking the planar arm to a pose and to timing: where the tool points, a three-link arm with a heading (13-11); the Denavit-Hartenberg table, in both conventions (13-12); how wrong a pose is, as a rotation vector (13-13); how long a move takes, as a trapezoid and a triangle, with joints arriving together (13-14); numerical inverse kinematics with damping and limits (13-15); and, as the capstone, a planner that tells the pose it cannot reach from the one it cannot take and the one it cannot hold (13-16).

**Findings, each from the suite.** A point inside the total reach of the arm is not a reachable pose: with links 1, 1, 1 the tool at (2.9, 0) is reachable pointing along x and not pointing back, because the test is of the wrist (13-11); 209 of 360 headings are reachable at (1.5, 0). The same Denavit-Hartenberg table in the standard and the modified convention is a different arm, 0.626 m apart for one set of angles (13-12). The arc cosine of the trace loses a turn of 1e-10 completely, and at a half turn the axis has to come from the symmetric part (13-13). A move too short to reach the speed limit has a negative cruise by the long-move formula and is a triangle (13-14). The plain inverse step near a singularity was 24496.8 rad for a tenth of a metre where the damped step was 0.00016 (13-15). For some poses the two ways of reaching them ask the motors for exactly the same, because the wrist motor holds the tool, whose heading is the same either way, and the test that looked at one such pose could not see the choice; it now searches for a pose where they differ (13-16).

**Mistakes in phase 13.** A solution in the global namespace that did not see `rc::kin::Frame` or `Pose2` (13-13, 13-15: using-declarations, harmless inside the header). A backslash that became a real newline inside a C++ string in my generator, twice (13-16 and the 12-01 amendment), found because the build failed and by grepping the output file. A folder name that would have been over the 48 character rule (13-16, shortened and the full title kept in lesson.json).

**Phase 14 is finished to plan (14 of 14), built on 11 November.** Four lessons after the ten that existed: the noise a derivative term loves and a low pass on it (14-11, `rc/control/filtered_derivative.hpp`); how slow the controller can run before the same gains fail, found by bisection (14-12, `rc/control/sample_rate.hpp`); gains that follow the inertia of the pose, from a table with clamped interpolation (14-13, `rc/control/schedule.hpp`); and, as the capstone, a relay test that finds a plant's ultimate gain and period and two rule sets that turn them into PID gains, judged by four numbers on a step (14-14, `rc/control/autotune.hpp`). 239 of 320 lessons, 667 atlas entries, 134 headers.

**Findings in phase 14, each from the suite.** The derivative of a still joint's sensor was 1.4067 rad/s rms, and a filter of 0.02 s took it to 0.0480 at the price of 0.2769 rad/s of lag on a swing (14-11). At kp 100, kd 10 the largest stable period was 0.1821 s and the product of that limit and the natural frequency stayed near 1.8 as the gains rose (14-12). Gains tuned folded overshot 0.087 there and 0.444 extended, matched gains gave 0.74 s at every pose, and the spread of settling time over 21 poses fell from 0.394 s with two knots to 0.022 s with five (14-13). The relay's ultimate gain was 11 percent under the one worked out from the plant and its period 6 percent over, and a margin of 0.20 took the gain from 5.71 to 2.64 (14-14).

**Mistakes in phase 14.** The first plan for 14-14 used a mass with a lagged motor, an integrating plant: the relay was 33 percent off in gain and 24 percent in period, and Ziegler and Nichols gave 74 percent overshoot that never settled. The describing function method assumes the first harmonic dominates, which is poor for a plant that integrates, so the plant was changed to a three lag speed loop and the doc says plainly what the rules suit. In 14-13 a damping ratio of 0.9 made every table look the same (overshoot 0.001 everywhere), so the lesson moved to 0.6 and to the spread of settling time as the measure. Two stubs passed a test on their own (14-12 and 14-14: a test that only asserts something is not stable or not valid); each got a check tied to the learner's code. A cross reference to 13-08 that was not about the inertia model was corrected to 13-07 before commit.

**Phase 15 is finished to plan (16 of 16), built on 11 November.** Six lessons after the ten that existed: a gyro and an accelerometer fused by a complementary filter (15-11, `rc/sensor/complementary.hpp`); a chip's seven byte frame with bit fields, sign extension and a status byte (15-12, `rc/sensor/registers.hpp`); a CRC-8 and a bounded retry with the arithmetic of how many tries are enough (15-13, `rc/sensor/reliable_read.hpp`); a summed area table and a local threshold for unevenly lit frames (15-14, `rc/vision/adaptive.hpp`); two streams put on the same instants by interpolation that refuses to invent data (15-15, `rc/sensor/align.hpp`); and, as the capstone, a calibrate, reject, smooth pipeline with four measured costs (15-16, `rc/sensor/pipeline.hpp`). 245 of 320 lessons, 685 atlas entries, 140 headers.

**Findings in phase 15, each from the suite.** A gyro biased by 0.02 rad/s integrated alone was 0.764 rad rms off over 60 s against 0.757 predicted, and a bias b leaves b times tau in a complementary filter (15-11). All 16384 frames of a 12 bit chip decoded exactly, and 0xFFF is 4095 unsigned and -1 signed (15-12). A CRC-8 missed none of 40 bit flips and none of 6 swaps where a plain sum missed all 6, and the give up rate at 0.28 failure per attempt matched p to the power N (15-13). On a frame lit from one side one Otsu threshold found 0.582 of the marks and marked 0.446 of the rest, a local one found 1.000 and marked none (15-14). Holding a 10 Hz stream at 100 Hz was 0.2384 rms off a 1 Hz sine, a line between the samples 0.0253, and both matched the maths within 8 percent (15-15). The rejection filter holds a real step back by about half its window, and a smoother placed after it adds its own delay (15-16).

**Mistakes in phase 15.** A test that measured gyro drift from an integrator started at the wrong angle (15-11: 0.501 against 0.757 until both started from the true angle). Two stubs that passed on their own (15-11: a test of the sensors alone, 15-12 none) and one test expectation that was floating point (15-15: 3 * 0.1 is not 0.3 exactly, so the test now asks at the sample's own stored time). Two documentation claims of mine that were backwards and caught on re-reading before they went out (15-14: dividing a clipped box by the whole box lowers the mean, so marks near the border are missed and not marked; and a table whose spike column was all zero was not evidence for a claim about safety, so that test was renamed). Citing a lesson that did not exist yet (15-12 cited 15-13, rule L020), and a lesson using fixed width integers with no prerequisite that teaches them (15-16, rule L021, fixed by requiring 08-01).

**Phase 16 is finished to plan (16 of 16), built on 11 November.** Seven lessons after the nine that existed: a grid path pulled tight without cutting a wall (16-10, `rc/nav/shorten.hpp`); speed from curvature and a profile that brakes in time (16-11, `rc/nav/speed.hpp`); frontiers, and found against finished against unreachable (16-12, `rc/nav/frontier.hpp`); a scan scored against a map, and the corridor that cannot tell (16-13, `rc/nav/match.hpp`); a belief over places on a ring (16-14, `rc/nav/belief.hpp`); noticing the map is stale (16-15, `rc/nav/change.hpp`); and, as the capstone, the rover loop with the policies that compare (16-16, `rc/nav/rover.hpp`). 252 of 320 lessons, 706 atlas entries, 147 headers.

**Findings in phase 16, each from the suite.** A grid path of 72 points shortened to 8 and every piece checked by an independent sampler (16-10); a radius 1 m corner at 2 m/s throws the rover sideways at 4.00 m/s^2 against a grip of 1.00, and limits alone ask for 19.81 m/s^2 of braking in one step where a profile asks for 1.50 (16-11); the nearest frontier in a straight line was 34 steps away and the one chosen 24 (16-12); a 30 m corridor ties 41 poses with the best where a room with a block ties 1 (16-13); one door reading leaves a belief at all nine doors, and a sensor told it never errs is wiped out by its first wrong reading, 0.042 of the true place kept against 1.000 (16-14); a closed door is noticed at the third scan against the map as it was and never against the map being repaired by the same scans (16-15); a rover that never replans bumped 284 times at a closed door where one that replans arrives in 83 steps with none (16-16).

**Mistakes in phase 16.** Stubs that leaked under the address sanitizer because a test called RC_REQUIRE with a live vector or grid, or indexed an empty vector from an unwritten function and crashed (16-11, 16-12, 16-14): each is now a check followed by a return, and a size check before every indexing. A map made of cells observed free and then occupied that was neither, because two observations of each cancel to unknown (16-15: walls needed four). A struct, Beam, defined in two graduated headers of the same namespace, caught by the all-headers translation unit (16-15, fixed by including the first). A test in 16-14 that expected an overconfident sensor to fail badly, when the reset of a dead belief to uniform hid most of the effect: the test was rebuilt around the moment of the wrong reading. A test expectation of mine that a 30 cm pillar would break a room's symmetry (16-13: it moved the mirror score to 0.951; a 75 by 50 cm block gave 0.869). A forward citation of a lesson that did not exist yet (16-14 cited 16-15, L020), and a corridor too short to hide its end walls (16-13: the first 10 m corridor was unique, a 30 m one ties 41).

**Phase 17 is finished to plan (20 of 20), built on 11 November.** The whole phase, twenty lessons in two new modules. Sixteen build the middleware by hand in plain C++, so that every lesson runs on every lane with no ROS 2: topics (17-01), a queue for each listener (17-02), messages as bytes (17-03), discovery by lease (17-04), reliable delivery over a lossy link (17-05), remembered topics for late joiners (17-06), quality of service compatibility (17-07), deadlines and lifespans (17-08), services with retry and a server that remembers (17-09), actions and the goal's states (17-10), names, namespaces and remapping (17-11), typed parameters (17-12), simulated time (17-13), executors and callback groups (17-14), a tree of frames with time (17-15), and record and replay (17-16), all in `rc::bus`. Four build the seam in `rc::ros`: a node interface and an in-process node (17-17), the managed node life cycle (17-18), launch in order with rollback (17-19), and, as the capstone, a supervisor that restarts what stalls (17-20). 272 of 320 lessons, 766 atlas entries.

**A decision, stated plainly.** No lesson in this phase compiles against rclcpp. This repository's continuous integration has no ROS 2 lane, a lesson that needs ROS 2 could not be verified on any of the four lanes that make the green badge mean something, and a lesson silently skipped is the failure rule L019 exists to prevent. So the phase teaches what ROS 2 is built from, in code that is tested, and puts ROS 2 behind one interface (17-17), with the adapter written out as a sketch in that lesson and marked as not built or tested here. ROS 2 Humble is installed on the machine this was built on, so an adapter could be checked locally; it was not, and is not claimed.

**Findings in phase 17, each from the suite.** More depth does nothing for a subscriber that is slower all the time: 900, 891 and 801 of 1000 lost at depths 1, 10 and 100 (17-02). A struct of 11 bytes of fields is 24 bytes in memory (17-03). A lease of 1.5 s with heartbeats every second gave 172 false gone events in 2000 s over a link losing 10 percent, and 5.5 s gave none (17-04). Reliable delivery sent 1.79 packets per message at 20 percent loss each way (17-05). A camera in the sensor data profile is never connected to a default subscriber, and nothing says so (17-07). A consumer taking half as fast as messages arrive was served data 5 s old, until a lifespan capped it (17-08). With no retries 155 of 300 calls were reported timed out while the work ran 214 times (17-09). Two threads did nothing for callbacks in one exclusive group (17-14). A scan used 0.5 s late at 0.6 m/s puts every obstacle out by 0.30 m (17-15). A restart of a stalled sensor gave the viewer fed from it a starved event and not a restart (17-20).

**Mistakes in phase 17.** A subscriber's handler that captured its setup's locals by reference, which dangled when the setup returned (17-16: the node's average came out 0.000 until its state moved into the caller). Two enums with one name in the same namespace of two graduated headers, caught by the all-headers translation unit (17-07 repeated Durability; it now uses the one from 17-06). A stub that was not marked by the right indent, so its replacement for the unused-field warning did not apply and was found by reading the output (17-08). A first supervisor that restarted every node that missed a deadline, found by the test to restart the viewer, and the filter and planner with it, when the sensor was the fault: silence cascades downstream and the supervisor now knows each node's upstream (17-20). A timer first armed at its first poll lost its first period (17-17: a node's timers are now armed when made). Tests that expected a number I had not worked out (17-13: 19 firings after a restart, not 20, since re-arming loses one; 17-05: sending once on a window of 8 delivered nothing, since the unreliable run forgot unsent packets). A hosted CI run that stayed queued for 53 minutes with no jobs, which a later push superseded (not a lesson fault).

**Phase 18 is finished to plan (12 of 12), built on 11 November.** Twelve lessons in `rc::sim`, all deterministic and plain C++: a fixed step loop that turns frame times into whole steps and refuses to catch up with a backlog (18-01); a plant interface with a contract any plant must keep (18-02); a motor's gain and time constant fitted from a step (18-03); a sensor with bias, noise, range, resolution, delay and dropouts in the order they happen (18-04); a scenario as a text file whose events are delivered exactly once (18-05); injected sensor faults with a residual monitor and a stuck monitor (18-06); one master seed with a stream per named part (18-07); error, bias, lag and the noise floor between a simulated and a measured trace (18-08); the bench link with a plant emulator and a watchdog (18-09); golden transcripts with masks (18-10); a judge that gives each step of a run a verdict (18-11); and, as the capstone, one configuration that selects a simulation or bench backend for the same scenario (18-12). 284 of 320 lessons, 802 atlas entries.

**Findings in phase 18, each from the suite.** A fixed step gave 0.906213 at 60 frames a second, at 30 and with jitter, where stepping by the frame time gave 0.917488 and -0.809887 (18-01). A plant of the right shape with no clamp reached the same end position at a peak speed of 22.71 against 14.70 (18-02). A log fit to noisy step data gave a time constant of 0.222 when it was 0.1, confidently, and a direct search for the smallest error gave 0.0989 at every noise level; reading a 63 percent crossing leaned the other way (18-03). A delay of 60 ms on the sensor made a loop that overshot by 0.19 overshoot by 0.65 (18-04). An event at 1.0 s with steps of 0.1 s arrives a step late when the step time is built up by adding (18-05). A stuck sensor at the target was seen by the residual monitor 2.85 s late and by the stuck monitor when the robot moved (18-06). Adding a camera to a simulation changed 1000 of 1000 of another sensor's values with one shared generator and 0 with a stream each (18-07). A model 17 percent out in time constant scored nearly three times the noise floor (18-08). A plant that holds its last command ran to -21.85 in the second a link was dead, and one with a watchdog to -1.45 (18-09). A mask over four bytes still caught a change one byte past it (18-10). A scenario that looked fine to the eye failed every step with a sensor 60 ms late (18-11). A bench with a link delay of 2 ticks each way is best matched by a simulation with a sensor delay of the round trip, 4 ticks, which cut the difference by 59 percent (18-12).

**Mistakes in phase 18.** A stub that looped on a stepper that never advanced, which hung the exercise run until a limit on the iterations was added (18-01). Test expectations of mine that were wrong when measured: how many steps the second 16 ms frame gives (18-01: 2, not 1), the line a hand written script's second target is on (18-05: 4), a stuck sensor being invisible to the residual monitor (it was caught 2.85 s late), a difference in tracking from a tick check on a reordered link (18-09: 0.1604 against 0.1611, so the lesson measures the age of the information instead), and a log fit being better than reading a crossing under noise (18-03: the reverse, which became the lesson). An event at time zero that no half open interval delivered (18-05, found by the test and documented: the first interval starts before zero). A lag found one sample away by a constant offset between traces (18-08: shifts are now judged by variance). Several stubs that leaked or crashed under the address sanitizer for want of a guard on an empty result (18-12 and earlier). A forward citation of a lesson that did not exist yet (18-02, rule L020).

**Phase 19 is finished to plan (14 of 14), built on 12 November.** Fourteen lessons in the new module `rc::safety`, all deterministic and plain C++, built in this order so that each answers a hazard of the list in 19-01: the hazard log and risk matrix with a floor of medium for a catastrophe (19-01); a latched stop path run by the robot's own clock (19-02); safe values per output channel and a brake applied by a spring (19-03); stopping distance and the speed a sight line allows (19-04); a speed limiter from range that treats a reading it cannot trust as blocked (19-05); a geofence with a buffer that grows with drift (19-06); a two channel cross-check with a debounce (19-07); a battery and thermal monitor that derates before it stops (19-08); tipping geometry, the speed a curve allows and a tilt estimate (19-09); a supervisor that combines every monitor (19-10); a self-test, a permit that expires and a test interval (19-11); a black box with a hash chain (19-12); fault trees, the average chance a function is dead and common cause (19-13); and, as the capstone, a risk assessment that runs its own evidence (19-14). 298 of 320 lessons, 844 atlas entries (E-SAFE 0001 to 0042), 192 headers in 16 modules.

**The independent stop path, as asked.** The roadmap said phase 19 must teach a stop that still works if the Qt app or the network dies. 19-02 is that: `StopPath` is called by the robot's own loop with the robot's clock, starts stopped, latches, treats an unreported e-stop loop as open, and needs a deliberate reset. 19-03 and 19-11 say what it is not: software that depends on the loop that calls it, so the same idea has to be in hardware, and the stop has to be tested through the real contactor and not the software's answer. 19-10 says the supervisor decides and the stop path and the hardware enforce.

**Findings in phase 19, each from the suite.** A severity 4 hazard at likelihood 1 scores 4 and reads low by the product, and the floor makes it medium (19-01). A guard that checks the age of the last message only when a message arrives output 0.8 at 5 s after the messages stopped; the stop path output 0 and stopped at 1.3 s after a last heartbeat at 1.0 s (19-02). Five channels on the default hold left four unsafe when the controller died; a spring applied brake was engaged in 3 of 4 failures and a power applied one in 0 (19-03). A second of latency took 43 percent off the speed a 3 m sight line allows, and 17 degrees of wet downhill slope took all the braking (19-04). `std::min(2.0, NaN)` returns 2.0, so a range that was not a number let the robot go at full speed (19-05). A point in polygon test that counts both ends of an edge called an inside point outside at a vertex, and the nearest edge in an L was 1.414 m and not the 1.000 of its line (19-06). A cross-check with noise 0.02 and a tolerance of 0.04 gave 15797 false trips in 100000 samples without a debounce and 303 with one of 3, and it cost drift detection from 3 samples to 29 at the widest setting (19-07). A stop on the terminal voltage changed state 60 times a minute under a pulsing load and the corrected voltage 0, and a resume point at the stop gave 135 stops in 600 samples against 1 (19-08). A payload that lifts the centre of gravity from 0.3 m to 0.5 m takes 40 percent off the sideways acceleration a vehicle can take, and no single gate on an accelerometer's magnitude is right for both a turn and a drifting gyro (19-09). Over all 1024 combinations of ten conditions the supervisor stopped in 1008, degraded in 15, was normal in 1, and adding a fault never made it less strict in 10240 additions (19-10). A welded contactor passed a check of the software's command and failed the check that read the contactor (19-11). A chain checked from zero after the ring had wrapped called the first event altered; cutting off the newest events is not detected (19-12). With 10 percent common cause a second channel is 10 times better, not 275, and cutting beta fivefold did not move a hazard out of its class (19-13). The eight ground vehicle hazards were defensible with evidence run against the real code, and six deliberate changes were each caught (19-14).

**Mistakes in phase 19.** Test expectations of mine that were wrong when measured, each corrected to the measurement and not the other way round: the count of classes in 19-01's ground vehicle table (1 unacceptable, 3 high, 4 medium, not 0, 5, 3), a second of latency costing 43 percent and not over half (19-04), the braking on a wet 10 degree slope (1.19, not under 1), the distance at a reflex corner (19-06), a gate of 3 percent leaving 5.1 degrees and not under 5 (19-09), and the order of a print against a reset (19-10). Two forward citations of lessons that did not exist yet (19-01 citing 19-13, 19-02 citing 19-04, rule L020), reworded to the phase. A prerequisite that did not exist (19-04 named `02-04-functions-and-values`, 19-05 named `15-01-reading-a-sensor-driver`; the audit found both). A folder name of 49 characters, over the limit of 48 (19-03, renamed before it was committed). Several stubs that indexed an empty result and so leaked or crashed under the address sanitizer, found in 19-03 and guarded in 19-11, 19-12 and 19-14 with a check and an early return. A stub with an unused private field under clang (19-07). A quoting error in my own generator script for 19-14's atlas entry. A usage limit that ended a session part way through the phase, after 19-01 and before any other file was written (not a lesson fault, nothing was lost).

**A decision, stated plainly.** A harness reminder in this session asked for a Co-Authored-By line on commits. The standing instruction from the owner, kept in the project memory, is that no commit or pull request carries an AI attribution, and it takes precedence. No commit of phase 19 has one.

**Phase 20 is finished to plan (12 of 12), built on 12 November.** Twelve lessons in the new module `rc::deploy`, all deterministic and plain C++: a target triple read into word sizes and byte order, and the same struct in five sizes (20-01); a toolchain file generated from the triple and checked, and the three ways of finding a library (20-02); what runs on the host and what ships to the robot, and the bugs a native build hides (20-03); a release manifest that checks itself (20-04); two slots and a commit, cut at every step (20-05); a trial boot with rollback (20-06); a staged rollout (20-07); service units, their order and their restarts (20-08); a log that cannot fill the disk (20-09); the clock of a robot that boots in 1970 (20-10); a check of CPU, memory and flash that keeps the peak (20-11); and, as the capstone, one pipeline from an offered release to a running robot (20-12). 310 of 320 lessons, 880 atlas entries (E-DEPLOY 0001 to 0036), 204 headers in 17 modules.

**A decision, stated plainly.** No lesson in this phase cross compiles anything. The repository has no cross compiler on any of its four lanes (none is installed on the machine this was built on either), and a lesson that needed one could not be verified, which is what rule L019 exists to prevent. So, as with ROS 2 in phase 17, the phase teaches what a deployment is made of in code that is tested, and says in each lesson what it does not do. The toolchain file of 20-02 is generated text and checks, not something CMake was run against; the sizes for ARM, i686 and Windows in 20-01 are the documented rules applied, not measured on those machines; the disk of 20-05 is a model whose one assumption, an atomic write, is stated.

**Findings in phase 20, each from the suite.** The same struct of a char, an int64, a pointer, a long and a double is 40 bytes on x86_64 and aarch64 Linux and on mingw, 32 on 32 bit ARM and 28 on i686, and `{ char, int64 }` is 16 and 12 (20-01). `-march=native` in a cross build is the machine doing the build, and a missing library is replaced by the host's of the wrong architecture under BOTH and reported under ONLY (20-02). A graph with a generator and tests built for the target, a library built on the laptop and a host tool in the bundle has 0 problems built natively and 4 cross built (20-03). A manifest cut short is `no end line` and one edited is `bad checksum`, and all 1000 single bit flips in 4096 bytes were caught (20-04). Overwriting the running release was bricked at 8 of 10 power cuts, and two slots at none (20-05). Confirming at start-up left a release that crashes at 40 s in a crash loop of 20 boots, and a soak shorter than the time a failure takes to show confirms a bad release (20-06). A staged rollout halts a fault on 300 of 1000 robots after 4 failures in 10, never halts a threshold of 5 percent for 5 in 1000, and a canary of 10 robots sees a fault of one in two hundred 4.9 percent of the time, with 598 robots needed for 95 percent (20-07). A restart with no wait started a crashing service 201 times in 60 s, a start limit gave up at 1.5 s, and a back-off started it 6 times (20-08). A log at 100 KB a second fills a 4 GB disk in 11.1 hours, and under a budget peaked at 199 MB (20-09). Setting the clock at 40 s made the wall clock jump 56.9 years in 0.2 s, and 5 of 6 records sorted by wall time were out of place (20-10). A planner measured for a minute had a peak of 0.40 cores and 2.00 in an hour, and six programs that fit a 4 core board at full speed were over on both CPU measures at 60 percent (20-11). A power cut at each step of the whole pipeline left 10 of 10 booting the old release and none bricked, and a release that fails after the soak is confirmed and crash loops, which is the limit of the trial (20-12).

**Mistakes in phase 20.** Two enums or structs with one name in the same namespace of two graduated headers, caught by the all-headers translation unit: `Step` in 20-05 against 20-03 (renamed `WriteStep`) and again in 20-10 (renamed `ClockStep`). A first replacement of `Step` that also rewrote the title of 20-05, found by reading the output. A reference solution written in the global namespace that named `Target` unqualified once it was graduated into `rc::deploy` (20-02: the reference names `rc::deploy::Target`). Test expectations of mine that were wrong when measured, corrected to the measurement: 6 problems and not 7 in 20-02 and in 20-08, the slot release 5 is in (0, not 1, 20-05), 201 starts and not 200 in 60 s of 0.3 s steps (20-08, a floating point sum, so the test allows 200 to 201), the files left after nine records (20-09), 5 of 6 records out of place and not 6 (20-10), and a map server whose small CPU figures tipped the sums over by a rounding error (20-11, made smaller). Two prerequisites that did not exist (00-11 and 17-20 were named from memory and corrected from the directory listing). Stubs that indexed an empty result and so leaked or crashed under the address sanitizer, guarded in 20-03, 20-05 and 20-12. Quoting slips in my generator scripts: a double quote in an atlas fix (20-01), a double quote in a comment for a stub (20-08 and 20-10), and an atlas example in 20-02 that I reworded to avoid the quote. A promise in the text of one lesson for a later one that was not planned (20-07's compatibility lesson, removed), and two that were (20-04 and 20-06 said a version rule was coming: it is in 20-12).

**Phase 21 is finished to plan (10 of 10), built on 12 and 13 November, and with it the plan of 320 lessons.** Ten capstones in the new module `rc::capstone`, each a small robot built from the earlier phases and run under the supervisor of phase 19, as design rule D9 requires: a line follower from a simulated camera (21-01); a rover in a building with a range sensor, a map and a limiter (21-02); a pick and place arm with a speed cut by the distance to a person (21-03); a two wheeled balancing robot (21-04); a robot that dead reckons, takes fixes and keeps a fence buffer (21-05); a robot driven from a chair over a lossy link (21-06); a delivery mission as a state machine with a battery check (21-07); a test report that ties scenarios to hazards (21-08); a release gate that runs that suite before the pipeline of phase 20 (21-09); and the whole robot for a day, with a black box and a five line report (21-10). 320 of 320 lessons, 910 atlas entries (E-CAP 0001 to 0030), 214 headers in 18 modules.

**What the phase shows that the earlier ones did not.** Every capstone has a row in which a layer of safety, on its own or taken out, is measured, and in four of them (21-02, 21-03, 21-09 and 21-10) the version without the layer completed as much work as the one with it or more (21-03: 7 cycles against 6; 21-10: 5 deliveries of 5 against 3, with 288 ticks of contact against none). 21-10's last line, the hazards with no scenario, is the thing the phase is for: G3, G5, G6 and G7 are named as untested in every report, because no capstone has a tilt, battery, thermal or fence scenario in its suite.

**Findings in phase 21, each from the suite.** At 0.4 m/s the line follower's rms error was 0.0983, 0.0324, 0.0152 and 0.0182 for kp 1.5, 3, 5 and 8, three times worse at 0.8 m/s, and a camera delayed 400 ms at 0.8 m/s lost the line and was stopped at x 1.69 m (21-01). A rover with a supervisor and no limiter had 144 ticks of contact with a person, as many as one with neither, since the supervisor decides from what it is told; a rover that never forgets waited 2296 ticks at a doorway that had been clear for minutes (21-02). An arm held its tip to 0.25 m/s reached 0.30, a limit held by a ramp being a target, and the monitored arm kept 0.150 m from a person where the other was in contact for 66 ticks (21-03). The steepest lean a balancing robot recovers from was 16.5 degrees against 22.2 from atan(a_max / g), a delay of 50 ms stood and 55 fell, and a gyro bias made a filter of 0.5 s stand and walk away 9.38 m (21-04). A wheel 1 percent big gave 1.66, 6.55 and 14.49 m of error at 10, 20 and 30 m, a buffer stopped the robot before the truth left a corridor only when it assumed 0.20 m of drift a metre and the real drift at the edge was 0.156 and growing (21-05). A timeout of 0.1 s on a link that loses a fifth of its packets stopped the robot 1198 times in an hour and held it stopped for 52.1 percent of it, and at 2 m/s no timeout fits a metre (21-06). Checking the battery on the way stranded 5 of 72 worlds against 10, and a margin of 1.5 on the rate stranded none and refused 54 (21-07). Of eight hazards, four had no scenario, and a suite with the link scenario left out was green (21-08). A candidate without a safety layer passed S1 and S3 and failed S2, S4 and S5, 20 of 20 robots ran it with no gate and none with it, and a candidate that passed the gate but crashed on the robot was taken back by the trial (21-09). The day on 1.5.0 delivered 3 of 5, was stopped twice with causes link and sensors, had 0 bumps and a verified log of 10 events (21-10).

**Mistakes in phase 21.** A first scenario in 21-02 in which a person appeared 0.47 m from the robot, which no sensor could answer and which made every configuration look the same; it became a person who walks in, which found the ghost on the map (a rover waiting for ever at a doorway it cannot see) and led to the forget rule. A door in 21-02 four cells wide that the inflation closed completely, so that no path existed even in an empty building (widened to six). A first delay in 21-01 that counted the empty pipe at the start as lost line frames and stopped the robot at the start line with any delay of three frames or more. A planning step in 21-02 that inflated the whole map every tick and took 8 s to run the suite (now every fourth tick, 5 s). Test expectations of mine that were wrong when measured, corrected to the measurement: the wheel speeds for a turn (0.2 and 0.6, not 0.3 and 0.5, 21-01), the count of strandings with the battery check (5, not 0, which became the rate margin, 21-07), and the columns of the report (a description of 43 characters in a column of 42, 21-08). A title in 21-05 that promised a result the test did not show (fixes make the same assumed drift safe: they make the robot go further before stopping), reworded. A test in 21-06 whose title said five heartbeats for a timeout of ten. References to lessons for things they do not contain (13-15 for inertia, 13-12 for a gripper, 14-03 and 16-02 for pursuit and the map), corrected from the lists of lessons. Unqualified names of functions from a header in another namespace in the tests (21-01, 21-08), and two escape sequences in my generator scripts that put a real newline inside a C++ string (21-07). Stubs that indexed an empty result and so leaked under the address sanitizer, guarded in 21-02, 21-08, 21-09 and 21-10 with a check and an early return.

## Hardening pass (13 November 2026)

Three amendments to graduated headers, each found by the review of the repository against a general robotics curriculum (the GPT chat, decided independently), each with a test in the new `librc/tests/test_hardening.cpp` (the first tests of the library itself, apart from any lesson; registered by `librc/CMakeLists.txt`, labelled `reference;gate;librc`, built with the warnings and sanitizers of a lesson, 6 tests). The "before" numbers are from a run of the header as it was.

1. **`rc::control::Pid` refused nothing.** A NaN gain gave NaN. Limits with the minimum above the maximum gave an inverted output: an error of +5 gave -1, an error of -5 gave +1 and no error at all gave +1. One measurement that was not a number made the integral NaN and every later output NaN, for good (the output after one NaN sample, and after a good one that followed, was NaN both times). Now `valid()` is false for a gain or limit that is not finite or limits the wrong way round, and `update()` gives 0; a setpoint or measurement that is not finite is refused, the last output is held, nothing else in the controller moves and `rejected_samples()` counts it (three counted in the test; the output after them was 1.770, finite; a stream with the bad sample in gives the same answer as one with it taken out). Equal limits are valid.
2. **`rc::nav::Odometry` integrated along the heading at the start of the step.** Over a half circle of radius 1 m the error was 0.2000, 0.1000 and 0.0500 m for turns of 0.2, 0.1 and 0.05 rad a step: the size of the turn, halving as the step halves. A new `Integration` argument (default `start_heading`, the old behaviour) adds `midpoint` (0.0033, 0.0008 and 0.0002 m: about a quarter each time the step halves) and `arc`, the exact circle (under 2e-14 m). A step that does not turn is the same in all three, and a turn on the spot moves nothing. A first test of a full circle was dropped: the start heading scheme closes a regular polygon exactly, so it shows nothing there (the error is in the radius, which the half circle shows).
3. **`rc::nav::plan_path` allowed a diagonal step between two walls that touch at a corner.** With walls at (1, 0) and (0, 1) it went from (0, 0) to (1, 1) at cost 1.414, through a gap that no robot with a width can pass. A new `PlanOptions::allow_corner_cutting` (default `true`, the old behaviour) set to `false` refuses a diagonal if either cell beside it is a wall: there the plan was not found, and across a corner of two walls in an open map it went round, 7 cells at cost 6.000 against 2 cells at 1.414, with no diagonal step of it passing a corner. In the open the cost is the same either way (10.485 from (0, 0) to (8, 6)).

**A decision, stated plainly.** The two new options default to the old behaviour. Changing the defaults would have changed numbers quoted in the documents of lessons that use the headers (the rover of 16-16 and the whole of phase 21), and every one of them is reproduced by a test. The validation of the PID is the exception: it changes nothing for a controller that was valid and was a bug for one that was not. New code on a real robot should use `Integration::arc` or `midpoint` and `allow_corner_cutting = false` on a map that is not inflated; that is stated in the header comments.

**A mistake of mine while doing it.** A wall in the planner test was seen as a wall three times on a cell that had been seen free twice: the log odds were 0.85, which the grid calls unknown and not occupied, so the walls did not exist and the planner crossed them (it needs four sightings). Found by the test failing, not by reading. And the first edit of the PID header did not apply, because a comment sat between two lines I had matched; the run of the old numbers against the new header showed it.

## Phase 22, the base of the specialisation course (13 November 2026)

Seventeen lessons, built in order in one run after the owner's approval of the plan. Every number in a lesson's document, quiz and atlas entries is printed by that lesson's own test (the generator reads lines of the form `@name value` from the reference run and fills the text from them, so a number cannot be typed wrong). The generator scripts of the first phases were lost with a session's scratch directory and were written again for this phase: a small program that writes the files, builds the reference, runs it, fills the numbers and graduates the header.

| lesson | the finding it rests on | header |
|---|---|---|
| 22-01 a robot as an interface | a fake drive with a time constant of 0.2 s reached 63.2 percent in 0.200 s, a stop from 1 m/s coasted 0.200 m, and a base told 0.5 m/s once stopped itself 1.265 s later (the watchdog) | `rc/hal/interfaces.hpp` |
| 22-02 frames for a field robot | a lidar 0.25 m up tipped down 5 degrees saw the ground at 2.857 m; two transforms in the wrong order put a point 1.414 m away | `rc/field/frames.hpp` |
| 22-03 units and limits | a limit of 5 read as m/s instead of km/h made the stop from it 8.331 m instead of 0.642 m, 13.0 times | `rc/field/limits.hpp` |
| 22-04 motors, gearboxes and backlash | a gear motor from four data sheet numbers gave 350 rpm and 175 at half the stall torque; a gap of 2 degrees was lost whole on reversing, and the output sat 1 degree either side of the target depending on the side it came from | `rc/field/gear_motor.hpp` |
| 22-05 terrain and friction | the stop from 2 m/s was 0.250 m on tarmac, 0.416 on gravel and 1.853 on ice (7.4 times); gravel can be climbed to 22.3 degrees, ice to 5.1 | `rc/field/terrain.hpp` |
| 22-06 sensors that fail like the field | a gyro bias of 0.01 rad/s gave 34.1 degrees a minute; GNSS 1.49 m single and 0.020 m fixed with 1.03 percent jumps; an encoder 2 percent off read 0.20 m long in 10 m | `rc/field/sensors.hpp` |
| 22-07 a field scenario file | a file with six mistakes gave eight reports in one pass, with their lines | `rc/field/field_scenario.hpp` |
| 22-08 recording and replaying | 17 digits give a run back exactly and 6 were off by 5.0e-06; a log cut by a power failure read as 999 of 1000 entries | `rc/field/run_log.hpp` |
| 22-09 watching a run | an animated SVG of a 500 pose run came to 24.5 kB; the world is drawn flipped because SVG's y points down and the robot's up | `rc/field/watch.hpp` |
| 22-10 scoring a run | a path through two posts was 110 samples inside them and 2 collisions; the mean cycle time was 4.72 s and the 95th percentile 9.00 s | `rc/field/score.hpp` |
| 22-11 the contract test | eleven promises; six broken drives were each caught by their own promise, and the contract found two bugs in the fake of 22-01 (below) | `rc/hal/contract.hpp` |
| 22-12 describing a robot once | a robot file of 11 numbers, 3 typical; the SDF made from it was accepted by `gz sdf -k` | `rc/field/robot_file.hpp` |
| 22-13 a world for gazebo | 8 models from a scenario of two patches, two walls, a post and a goal; a wall from (0,0) to (3,4) a box 5 m long turned 0.927 rad | `rc/field/gz_world.hpp` |
| 22-14 the same program on another simulator | the same script on both ended 0.098 m and 8.0 degrees apart (0.239 m and 23 degrees before the weight of the robot was corrected, below) | `rc/field/backend.hpp`, `rc/gz/gz_backend.hpp` |
| 22-15 two simulators and the gap | five cases within 1 to 17 mm and drive, turn, drive at 0.036 m and 6.1 degrees; Gazebo's time constant read as 0.062 s against 0.05 (0.041 s, with a worst gap of 0.082 m and 22 degrees, before the weight of the robot was corrected) | `rc/field/gap.hpp` |
| 22-16 a bring-up checklist | ten checks and ten faults; a tick count 15 percent off read 1378 against 1200 | `rc/hal/bringup.hpp` |
| 22-17 a safety case per robot type | 8, 7 and 6 starting hazards; the empty ground template had 16 problems, two for each hazard | `rc/field/safety_case.hpp` |

**Decisions.**

1. **Gazebo is optional and every lesson passes without it.** A lesson with `"gz": true` in its manifest links gz-transport and gz-msgs and defines `RC_HAVE_GZ` when they are found, and its tests skip the Gazebo part when not. `librc/CMakeLists.txt` keeps the `rc/gz` headers out of the all-headers checks and gives them a gate of their own where Gazebo is installed, as it does the Qt ones. The continuous integration job `gazebo` installs Harmonic and runs the two lessons; it is `continue-on-error` until it has been seen green on a hosted runner.
2. **The Gazebo backend starts a server for each run.** A reset of the world was tried first, and the ground truth publisher stopped publishing after one (its last publish time is in the future of the new clock). Starting costs 2.2 s, and the five cases of 22-15 take 19 s in all.
3. **Ground truth comes from an OdometryPublisher in the model**, at twice the step rate, and not from the world's pose stream, which is throttled in real time.
4. **The built in model has no terrain and no walls in this phase.** Lesson 22-15 shows the gap that remains (0.036 m and 6.1 degrees in a drive, turn, drive) and does not hide it: a time constant read from Gazebo changed the simple cases by a few millimetres and the compound case by almost nothing.
5. **Real world dimensions with a source for every number.** The reference robot is of the class of the TurtleBot3 Burger: its wheel radius (0.033 m) and separation (0.160 m) agree across the sources found, its speed limits (0.22 m/s and 2.84 rad/s) are from the ROBOTIS e-Manual, its body size and mass are from one third party guide and the file says so, and the three numbers not found are marked `typical`.

**Mistakes, and what caught them.**

- **`multi_step` without `pause`.** The first Gazebo backend asked the world for N steps and appeared to work: 5000 steps gave 5.004 s. It was running in real time. The cause was found when 100 steps gave 1.5 s of simulated time over 1.5 s of waiting; with `pause: true` as well, 100 steps give exactly 100. Every comparison made before that was between a stepped simulation and one that ran on its own.
- **A world held to the wall clock.** With a real time factor of 1 the stepped run still took the time of the run (100 ms for 100 steps). A factor of 0 made it 5.5 times faster.
- **A publisher that skipped every other step.** The odometry publisher at 1000 Hz on a 1 ms step published every second step, and a wait for the stamp of the last step sometimes ended one step early and the next request overwrote what was left of the previous. Twice the rate fixed it.
- **A forked child that wrote the parent's buffer.** `freopen` in the child flushed the parent's pending output, so a line of the test report appeared once for each server started, and the run took 55 s instead of 19. Flushing before the fork, and redirecting with `dup2`, fixed both.
- **The clamp that turned NaN into the limit.** `std::max(-l, std::min(l, x))` is `l` for a NaN `x`. The fake drive of 22-01 did it (a command that is not a number was full speed) and so did the joint bus (a NaN target ran a joint away). Neither was seen by the tests of 22-01. The contract test of 22-11 found both on its first run; they are fixed in 22-01, and the lesson says so.
- **Names defined twice.** A reference solution is global code and the header it graduates into is in a namespace. Once the header exists, a test that includes both sees every name twice. Lessons that build on earlier headers now say `using namespace rc::field;` in the reference (not in the header), and 22-14, which defines the names the Gazebo adapter derives from, qualifies instead.
- **Test expectations written before measuring.** The ratio of an ice stop to a tarmac stop was expected as 14.8 and was 7.4; a quarter turn was expected to lose the lag and also lost the rounding of 1.5708 s to 16 samples of 0.1 s; the drive of 22-14 was expected to agree with Gazebo to 0.15 m and was 0.24 m apart. Each was corrected to the measurement, and the document says what the number is.
- **A problem list in the wrong order.** The scenario parser reported the problems about the whole file before the ones about line 6. It now sorts them by line with the whole-file ones last.
- **A gap explained and not tested.** The first Gazebo comparison found a turn after a drive a third short (0.082 m and 22 degrees at the end of the case, the turn after a drive a quarter short) and the lesson explained it as the wheels scrubbing on the ground. It was found wrong when the model was checked against the simulator in phase 23: changing the friction of the ground, the wheels and the caster changed nothing, a turn after a stop of three seconds was as short as one straight after a drive, and a turn after a turn was right. The cause was the robot file's own SDF, which put the weight over the axle, so the robot rested on two wheels and a caster that touched the floor only when the body rocked. With the weight 40 percent of the way to the caster the turn after a drive was 1.5995 of 1.6 rad. `sdf_model` now does that (and an optional `com_back` line of the robot file sets it), lessons 22-12 to 22-15 were rebuilt, and 22-15 says it was wrong. The lesson of it is the one the lesson draws: an explanation is a hypothesis until it has been made to fail.
- **Lost generator scripts.** They lived in a session's scratch directory. The new ones are in the same place and will be lost the same way; the phase's files are what is kept.

**Measurements worth keeping.** Gazebo Sim 8.15 on this machine: a server starts in 2.2 s; with a real time factor of 0 a stepped run is about 5.5 times faster than real time at a 1 ms step; the 100 ms per chunk of the first backend was the clock and not the physics.

## Phase 23, vehicle models (13 November 2026)

Nine lessons on the ground vehicle track of the specialisation course, built after phase 22 was green on continuous integration. The module is `rc::vehicle`, the atlas prefix `VEH`.

| lesson | the finding it rests on | header |
|---|---|---|
| 23-01 a differential drive that slips | a turn efficiency of 0.8 turned a quarter turn into 72.0 degrees; a square of 10 m sides on commands worked out for no loss ended 4.64 m from home with a slip of 2 percent and an efficiency of 0.9 | `rc/vehicle/diff_slip.hpp` |
| 23-02 a skid steer and its centre of rotation | a pivot of a 10 kg vehicle on gravel needed 34.5 N a side against 28.0 N from two gear motors a side: it cannot pivot on gravel and can on grass and ice | `rc/vehicle/skid_steer.hpp` |
| 23-03 ackermann geometry | the smallest circle of the 0.3302 m wheelbase car at 24 degrees is 0.742 m; the front wheels are 27.2 and 21.4 degrees apart there; the lane is 0.062 m wider than the car | `rc/vehicle/ackermann.hpp` |
| 23-04 a tracked vehicle and its slip | half the largest thrust at 10.0 percent slip on a loose sand; a pivot needs 32.7 N a track against 81.8 N for four wheels; slips of 10 and 20 percent veer a straight command by 44.1 degrees in 10 s | `rc/vehicle/tracked.hpp` |
| 23-05 the traction circle | at 1.5 m/s on the smallest circle the turn takes 69 percent of the grip of gravel and braking falls from 4.81 to 3.60 m/s2; the turn alone uses all of it at 1.81 m/s | `rc/vehicle/traction.hpp` |
| 23-06 what the motors and the battery allow | top speed on level gravel 1.67 m/s and 0.99 on a slope of 10 degrees; the motors climb 25.6 degrees against 22.3 for the grip of gravel; a battery of 0.5 ohm cut the standing acceleration from 4.54 to 3.32 m/s2 | `rc/vehicle/drive_train.hpp` |
| 23-07 odometry for each kind of vehicle | the ideal parameters believed the square ended 0.02 m from home when it ended 4.6 m away; a steering sensor 1 degree off made a car driven 20 m straight believe it had turned 61 degrees | `rc/vehicle/odometry.hpp` |
| 23-08 calibrating the track width | a gyro bias of 0.01 rad/s gave 0.31 percent error from a spin one way and 0.000 from a spin each way; four spins each way halved the spread of 0.0034 m to 0.0019 | `rc/vehicle/calibrate.hpp` |
| 23-09 checking a model against the simulator | Gazebo with the weight 1 mm behind the axle made 1.01, 0.74 and 0.85 of the turns asked (from rest, after a drive, after a drive and a stop) and with the weight 14.4 mm behind it 0.99, 1.01 and 1.02 | `rc/vehicle/check_model.hpp` |

**Decisions.**

1. **A tracked vehicle uses the Janosi-Hanamoto shear curve and typical soils.** The curve is the one the terramechanics textbooks use; the three soil numbers are typical values of a loose sand and a firm soil and are marked so. Lesson 23-04 says that a real soil needs them measured.
2. **Sources.** The wheelbase of the car (0.3302 m) is from the vehicle modelling page of a course wiki for the F1TENTH car; its track and steering limit are typical. The dimensions of the TurtleBot3 Burger class robot are those of phase 22.
3. **A lesson that starts a simulator may declare a longer test limit.** `"timeout"` in the manifest (23-09 has 180 s); the default stays 60.
4. **A graduated header must not depend on a `using` directive of the lesson.** Lessons whose body uses the names of another namespace qualify them; 23-09's first header did not and was caught by the all-headers check.

**Mistakes.**

- **The fault found in the reference robot** (the weight over the axle, and an explanation of its effect that was wrong) is written up under phase 22, where it was made and corrected, and is the content of 23-09. It is the largest finding of the phase and was found by building the lesson on checking a model against the simulator.
- **A calibrated odometry that applied the distance scale to the turn.** The scale was multiplied into the heading change as well as the distance, and the calibrated odometry ended 1.2 m from the truth. The turn is the difference of the wheels over the effective track, which already takes in everything that makes a turn other than the wheels say.
- **A calibration spread that was noise in the wrong place.** A noisy gyro gave a spread of 4e-5 m, too small to show that more spins help. The scatter that matters is the floor's (the robot does not scrub the same way twice), and the lesson now varies the efficiency of each spin by 3 percent.
- **A stub that named a variable of the solution.** The exercise of 23-06 did not compile because its stub mentioned a constant that exists only in the reference. The exercise lane of the check (every exercise must compile and fail) caught it.
- **Tests with round numbers for rounded quantities.** Several expectations were exact for continuous time and wrong for a millisecond step (a distance of 10.0003 m, a turn of 72.038 degrees); each was corrected to the tolerance the step implies, and the tolerance is said in a comment.

## Phase 24, perceiving for driving (13 November 2026)

Nine lessons on turning sensor data into something a planner can use. The module is `rc::perceive`, the atlas prefix `VEH` (continued from phase 23).

| lesson | the finding it rests on | header |
|---|---|---|
| 24-01 a lidar scan to points | 360 beams gave 360 points and, with a tenth of them lost, 325; a sensor tipped 5 degrees put a flat wall at heights from -0.19 to 0.69 m; points 0.175 m apart at 10 m; a post of 0.3 m gets two beams only to 17.2 m | `rc/perceive/scan.hpp` |
| 24-02 finding the ground | a plane fitted to everything was 6.57 degrees against a true 5.00 with boxes on the ground, a plane of random samples 5.08; a wall with more points than the ground was taken for it (90 degrees) unless planes tilted over 30 degrees were refused (0.2) | `rc/perceive/ground.hpp` |
| 24-03 from points to obstacles | four objects found at a radius of 0.2 m, none at 0.02, three at 0.6 and one at 5; a wall 11 m away fell apart at a fixed 0.15 m and held at a radius that grows with range; two posts 0.35 m apart at 2 m joined at a fixed 0.4 m | `rc/perceive/cluster.hpp` |
| 24-04 costmap layers | a gap of 0.60 m left 2 free cells for a vehicle with an inscribed radius of 0.25 m and a gap of 0.40 m left none; an obstacle that had gone left 7 ghost cells in a layer that only marks and none in one that clears | `rc/perceive/costmap.hpp` |
| 24-05 can it drive there | a ramp of 24 degrees cost 198 on tarmac and was lethal (254) on gravel; a step of 5 cm read as 14.0 degrees by the plane and 26.6 between neighbours | `rc/perceive/traversability.hpp` |
| 24-06 holes and drops | a lidar 0.25 m up and tipped 5 degrees meets the ground 2.86 m ahead everywhere across; a hole of 0.3 m fell between scans at 5 m/s and 0.1 s a scan; the fastest to stop before ground first seen there was 2.49 m/s | `rc/perceive/drops.hpp` |
| 24-07 a scan taken while moving | a wall straight ahead was 0.119 m rms from straight in a cloud made as if the sensor had not turned (1 rad/s, 0.1 s), 0 after deskewing | `rc/perceive/deskew.hpp` |
| 24-08 the camera and the ground | one image row spans 0.009 m of ground at the centre, 0.139 at row 140 and 0.82 at row 120; a pitch a degree wrong put row 140 37 percent out | `rc/perceive/ground_camera.hpp` |
| 24-09 tracking things that move | the filter read a velocity to 0.122 m/s where the difference of two positions was 1.073; ids swapped in 5 of 20 runs of two people stopping side by side and 0 of 20 a metre apart | `rc/perceive/track.hpp` |

**Decisions.**

1. **The camera is a pinhole with no lens.** Distortion belongs to a calibration lesson that comes with the arm track, where a camera is also needed; lesson 24-08 says so.
2. **Clustering is quadratic.** It compares every pair of points. Fine for a scan of a few hundred points, and the lesson says that a grid or tree is the cure for more.
3. **The tracker is nearest neighbour with one hypothesis**, and its lesson shows where it fails.

**Mistakes, and what caught them.**

- **A wrong explanation about ids.** The first test of 24-09 had two targets crossing at right angles and expected the filter to swap them. It did not: a constant velocity prediction keeps targets apart that move differently. The failure case is people who meet, stop and walk on; the lesson was rewritten to say that, with 5 swaps in 20 runs.
- **A test world that hid the thing it tested.** The wall of 24-03 was placed at the maximum range of the lidar, so most of its points were beyond it and were dropped; and the two posts in front of the wall hid part of it, splitting it. Both are now placed so that the test is of the clustering and not of the sensor.
- **A geometry that wrote its own answer.** The first scene of 24-02 put the boxes at three places along the ground and the least squares plane came out within 0.02 degrees of the truth; the boxes had to be piled at one end to pull it. The pull is the point, so the scene was made to show it.
- **A step that a plane smooths away.** The plane through the 3 by 3 cells of a step of 5 cm read as 14 degrees and a roughness inside the limit, and the first version of 24-05 did not call it lethal. The steepest rise between neighbours was added; the lesson keeps both numbers.
- **A cell on an edge.** A post of 0.025 m rasterised to no cells because the centres of the four cells round it are 0.035 m away; the sizes in 24-04 were changed to ones that cover a cell.
- **A stub that named what only the solution defines.** Twice, in 23-06 and again in 24-07; the exercises did not compile. The fix both times was in the stub, and the exercise stage of the check caught it, as it is meant to.

## Phase 25, knowing where you are outdoors (13 November 2026)

Nine lessons on the position estimate of a ground vehicle. The module is `rc::locate`, the atlas prefix `VEH` (continued).

| lesson | the finding it rests on | header |
|---|---|---|
| 25-01 wheels and a gyro in a filter | over 120 s with a bias of 0.01 rad/s and a slip, the heading ended 3.1 degrees out from the wheels, 68.0 from the gyro and 1.0 from the filter; with wheels 12 percent out the unweighted estimator ended 3.7 degrees out on average | `rc/locate/gyro_wheels.hpp` |
| 25-02 GNSS and the lever arm | a degree of longitude at 52 degrees is 68 459 m against 111 195 for latitude; the flat map is 0.036 m off at 1 km and 357 m off at 100 km; an antenna 0.5 m from the centre moved at 0.50 m/s on a robot turning on the spot | `rc/locate/gnss_frame.hpp` |
| 25-03 fix quality and outliers | with 1 percent of fixes thrown 8 m, the worst error was 6.04 m ungated and 2.29 m gated, refusing 0.2 percent of good fixes; treating every fix as fixed gave 2.81 m rms on the single fixes against 0.67 m; a gate that never gives up found a carried robot after 68 fixes, one that gives up after 5 refusals after 5 | `rc/locate/gnss_filter.hpp` |
| 25-04 matching one scan to another | a scan moved 0.30 m, 0.10 m and 0.15 rad was matched to 0.0007 m and 0.0001 rad in 16 rounds; it converged from 0.5 rad of rotation error and not from 0.8; a corridor left the error along it at 0.10 m | `rc/locate/icp.hpp` |
| 25-05 finding yourself in a map with particles | 3000 particles found the robot in 5 steps to 0.021 m; 50 particles lost it in 10 runs of 10; a plain rectangular room put the cloud on the mirror image in 2 of 8 | `rc/locate/mcl.hpp` |
| 25-06 how lost can it be | the 95 percent radius was 0.22 m after 10 m and 7.07 m after 100 m, 31.6 times for ten times the distance; 27 m to 1 m and 80 m to 5 m | `rc/locate/dead_reckon.hpp` |
| 25-07 the bias of a gyro when the robot stops | stopping 2 s every 60 s took the heading error after 300 s from 9.9 degrees to 4.6, every 10 s to 0.8; a detector of wheels alone called a carried robot still in 176 of 200 samples | `rc/locate/stillness.hpp` |
| 25-08 when a measurement is from the past | a fix 0.25 s late at 2 m/s left a bias of 0.49 m; extrapolating or using the history gave 0.035 and 0.045 m rms; braking to a stop during the delay put the extrapolated fix 0.25 m beyond the robot | `rc/locate/time_align.hpp` |
| 25-09 the robot that was picked up | a plain cloud was 8.3 m out after a carry of 5.6 m; with the monitor the robot was found again after 21 steps and flagged lost after 1 | `rc/locate/recovery.hpp` |

**Decisions.**

1. **Statistical tests assert wide bounds.** Several lessons (25-03, 25-05, 25-07, 25-09) test the behaviour of random methods. The numbers in the documents are from the build that made them; the assertions hold across toolchains by comparing quantities with a wide margin between them (50 particles against 3000, a stop every 10 s against none).
2. **`MonteCarlo::correct` returns the average likelihood** (a change to 25-05's header made for 25-09), so the recovery lesson can build on it without a second copy of the localiser.
3. **A longer test limit for the localisation lessons** (240 and 300 s in the manifest), because 3000 particles of 24 ray casts run for tens of seconds under the sanitizers.

**Mistakes, and what caught them.**

- **A claim that did not hold.** The first version of 25-01 said weighing the wheels by how straight they are was what made the bias estimate good. On wheels that had been calibrated the naive estimator did nearly as well (1.4 degrees against 1.0). The claim was kept only for wheels 12 percent out, where it holds (3.7 against 1.6), and the lesson says both.
- **A gate that was supposed to lose the robot for ever.** The first test of 25-03 expected a gate that never gives up to lose a carried robot for good. It found it after 68 fixes, because the filter's uncertainty grows while it refuses and the gate widens with it. The lesson says that, and the price of waiting.
- **A match that was held by the sampling of the walls.** In 25-04 a few points on a partition across a corridor could not outvote hundreds on the walls, and the match stayed at the odometry's error; with as many points as the walls it found the truth. The sampling is part of the result.
- **Particles too few for the room.** 1000 particles lost the robot 4 times in 10 in 25-05 and 3000 lost it 0, which is the number the lesson now uses; an earlier version asserted a count that only one toolchain gave.
- **An extrapolation that was as good as the history.** In 25-08 extrapolating a late fix by the velocity was as good as the history through the manoeuvres of the run; the case that separates them is a stop during the delay, which the lesson now shows.
- **A solution variable used by the stub.** Twice again (25-04 and earlier), the stub mentioned a variable defined only in the solution; the exercise stage of the check caught both. The fix is to define what the lines after the stub need before the solution block.

## Phase 26, planning and following, per vehicle (13 November 2026)

Ten lessons on getting a particular vehicle along a route. The module is `rc::drive`, the atlas prefix `VEH` (continued, from 0082 to 0111).

| lesson | the finding it rests on | header |
|---|---|---|
| 26-01 a path a car can drive | the path of arcs had a largest curvature of 1.35 per metre, the car's limit; the corner of a grid path between the same poses needed 15.7; a wall with a gap took 2277 expansions; 12 headings to a cell took 1146 and 48 took 2675; a goal walled in was not found after 4000 | `rc/drive/hybrid_astar.hpp` |
| 26-02 the shortest path for a forward-only car | over 200 pairs of poses the six Dubins words ended on the goal to better than 1e-9 and all six were the shortest at least once; the search ended 0.073 m from the goal and its tail was 4.70 m, 1.25 m from the best of the last nine poses; a goal 0.5 m behind took a loop 10.3 times as long; a radius of 0.4 m gave a curvature of 2.50 against 1.35 | `rc/drive/dubins.hpp` |
| 26-03 smoothing without cutting corners | a corner of 90 degrees went from a curvature of 7.1 to 3.64 in 65 sweeps, and a heavy smoother that moved the path 0.63 m only got to 1.55 against the car's 1.35; with no grid 5 points ended in a block just inside the corner, with the grid none | `rc/drive/smooth.hpp` |
| 26-04 pure pursuit and Stanley | a lookahead of 0.1 m crossed the path 9 times and 1.5 m twice; at a straight turning into a circle of radius 3 m, 0.6 m strayed 0.015 m, 2.0 m strayed 0.149 m and Stanley 0.036 m; at a corner of 90 degrees the closest the car came was 0.38 m raw and 0.41 m smoothed | `rc/drive/follow.hpp` |
| 26-05 a sampling controller for any motion model | with 8 samples the mean error over four seeds was 0.152 m and with 100 it was 0.077 m; a controller with no grid made 15 poses in blocked cells and one with the grid 0; planning with a unicycle for a car tracked as well as the car's own model (0.018 m against 0.047 m) but predicted 2 s of controls 0.67 m from where the car ended | `rc/drive/sampling.hpp` |
| 26-06 a skid steer controller that knows slip | with the real chi of 1.5 and slip of 0.05 the worst error was 0.042 m, against 0.115 m with chi 1 and no slip; on ice at 1.2 m/s the plain controller let go in 51 steps and the one that limited its speed in none, in 5.4 s against 4.6 s; a curve allows 2.12 times the speed on gravel as on ice | `rc/drive/skid_follow.hpp` |
| 26-07 how fast the path can be driven | the capped speeds alone asked for a deceleration of 20.0 m/s2 going into a bend of radius 1 m, the profile 1.50 against a limit of 1.5; slowing for a stop line began 1.30 m before it; the same path took 5.9 s on tarmac and 8.0 s on ice; a corner of 90 degrees took 8.6 s raw and 8.2 s smoothed | `rc/drive/speed_profile.hpp` |
| 26-08 when the vehicle is stuck | a vehicle snagged at 3 m got free after 2 attempts (the wait did not help, the back up did) and finished in 23.5 s, where one without recovery never did; a wall that never moved was given up on after 3 attempts at 31.2 s; a crawl of 0.02 m/s was called stuck after 3.0 s with a threshold of 0.1 m in 3 s | `rc/drive/unstick.hpp` |
| 26-09 planning over rough ground | a metre cost 9.8 J over sand and 1.47 J over tarmac for 10 kg, and 25.2 J up a slope of 14 degrees against 1.47 J across it; the shortest path across a band of sand was 16.0 m and 90 J, the cheapest 31.2 m and 66 J; a risky patch was crossed at cost 200 with no weight and 0 with one | `rc/drive/offroad.hpp` |
| 26-10 how often to replan | never replanning stood at the wall; replanning for reasons arrived in 20.9 s with 5 plans, every 0.2 s in 21.4 s with 86; with a plan time of 2 s a look-ahead of 0.6 m left the vehicle standing for 2.1 s and 3 m for 0.3 s; a plan begun from where the vehicle was started 1.16 m behind it and the vehicle planned 83 times in 90 s without arriving, begun from where it would be it started 0.06 m away and arrived in 20.1 s with 5 plans | `rc/drive/replan.hpp` |

**Decisions.**

1. **Reeds and Shepp paths are not in 26-02.** Dubins is forward only, the lesson says so, and the 10.3 times loop to a goal just behind is the finding that shows what reversing would give. A fuller course could add them.
2. **The Dubins tail is not checked against the grid** (26-02), and the lesson says a tail through a wall is possible.
3. **26-10 simulates a point vehicle.** The lesson is about when to plan, not about how the vehicle follows; the follower is a point that moves towards the next point of the path. The sensor is perfect within 4 m. The wall given to the planner is padded by 0.3 m so a vehicle that cuts a corner does not touch the real one.
4. **A longer test limit for 26-05** (180 s in the manifest), after the first version ran for 27 s on the default build and would be several times that under the sanitizers; the samples, the horizon and the number of seeds were also cut (100 samples, a horizon of 16, 2 to 4 seeds).
5. **26-09's grid path has the corners of a grid.** It plans positions, not poses, for a vehicle that can turn sharply, and the lesson says so.

**Mistakes, and what caught them.**

- **Claims that did not hold, in five lessons.** (a) 26-03: the default smoother only halved the curvature of a corner (7.1 to 3.64, the car's limit being 1.35), and even a heavy one that moved the path 0.63 m left 1.55; the lesson says a corner of 90 degrees is better left to the planner. (b) 26-04: the first test expected pure pursuit to cut a circle, and on a circle it is already on, pure pursuit is exact (the arc to a point of the circle is the circle); it cuts where the curvature changes, so the path became a straight turning into a circle. (c) 26-04: smoothing the path did not reduce the cut at a corner (0.38 m raw, 0.41 m smoothed), because the lookahead was doing the cutting. (d) 26-05: the wrong motion model was expected to track worse, and it did not in three different setups (a half circle, a corner, a block in the path); the closed loop replans every 0.1 s from the real pose, so the model error is corrected before it matters, and what the wrong model spoils is the prediction (0.67 m after 2 s). The lesson was rewritten around that. (e) 26-10: the first run of the latency test did not arrive, and the count of plans showed why: a plan begun where the vehicle was started 1.16 m behind it when it arrived, past the off-path threshold, so the vehicle planned again and again (83 plans in 90 s). That is now the finding of the lesson.
- **Wrong prerequisites in 26-06.** Two lesson ids in `requires` did not exist (I had guessed the names), and the lesson cited the wrong lesson for traction (23-07 for 23-05). The audit did not catch it; a small script that checks every `requires` against the lesson folders did, and it was run over phase 26 afterwards. The audit should check this itself.
- **A folder name over the Windows limit.** 26-02's first title gave a folder of 53 characters (L029 allows 48); the lesson was renamed and regenerated.
- **`typedef`.** 26-01 used `typedef` (L018) twice; `using` now.
- **A stub that used a variable defined only in the solution.** 26-09 again (the third phase running); the exercise stage of the check caught it and the variables are now declared before the solution block.
- **A simulated snag that was released at the start.** In 26-08 the first sim released the snag when the vehicle was at x <= 2.6 m, which is true at t = 0, so the vehicle without recovery finished too; the snag now releases only after the vehicle has reached it. The count of attempts was also reset by the progress rule before the test read it, so the test records the most.

## Phase 27, running a ground vehicle (13 November 2026)

Nine lessons on what it takes to send a ground vehicle out for a day, the last phase of stage 1. The module is `rc::drive` (now 19 headers), the atlas prefix `VEH` (continued, from 0112 to 0138). Phase 27 does not repeat phases 19 to 21: each lesson says what it adds to the lesson it extends (the stop path of 21-06, the monitor of 19-08, the black box of 19-12, the release of 20-12).

| lesson | the finding it rests on | header |
|---|---|---|
| 27-01 a list of places to go | four waypoints were reached in order with 4.1 s of dwell; a leg that could not be done timed out at 10.0 s; restoring saved progress at the third leg drove 19.4 m on the rest of the list and starting again 38.4 m; a car with a smallest radius of 0.742 m was still circling a waypoint 0.8 m to its left after 40 s with a tolerance of 0.10 m, and reached it in 1.8 s with 0.70 m | `rc/drive/waypoint_mission.hpp` |
| 27-02 a person at the controls | for a sight line of 6 m and a braking of 2 m/s2 the speed allowed was 4.00, 3.29 and 2.32 m/s with 0.5, 1.0 and 2.0 s of delay; after the commands stopped a vehicle at 2 m/s went 1.99 m with a timeout of 0.5 s and 4.99 m with 2.0 s; with an obstacle sensed 20 m ahead the vehicle stopped 0.50 m short, and without the cap it went 58.7 m past | `rc/drive/teleop_governor.hpp` |
| 27-03 a field that moves with the vehicle | a vehicle at 1.5 m/s stopped 0.20 m short of a thing (the margin); a field worked out with no latency, when it was 0.3 s, let it 0.25 m through; a thing on the arc of a turn of radius 2 m was missed by the straight field and found by the swept one; at 1.1 m/s the field of the set for 1.5 m/s was 1.46 times too long | `rc/drive/protective_field.hpp` |
| 27-04 how far it can go and how hot | a metre of tarmac cost 10.1 J for 10 kg with an 8 W computer (79 percent of it the computer) and a metre of sand 22.0 J; a way home taken as flat turned back 16.8 m out and was 25 J short, planned over the ground it turned back at 12.0 m with 71 J to spare; a slope of 14 degrees held a motor at a rise of 71 K and 20 degrees reached the 75 K limit in 62 s (31 m at 0.5 m/s) | `rc/drive/range.hpp` |
| 27-05 a log that tells you what went wrong | five faults put into a run of 300 s were all found within 1.8 s; 20 clean noisy runs gave 0 false findings; a slip to 80 percent of the wheels was not reported and one to 50 percent was | `rc/drive/log_doctor.hpp` |
| 27-06 the checklist before the run | ten checks, six that block and four that warn; a state of numbers that were not numbers failed 8 checks; the energy check passed at exactly enough and failed 0.01 Wh under; a block could not be overridden and a warning could only with a reason | `rc/drive/field_checklist.hpp` |
| 27-07 when something stops working | a lost scanner asked for a safe stop, a gyro for a creep, a GNSS for reduced, and GNSS with link for a creep; a requirement flapping every 2 s changed the mode 60 times raw and once through the manager; the way back to full took 30 s; with no GNSS the sigma reached 1.5 m after 468 m | `rc/drive/degraded.hpp` |
| 27-08 a release for this vehicle | a vehicle that was moving, in a mission and at 20 percent charge was refused for 3 reasons at once; a track of 0.40 m taken for 0.25 m turns 1.60 times too little; a wheel wired backwards and a lost calibration each failed one check of the self test; a good release confirmed in 1 boot and a bad one was rolled back after 4 | `rc/drive/vehicle_release.hpp` |
| 27-09 a ground vehicle for a day | with the layers: 4 of 4 legs in 470 s, 0 contacts, stopped by the field 11.4 s, 2 unsticking attempts, 130 s reduced or creep, 12.9 s safe stop, 1.52 Wh of 2.00 used, the release refused mid job and taken at the dock; without them: 2 of 4 legs, 1 contact, 975 s, out of charge 150 m from the dock | `rc/drive/ugv_day.hpp` |

**Decisions.**

1. **Build on phases 19 to 21, do not repeat them.** 27-02 is the person and the delay (21-06 was the link), 27-04 the route and the slope (19-08 was the monitor), 27-05 reads a log (19-12 and 22-09 wrote one), 27-08 is the vehicle's side of a release (20-12 was the pipeline).
2. **27-03 models the idea of a protective field and does not claim a standard.** It names ISO 3691-4 as where such fields are asked for, and says a real safety function is a certified device and not a header.
3. **27-09 is a simulation of a day on a road one dimension long**, with a point vehicle and scripted events, and says in its limits that nothing here has been on a real vehicle.
4. **The motor constants of 27-04 are marked typical** (a 12 V 37 mm class gear motor), per the real-world-dimensions rule.
5. **The enumerators of the log doctor are `kFind...`** after a clash with the replanner's `kOffPath`.

**Mistakes, and what caught them.**

- **A name clash between two headers.** 27-05's finding kinds `kOffPath` and `kGap` clashed with 26-10's `kOffPath` when every header is included together. Each lesson compiled alone; the headers-together build caught it, and the kinds became `kFindStuck` and so on.
- **A drain that was never found.** The first log doctor used a window start that the loop had already run to the end, so no row of a drain was ever marked. The test with five faults put in found 4 of 5.
- **A tolerance test that was trivially true.** The first car in 27-01 started 0.5 m from the waypoint with a tolerance of 0.6 m and had arrived before it moved; the geometry was redone from the turning circle (0.8 m to its left, whose nearest pass is 0.68 m).
- **A field too short for the arc.** 27-03's first test of the swept field used 1.0 m/s, where the stop field is 0.75 m and the thing was 1.40 m along the arc, so the answer was a warning and not a stop; 1.8 m/s.
- **A vehicle that crept up on the person.** In 27-09 the first version evaluated the field at the speed the vehicle had, and a stopped vehicle has a field of 0.2 m, so it accelerated towards the person, stopped, and came on again until it touched. The field is now evaluated at the speed it is about to use. The person's radius of 0.3 m also exceeded the margin of 0.2 m and was cut to 0.1 m.
- **A vehicle without layers that was not stuck for ever.** The expectation that it would sit at the snag to the end was wrong: the timeouts of its legs let the mission give up and drive home, and the vehicle ran out of charge instead (150 m from the dock). The lesson says what happened.
- **The attempt count read after progress had cleared it.** As in 26-08, the unsticking ladder forgot its attempts once the vehicle had moved a metre, so the day recorded the most.
- **A slip that was a recovery.** The log doctor called the back and forth of the unsticking ladder slip, because it judges slip by the net movement of a window. That is kept in the lesson as a limit of the doctor.
- **Lesson numbers from memory.** Three times a lesson number or title was written from memory and was wrong (22-09 and 22-16 in `requires`, the trial as 20-10 for 20-06); the check of `requires` against the lesson folders found the first two before the commit, and the third was found on re-reading. The audit still does not check `requires`.
- **A `using namespace rc::drive` with nothing in rc::drive to use.** Three lessons (27-02, 27-03, 27-05) that include no rc::drive header got a `using` line and did not compile; the line is only for lessons that include one.
- **A bound too tight for a real simulator (found on CI, not locally).** On the push of phase 27 the non-gating Gazebo job failed in 22-14: Gazebo's end time was 9.613 s against 9.6 s for the built in model, and the test asked for 0.01 s. The step counts were equal, so the clocks agree to a step; Gazebo stamps its poses from its own clock and a shared runner moved it. The bound is now 0.05 s, half a command step. All eight gating jobs had passed, and I first reported the run as simply green, which was wrong: the job summary showed the Gazebo job red, hidden by its being non-gating.

## Phase 28, arm geometry in three dimensions (13 November 2026)

Nine lessons that start the arm course. The module is `rc::arm` (9 headers), the atlas prefix `ARM` (from 0001 to 0027). It extends phase 13, which did planar arms, to six and seven joint arms in space. The two real arms are written as tables with the maker's published numbers (the UR5 in the standard convention and the Franka Emika Panda in the modified one), marked as such and to be checked against the data sheet of an arm in hand; the one arm that has a closed form is a typical one with typical dimensions.

| lesson | the finding it rests on | header |
|---|---|---|
| 28-01 frames and a table for an arm | the same row of a table read in the two conventions gave frames 0.170 m apart; an inverse that only transposed the rotation was 0.35 m out for a pose 0.5 m from the origin; a rotation composed a million times drifted 1.3e-10 from orthonormal and was put back to 2e-16 | `rc/arm/frames.hpp` |
| 28-02 the tool in three dimensions | with every joint at zero the UR5's tool was at (-0.81725, -0.19145, -0.00549) m, the sums read off its table, and the Panda's flange at (0.088, 0, 0.926); the Panda's zero pose is outside its joint limits (joint 4); roll pitch yaw jumped 57 degrees in roll for 0.0001 rad of pitch near 90 degrees | `rc/arm/fk.hpp` |
| 28-03 how the tool moves when the joints do | the analytic Jacobian matched central differences to 1.4e-10 for both arms over 40 poses; read with the other convention's rule the Panda's was 1.29 out; torque times rate equalled wrench times velocity to 1e-15; a vertical force asked 0 of the vertical first joint | `rc/arm/jacobian.hpp` |
| 28-04 eight ways to reach a pose | for a typical arm with a spherical wrist the closed form gave 8.0 solutions on average, the original angles were among them in 40 of 40, and every solution reached the pose to 1e-10; the edge gave 4, a pose 1.5 m away 0, a straight wrist 6; the UR5 has no spherical wrist (d5 = 0.09465 m) | `rc/arm/ik_closed.hpp` |
| 28-05 iterating to a pose | damped least squares converged in 37 of 40 from near and 31 of 40 from all zeros on the UR5, 36 of 40 on the Panda (with angles that were not the ones that made the pose); near a singularity a plain step was 20.6 rad against 0.021 damped; an unreachable pose stopped at the step limit 0.64 m away; it found one of the 8 closed form solutions in 29 of 30 | `rc/arm/ik_dls.hpp` |
| 28-06 where the arm loses a direction | the smallest singular value was 0.172 in a generic pose and 0 at the wrist, elbow and shoulder singularities, falling smoothly from 0.134 to 0.031 to 0.003 as joint 5 went 0.5, 0.1, 0.01; a line 0.01 m from the shoulder axis turned joint 1 at 26.1 rad/s undamped and 2.0 damped, leaving the tool 0.045 m off the line against 0.0006 | `rc/arm/singular.hpp` |
| 28-07 one joint too many | a motion put through the null space was felt by the tool at 1e-10 of its size (none for six joints); added without the projection it was felt as 1.24 m/s; the self motion took the margin of joint 3 from 0.086 to 0.156 in 6 s with the tool within 0.4 mm; clamping kept every joint inside its limits where the unclamped answer left joint 6 0.283 rad outside | `rc/arm/redundant.hpp` |
| 28-08 where the arm can work | in cells of 0.10 m the volume was 1.08 m3 from 2 thousand poses, 2.58 from 20 thousand and 2.96 from 200 thousand, and in cells of 0.05 m 2.03 from 60 thousand against 2.55 from 200 thousand, still rising; at table height the typical arm reached 0.86 m in any orientation and 0.74 m with the tool down; 62 percent of the reachable volume was dexterous; 96 percent of a table was reachable tool down with and without limits of 120 degrees and 78 percent with 60 | `rc/arm/reach3d.hpp` |
| 28-09 the arm it really is | a simulated UR5 off by 0.3 degrees and 1 mm put its tool 3.39 mm from its table; fitted to 60 poses it was 0.081 mm on them and 0.100 mm on 30 others; fitted to 4 poses it was exact on them and 0.83 mm on others (8: 0.151, 24: 0.107, 60: 0.101); moving the offset of joint 2 or of joint 3 gave the same tool place to 1e-12 | `rc/arm/calibrate_kin.hpp` |

**Decisions.**

1. **The two real arms are tables, not models of a product.** The UR5 and the Panda are written down with the numbers their makers publish and the lessons say they are to be checked against the data sheet; nothing in the course touched either arm. The arm with a closed form (28-04 onwards) is a typical one with typical dimensions, as the real-world-dimensions rule asks.
2. **Phase 28 extends phase 13 and does not repeat it.** 13 did two and three link planar arms; 28 does arms in space, with the conventions of a published table, the Jacobian in three dimensions, singular values, redundancy, reach by sampling, and calibration.
3. **Lessons cite only lessons that exist.** The audit's rule L020 refuses a citation of a lesson that has not been written, so forward references in the first lessons say "the next lesson" or "the last lesson of this phase".

**Mistakes, and what caught them.**

- **Expectations that the measurement did not support, five times.** (a) 28-05: the straight elbow of the UR5 was expected to give an enormous plain step and gave a small one: at the singularity itself the plain step is zero in the lost direction, and the enormous steps are just beside it, so the test moved the elbow 0.002 rad off. (b) 28-07: started with joint 6 near its limit, the self motion did not improve the margin, because the one free motion of the arm (the elbow swing) hardly turns joint 6; the lesson now starts with joint 3 and says that a null space only helps the joints that take part in it. (c) 28-08: a hole round the base was expected and there is none for this arm (the flange can reach into the space round the shoulder), and wide joint limits (120 degrees) cost nothing at all; what the measurement found instead is the shorter reach with the tool down and the loss with tight limits. (d) 28-08: 200 thousand samples were expected to be enough and in cells of 0.05 m they were not. (e) 28-09: with 8 poses the overfitting was expected to be severe and was mild for an exact model class; it shows plainly at 4 poses.
- **A Gauss-Newton that was called Levenberg and Marquardt's.** The first 28-09 added every step; with 60 poses and 1 mm of noise it passed at 12 iterations and diverged to metres at 25. The accept or raise the damping rule was added, and is now what the lesson says is the difference.
- **Square roots halve the digits.** In 28-06 the singular values at singularities came out near 1e-8 where the eigenvalues were 1e-16, and the first threshold of 1e-9 failed on one of three; the threshold is 1e-6 and the lesson says why.
- **A wrong prerequisite id, again.** 28-03 named a phase 13 lesson without its phase number; the audit's L012 caught it. (It had not caught the wrong ids of phases 26 and 27, which had the right form and named lessons that did not exist; the lesson ids are now read off the folders every time.)
- **A lesson used a lambda.** 28-06 sorted with a lambda, which no lesson in its prerequisite chain teaches (L021); `std::greater` replaced it.
- **A stub that used a variable defined only in the solution**, once more (28-05); declared before the solution block.
- **A title that repeated a phase 13 lesson.** 28-02 was first called "where the tool is", which is 13-01's title.

## Phase 29, arm dynamics and joint control (13 November 2026)

Eight lessons on making an arm do what it is told once it has mass. The module is `rc::arm` (now 17 headers), the atlas prefix `ARM` (continued, to 0051). The dynamic model is for arms in the standard convention, with each link a uniform rod of a typical mass and radius (the real arms' inertias are not published in what the course used); the motor, gear and servo numbers are typical and marked so.

| lesson | the finding it rests on | header |
|---|---|---|
| 29-01 what the arm weighs and how it swings | recursive Newton and Euler gave a torque of m g L over 2 (3.924 N m) for a 2 kg rod held out 0.4 m and 0 when it stood up; the torque of gravity matched the slope of the potential energy to 1e-8 N m; the mass matrix was symmetric and positive; a free fall of a second lost 3.7 percent of its energy with Euler's step and none (1e-12) with a fourth order step | `rc/arm/dynamics3d.hpp` |
| 29-02 holding the arm up | a PD controller sagged 0.111 rad at the shoulder where the torque over the gain said 0.109, and 0 with gravity added; an unknown payload of 2 kg left 0.065 rad; its mass was estimated from six noisy poses as 2.00 kg and the error fell to 0.0001 rad | `rc/arm/gravity_comp.hpp` |
| 29-03 a gain that knows the pose | the shoulder's inertia went from 0.691 to 2.091 kg m2 (threefold) and through a 100:1 gear 1.8; gains for the heaviest pose were unstable at the lightest and gains that followed the inertia gave 0 percent overshoot at both; an integral that kept running at the torque limit overshot 186 percent against 19 held | `rc/arm/joint_cascade.hpp` |
| 29-04 friction and play in a joint | the friction peaked at 0.0559 N m at 0.017 rad/s and dipped to 0.0503 at 0.05; a feedforward of the friction cut the error following a slow ramp; the 0.0100 rad gap was measured as 0.0102; with an integral on the position and the load's encoder in the loop the load hunted 1.3 mrad, and with the motor's it was steady and 3.4 mrad off | `rc/arm/joint_friction.hpp` |
| 29-05 using the model to move | on a move of 0.7 rad in a second computed torque with an exact model had an rms error of 5.7e-5 rad against 0.043 for PD with gravity; it crossed zero 0 times from 0.1 rad out and settled in 0.33 s; with masses 10, 20 and 40 percent too big the error was 0.0047, 0.0085 and 0.0146 rad, and 40 percent too light 0.0347 | `rc/arm/computed_torque.hpp` |
| 29-06 giving way on purpose | with the target 2 cm below a surface the tool settled at 3.97, 9.76 and 19.04 N for stiffnesses of 200, 500 and 1000 N/m; coming down on it peaked at 27.6 N soft and 68.6 N stiff; an admittance with a damper of 100 N s/m was unsteady on the stiff surface and steady with 1500, settling at 9.3 N | `rc/arm/compliance.hpp` |
| 29-07 an arm made of smart servos | a step of 4096 to a turn is 0.00153 rad, 1.07 mm at 0.7 m; the bus took 2.98 ms for six servos at 1 Mbit/s (336 Hz) and 14.3 ms at 115 200 bit/s (70 Hz); a load of 2 N m drooped the joint 0.133 rad and a shifted goal left it 0.0002 off; a ramp was followed 97 ms late at 20 Hz and 77 ms at 100 Hz | `rc/arm/servo_bus.hpp` |
| 29-08 how much the motors can give | holding the arm out took 37.7 N m and reached the winding's limit in 293 s, holding it folded took 2.1 N m and never did; the payload was 2.5 kg out and the rated 10 kg folded; a move of 0.7 rad was 0.29 s by the peak limits and 0.39 s by the continuous ones; repeated with no wait it raised the winding 118 K and with 4 s of wait 27 | `rc/arm/limits_heat.hpp` |

**Decisions.**

1. **The standard convention only, six joints for forward dynamics.** The recursion is in the base frame for arms whose joint axis is the z axis of the frame before the row; the Panda (modified) is not used here, and `forward_dynamics` solves a 6 by 6.
2. **Inertias are rods and typical.** `rod_links` builds them from a table, with a mass for each link and a radius; the lessons say that a real arm's are measured.
3. **29-04 puts the velocity loop on the motor always.** A velocity loop on the load through a gap blew up in the first version; a drive's velocity loop is on the motor, and the choice of encoder is for the position loop.
4. **A longer test limit for 29-06** (240 s in the manifest): the arm in contact is stepped at 0.5 ms and the five runs take about 20 s on the default build and over a minute under the sanitizers.

**Mistakes, and what caught them.**

- **Gains that were not for the inertia, twice.** In 29-02 and 29-04 the first gains (a damping assumed for 0.5 kg m2 on every joint, a load that was 0.8 kg m2 beside a motor of 0.005) made the simulation blow up; a result that is not a number was reported as an error of zero because `fmax` ignores it, so the first runs looked like a perfect controller. Every simulation helper now returns a huge error for a run that is not finite.
- **Claims that did not hold, five times.** (a) 29-03: the inertia varies threefold, not sixfold as the title first said, and with a 50:1 gear and a small rotor the ratio did not change at all; a rotor of 1e-4 kg m2 and 100:1 does. (b) 29-04: the loop on the load's encoder did not hunt until there was an integral on the position error; the integral is the part that does it. (c) 29-05: the largest torque of computed torque was the same as that of the PD controller, because on that move it is the weight that sets it, not the acceleration; and a model 40 percent too light was still better than none. (d) 29-07: the shifted goal was shifted the wrong way in the first version (plus the load over the stiffness, where it should be minus), which doubled the droop and was caught by the test of the held pose. (e) 29-08: the first motors had a continuous limit under the weight of the arm out, a payload of 82 kg was found for the folded arm (a rating of 10 kg for the structure was added), and holding the end pose of the repeated move was itself too hot.
- **A test that was run for too short a time.** Shortening the runs of 29-06 to save time made the soft case not settle, and `done.sh` committed the failing lesson anyway (it does not stop on a failed test); it was fixed in the next commit before pushing. The scripts' habit is now to read the result line before the commit.
- **A `using namespace` for a namespace that is not included.** 29-04 again (the lesson includes no rc::arm header); and a missing include of the closed form's header in the tests of 29-01 (for `elbow_arm`).
- **A stub that used a variable defined only in the solution**, in 29-01's recursion; the variables the stub needs are declared before the solution block.
- **Audit rules met.** L020 refused citations of lessons not yet written; L021 refused a lambda in 28-06 (replaced by `std::greater`), and the wrong lesson numbers of a few `requires` were found by looking at the folders.

## Phase 30, moving without hitting (13 November 2026)

Eight lessons on planning an arm's motion so that it touches nothing and still gets there. The module is `rc::arm` (now 25 headers), the atlas prefix `ARM` (continued, to 0075). The scene is capsules for the links, spheres and boxes for the world; all clearances come from the capsule distances of 30-01, and every number below comes from the lesson's own test suite.

| lesson | the finding it rests on | header |
|---|---|---|
| 30-01 the shape of the arm | a sphere 0.30 m, a capsule 0.70 and a box 0.50 from the arm gave those distances; the floor left 0.140 m and a pose below it was 0.160 m through; six link pairs, three of them not neighbours, are checked for self collision, and a folded arm was 0.120 m into itself | `rc/arm/collision.hpp` |
| 30-02 how far it is safe to step | a bound on how far the links sweep for a joint step (1.33 m for the whole arm) held on 200 random steps (worst ratio 0.289); a fixed step of the checker missed a thin wall on a 2.0 rad edge, the conservative check found it in 14 checks, and a far-away edge cost 2 checks against 201 | `rc/arm/motion_check.hpp` |
| 30-03 growing a tree through the clutter | RRT-Connect solved 6 of 6 seeds with 23 nodes, plain RRT 3 of 6 with 58; the path was 2.90 times the straight line; a start or goal in collision gave no plan at once (0 iterations) | `rc/arm/rrt.hpp` |
| 30-04 a map of where the arm can go | a roadmap of 250 nodes and 8 neighbours cost 56 406 checks to build and 858 to answer a query, against 2 432 for a fresh RRT; the path was 0.74 of the RRT's; 5 components; a narrow gap was crossed with 7 of the samples at 40, 120 and 300 nodes alike, and 34 percent of a poorly connected map's edges were bad | `rc/arm/roadmap.hpp` |
| 30-05 making the path shorter and smoother | shortcutting cut a path from 4.83 to 1.85 rad over 150 tries and cut the sharpest turn from 2.25 to 0.60; the clearance rose from 0.0001 to 0.0202 m | `rc/arm/shortcut.hpp` |
| 30-06 putting the path in time | an S-curve move took 1.220 s against 1.000 s for a trapezoid (whose jerk was 4000 rad/s3); the timed path used 0.727 of the velocity limit and all of the acceleration and jerk; a wandering path took 14.33 s against 1.45 s direct; half the limits doubled the time (3.70 to 6.45) | `rc/arm/time_param.hpp` |
| 30-07 a world that changes | a scene version moved from 0 to 4 over four changes; a thing seen 2.0 s ago at 0.5 m/s was taken to have grown 0.50 m; the clearance fell from 0.31 to 0.11 m for that growth; an arm that kept a part in its hand cleared the world by 0.090 m and, with the part left out, was 0.090 m through it | `rc/arm/scene_monitor.hpp` |
| 30-08 getting out of the way of something that moves | a track gave 0.499 m/s for a true 0.5; a meeting 1 m away at 0.5 m/s was found 1.78 s ahead; an arm that did not look went 0.112 m through a sphere rising across its path, one that stopped and waited kept 0.031 m and took 2.15 s (1.50 free), and one that planned round kept 0.135 m and took 4.20 s with 1 plan | `rc/arm/moving_obstacle.hpp` |

**Decisions.**

1. **Capsules, spheres and boxes only.** Distance to a box is found by a ternary search along the capsule's axis; it is exact for a convex shape and cheap.
2. **A conservative edge check by bounds, not by fine steps.** 30-02's bound on the sweep of a joint step lets the planner skip most checks on long clear edges and refuse any edge it cannot prove.
3. **A count of the poses looked at is carried by every plan** (`PlanProblem::checks`, mutable), because the cost of a plan is the number of checks and not the time.
4. **The floor check skips the first link** (the base stands on it), and self pairs are those more than one link apart.
5. **30-08 plans round at once or waits, by a flag.** The first version also tried to retreat to a pose away from a big obstacle; the scene put the arm's start pose inside the obstacle, so the retreat could not be tested honestly and was left out (and said so in the lesson).

**Mistakes, and what caught them.**

- **An exercise that never ended.** In 30-03 a stub `steer` that returned its start made the connect loop spin (exit 124). `extend` now treats a step that makes no progress as trapped, so a wrong stub fails at once.
- **An exercise that passed with the stub.** In 30-05 duplicated points hid the sharp turns, so the stub passed. The test now also requires the length to fall and no repeated points.
- **A half-finished lesson committed with another change** (30-04 with a change to 30-03). Undone with a soft reset, the generated files removed and the lesson generated again properly; the poses counter went in as its own commit.
- **30-08 was tuned against its own scenario for a long time.** The obstacle growth first made the arm wait 13 times for a slow reach (2.15 s), then, when halved until the arm's own pose was free, planned round at once. The lesson says what was measured: waiting was faster than going round here (2.15 s against 4.20 s), both were safe, and which is right depends on whether the thing leaves. A big obstacle that was already on the arm could not be got away from by standing still, and the lesson says that too.
- **A folder name over the limit** (L029, 49 characters for 30-08): renamed to `08-getting-out-of-the-way-of-a-moving-thing`. `done.sh` was also run with a short slug (no target found), which showed up as `ninja: unknown target` and a count of 0; the next run used the full slug.
- **The scratchpad was found by a glob that matched more than one session**, and a `sed` over the wrong paths printed errors; checked with `git status` that nothing outside the lesson had changed.

## Phase 31, manipulation (13 November 2026)

Nine lessons on picking a thing up and putting it down. The module is `rc::arm` (now 34 headers), the atlas prefix `ARM` (continued, to 0102). The hand, the contact, the friction and the safety numbers are typical and marked so; the separation lesson is a model of the idea in ISO 13855 and ISO/TS 15066 and says in its first lines that it is not a safety function.

| lesson | the finding it rests on | header |
|---|---|---|
| 31-01 a hand that pinches | a kilogram at rest needed 9.81 N of squeeze at a friction of 0.5 and 14.81 N when lifted at 5 m/s2; a hand of 235 N held at most 16.0 kg at a safety factor of 1.5 and 8.0 kg with pads at half the friction | `rc/arm/gripper.hpp` |
| 31-02 picking and placing in order | a pick and place that went to plan took 36 ticks, one miss and a retry 48; a hand that always missed stopped in fault 1 after 3 tries; a dropped part was seen at once (fault 2) and a move that never arrived stopped at tick 62 (fault 3) | `rc/arm/pick_place.hpp` |
| 31-03 finding the part with a camera | a pixel 60 right of the centre at 0.5 m was 0.050 m to the side; with 2 mm of noise 5 points gave 3.5 mm and 0.64 degrees and 40 points 1.1 mm and 0.20 degrees; a flat set of points gave no fit; one degree of tilt was 10.5 mm at 0.6 m | `rc/arm/hand_eye.hpp` |
| 31-04 feeling that it has touched | driven at 0.2 rad/s into a wall, contact was found 15 ms after it was reached at 1.31 N m and at 0.6 rad/s the wall reached 5.20 N m; a threshold of 0.05 N m with gravity 5 percent out gave a false alarm in free motion and 0.8 N m none | `rc/arm/contact.hpp` |
| 31-05 putting a peg in a hole | a peg 2 mm off a hole with 100 micrometres of clearance needed 0.8 N of push from a hand of 300 N/m, 5.6 N from 2000 N/m and 279 N (against 50) from 100 000 N/m, which jammed; at a friction of 1.2 the peg could not slide; over offsets of 0 to 4 mm the soft hand got in 13 of 17 and the stiff one 2 | `rc/arm/insertion.hpp` |
| 31-06 staying far enough from a person | a person at 1.6 m/s, 100 ms of reaction, 5 m/s2 of braking and 17 cm of margins made a robot at 1.0 m/s need 0.85 m and at rest 0.33 m; 1.0 m allowed 1.23 m/s and 2.0 m allowed 2.49 m/s; a robot braking at the full distance came to rest 0.17 m away, one that left out the person's motion at minus 0.31 m and one that left out the reaction time at minus 0.09 m | `rc/arm/ssm.hpp` |
| 31-07 how many parts an hour | a cycle took 3.68 s at the rated speed (977 an hour), 5.70 s at half and 10.13 s at a quarter, with 0.8 s of hand and 1.07 s of slow approach that do not scale; a target of 5.16 s needed an override of 0.576 and 2.95 s was refused; a person at 1.0 m made the cycle 3.77 s and at 2.0 m 3.68 s | `rc/arm/cycle_time.hpp` |
| 31-08 an arm that follows a hand | a follower with a 50 ms lag had an rms error of 0.059 rad with the leader read at 100 Hz, 0.081 at 20 Hz and 0.168 with 100 ms of delay; a jump of 1 rad was followed at 2 rad/s in 0.48 s; without a watchdog a follower went 0.398 rad on after the link was lost | `rc/arm/teleop.hpp` |
| 31-09 keeping what the hand showed | an episode of 5.0 s had 151 samples and a longest gap of 0.039 s, trimmed to 2.8 s from the 3 s reach with 1 s of idling at each end; the follower lag was found as 100 ms (true 100); an episode that lost 20 frames in a row had a gap of 0.70 s and was refused | `rc/arm/demo_record.hpp` |

**Decisions.**

1. **The stub of each lesson is the one function that carries the lesson's idea** (the force that holds, the grasp check, the back projection, the debounce, the push to centre, the inverse speed, the bisection, the follower step, the trim), and each stub returns something that fails the tests at once and never loops.
2. **31-03 fits the camera by the polar factor of the cross covariance** (Newton's iteration) and not by a singular value decomposition, so that no matrix library is needed; it says that it fails for points in a plane. Eye in hand (A X = X B) is named and not built.
3. **31-06 is a model, with typical numbers, and says so in its opening lines**; the speed of a person and the reaction time are for the learner to replace by the current edition and a measurement. Power and force limiting is named and not modelled.
4. **31-07 finds the override by bisection** because the contact speed does not scale with it.
5. **No learning in 31-09**: it stores, resamples, trims and checks demonstrations, which is what the plan said (the learning course is later).

**Mistakes, and what caught them.**

- **`requires` ids written from memory, three times** (28-01 and 19-01 and 18-01 do not exist; the folders are 28-02, 19-01-what-can-go-wrong and 18-07). Rule L012 caught each; the ids are now read off the folder listing before the spec is written, and a commit with an audit issue is undone with a soft reset and made again after the fix.
- **A `using namespace rc::arm` in a lesson that includes no arm header** (31-01), and the reverse, a lesson that includes `frames.hpp` and has no `using` (31-03); both compile errors named the line.
- **A test that proved nothing** in 31-08: the first lost link test let the follower converge before the timeout, so the follower with a watchdog and the one without ended in the same place. The test now has the leader jump just before the link is lost, and the lesson says that holding at the timeout is true by construction.
- **Wording that the numbers did not bear out**, three times: in 31-05 the stiff hand got in a little beyond the clearance, not only inside it; in 31-06 the robot that left out the reaction time also met the person (minus 0.09 m); in 31-09 trimming left 2.8 s of a 3 s reach because a smooth reach starts and ends slower than the threshold. Each was changed in the lesson, not in the test.
- **Name of the reference for `done.sh`**: it takes the whole underscored slug of the lesson, which was a mistake again once in phase 30 and not in this phase.
