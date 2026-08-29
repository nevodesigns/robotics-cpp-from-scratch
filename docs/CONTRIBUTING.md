# Contributing

## Before anything else

```bash
cmake --preset default
cmake --build build/default
ctest --test-dir build/default -L reference --output-on-failure
./build/default/bin/rcpp audit
```

All four must pass. The last one is the gate, and it enforces every rule in
[CONTRACT.md](CONTRACT.md).

The counts in the README are generated. After adding a lesson or an atlas entry:

```bash
./build/default/bin/rcpp readme --fix
```

Continuous integration runs `rcpp readme` without `--fix` and fails if a person
typed a number that disagrees with what is on disk.

## Adding a lesson

```bash
mkdir -p phases/NN-phase-slug/NN-lesson-slug/{docs,exercise,reference,tests}
echo 'rc_add_lesson()' > phases/NN-phase-slug/NN-lesson-slug/CMakeLists.txt
```

Then write, in this order, because each one constrains the next:

1. **`tests/`** first. Decide what finished means before deciding how to explain
   it. The test suite is the specification.
2. **`reference/solution.hpp`**, until the tests pass.
3. **`exercise/solution.hpp`**, the same interface with the work removed and a
   comment saying what to do. It must fail, and it must fail with a readable
   message rather than a crash. Use `RC_REQUIRE` before anything that indexes.
4. **`lesson.json`**, including the three `breaks_first` atlas entries.
5. **`docs/en.md`**, with all six required sections.
6. **`quiz.json`**, six questions: one pre, three check, two post.

Then:

```bash
cmake -S . -B build/default
./build/default/bin/rcpp audit
./build/default/bin/rcpp verify NN-NN --reference
```

## Adding an atlas entry

Every lesson names three failures a newcomer will hit, and each must exist in
`atlas/errors/`. Do not invent them. Cause the error, copy the real text, and
write the entry from what actually happened on a supported toolchain.

```
id: E-QT-0005
title: One line naming the failure
match: a regular expression matched against the error text
platforms: linux, windows
teaches: 09-02-what-moc-generates
---

## Symptom

What the learner sees, in their words.

## Cause

What is actually happening underneath.

## Fix

What to do, per platform where they differ.
```

Test it against the real error:

```bash
cmake --build build/default 2>&1 | ./build/default/bin/rcpp explain
```

## Where things live

```
CMakeLists.txt          the super project: every lesson is a build target
CMakePresets.json       one preset per supported toolchain, plus the sanitizers
platforms.json          the toolchains, and what each lane provides
cmake/RcLesson.cmake    rc_add_lesson(), which every lesson calls with one line
cmake/RcPlatform.cmake  maps this compiler to a toolchain identifier
librc/                  the library the learner accretes across the curriculum
tools/rcpp/             the tool that carries the repository, written in C++
atlas/errors/           the Failure Atlas, one file per catalogued error
phases/NN-slug/NN-slug/ the curriculum
docs/                   CONTRACT, PLATFORMS, CONTRIBUTING, ARCHITECTURE
```

Read [CONTRACT.md](CONTRACT.md) before adding anything. It is the enforced
contract, and `rcpp audit` is the enforcement. The rule behind the rules is that
those two are the same set: a rule written down without a numbered check will
drift, and the drift stays invisible because the badge stays green.

## House rules

- **C++ only under `phases/`.** No Python, no shell scripts, no Node. If
  automation is needed it becomes an `rcpp` subcommand, so the learner can read
  it.
- **Stdlib first.** The curriculum has no package manager. Eigen and Qt arrive
  in the phases that teach them, pinned, and nothing else is added without a
  reason written down.
- **No em dashes.** Rule L016 enforces it.
- **Commit subjects in past tense**, describing what the change did.
  `added the watchdog lesson`, `fixed the float comparison in the rate limiter`.
- **One lesson per commit.** A pull request adding five lessons has five commits.
- **Never commit generated files.** `build/` and `catalog.json` are ignored.
- **The reference implementation always passes**, on every toolchain the lesson
  claims. That is the gate.
- **The exercise always fails, and never crashes.** A learner must see a sentence
  naming what is wrong, so use `RC_REQUIRE` before indexing or dereferencing.

## What a good lesson looks like

The test suite is the hard part and it is where the teaching lives. A test that
only checks the happy path teaches nothing. The tests that matter are the ones
that catch the specific mistake the lesson exists to prevent:

- The battery test that walks the voltage range and requires the percentage
  never to decrease, which catches a whole family of wrong formulas.
- The drive test that faces the robot along y, which catches a swapped sine and
  cosine and nothing else.
- The watchdog test that constructs one and immediately asks whether it expired,
  which catches the single most dangerous default in the file.

Write those first. The prose is easier afterwards, because you know exactly what
the learner has to understand.
