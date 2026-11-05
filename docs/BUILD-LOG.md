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
| lessons | 108 of 320 planned |
| phases opened | 16 of 22, none complete to plan |
| hours of material | 221 |
| Failure Atlas entries | 272 |
| librc headers | 97 across 13 modules |
| reference tests | 143 |
| exercises | 108, of which 4 are declared not to compile |
| commits | first on 28 August 2026, count with `git rev-list --count HEAD` |
| continuous integration | eight jobs, all green |

```
gcc 11   143/143, 0 warnings      clang 14  143/143, 0 warnings
asan     143/143, 0 warnings      gate      108 of 108 reported
audit    108 lessons, 272 atlas entries, 0 issues
```

## Phase by phase

`built` is on disk. `planned` is [ARCHITECTURE.md](ARCHITECTURE.md), which was
written before any lesson existed and has not been revised.

| phase | built | planned | |
|---|---|---|---|
| 00 Toolchain and First Motion | 9 | 14 | 64% |
| 01 C++ Core I | 13 | 18 | 72% |
| 02 Memory and Ownership | 10 | 16 | 62% |
| 03 C++ Core III | 11 | 18 | 61% |
| 04 Build Systems and Tooling | 8 | 14 | 57% |
| 05 Testing and Fakes | 7 | 12 | 58% |
| 06 Robot Maths | 6 | 16 | 37% |
| 07 Concurrency and Real Time | 5 | 16 | 31% |
| 08 Systems Input and Output | 5 | 14 | 35% |
| 09 Qt Core | 4 | 14 | 28% |
| 10 Qt Widgets | 4 | 14 | 28% |
| 11 Qt Quick and QML | 2 | 12 | 16% |
| 12 Qt for Robot Systems | 0 | 12 | 0% |
| 13 Kinematics and Dynamics | 5 | 16 | 31% |
| 14 Control | 6 | 14 | 42% |
| 15 Sensors, Drivers and Vision | 6 | 16 | 37% |
| 16 Estimation and Navigation | 7 | 16 | 43% |
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
and the stack, named in no lesson between 02-01's pointers and 02-02's heap
(02-10).

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
work whose reasoning is not yet anywhere else. Earlier commits carry their
reasoning in their own messages, and `git log` is the record.

| commit | what it did |
|---|---|
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

**No phase is complete to plan**, and the plan's counts have not been revised
since before the first lesson existed. A better definition of done is that
every phase delivers the ending it promises, which the front of the book
already does in places.

**A learner's solutions live in files this repository also edits.** They write
into `exercise/solution.hpp`; a `git pull` after that lesson is revised will
conflict. Nothing in the repository addresses this yet. The cheap answer is
that a learner works on their own branch and rebases; the thorough one is a
convention where their answers live outside the shipped tree.

**Phase 00 still lacks** a lesson on keeping a result, which means writing a
file, where 01-12 only reads one. And nothing anywhere covers reading somebody
else's code.
