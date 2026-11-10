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
| `58af7a8` | 04-14, the header that changed and took everything with it, graduated `rc/core/smoother.hpp` |
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
