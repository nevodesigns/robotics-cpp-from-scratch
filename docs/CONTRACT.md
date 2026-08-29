# The Lesson Contract

Every rule here has a numbered check in `rcpp audit`. A rule without a check is
not a rule, and adding one to this document without adding its check is itself a
failure. That is the whole point: a curriculum whose documentation and whose
gate disagree will drift, and the drift is invisible because the badge stays
green.

Run it before every commit:

```bash
./build/default/bin/rcpp audit
```

## Directory shape

```
phases/NN-phase-slug/NN-lesson-slug/
  lesson.json
  CMakeLists.txt        exactly one line: rc_add_lesson()
  docs/en.md
  exercise/             the learner's copy, ships failing
  reference/            the worked implementation
  tests/                one suite, run against both
  quiz.json
```

## Rules

| Rule | What it checks |
|---|---|
| L001 | Phase and lesson directories are named `NN-slug` |
| L002 | `lesson.json` exists and is valid JSON |
| L003 | `lesson.json` has id, title, a valid type, positive minutes, and a hardware tier from 0 to 3 |
| L004 | The id matches the lesson's position on disk, and no id is used twice |
| L005 | `docs/en.md` exists, opens with a level one heading, and carries a one line motto as a block quote near the top |
| L006 | `docs/en.md` contains all six required sections |
| L007 | `CMakeLists.txt` exists at the lesson root and calls `rc_add_lesson()` |
| L008 | `reference/` holds a worked implementation |
| L009 | `tests/` holds at least one test file |
| L010 | `quiz.json` holds exactly six questions, one pre, three check and two post, each with four options, a correct index in range, and an explanation |
| L011 | Every platform named in `lesson.json` is a known toolchain identifier |
| L012 | Every prerequisite resolves to a real lesson, and the graph has no cycle |
| L013 | Every id in `breaks_first` resolves to an atlas entry, every atlas pattern compiles, and every entry has a symptom, a cause and a fix |
| L014 | A hardware tier of 2 or 3 declares a fallback, so no learner is blocked for lacking hardware |
| L015 | Only C++, CMake, markdown and data files appear under `phases/`, which is the one language rule |
| L016 | No em dash appears anywhere in the repository |
| L017 | The lesson is C++17: `cxx_standard` is 17, and no C++20 or C++23 facility is used directly instead of its `rc::` equivalent |
| L018 | No discarded style: `NULL`, `malloc`, `printf`, `strcpy`, `typedef`, `using namespace std` and friends |
| L019 | Every platform a lesson claims is defined in `platforms.json`, and a lesson needing Qt only claims a toolchain whose lane installs Qt |
| L020 | A lesson that cites another lesson by number cites one that exists. Promises to a phase are fine, a promise to a specific lesson is a claim |
| L021 | A lesson only uses a facility that it or one of its prerequisites declares in `teaches`, so nothing is met before it is taught |

## The six required sections

| Section | Its job |
|---|---|
| The Problem | A concrete failure the learner can feel, not an abstract motivation |
| The Concept | The mental model, with diagrams as SVG or Mermaid only |
| Build It | Implement it by hand, pointing at `exercise/` and the failing tests |
| Use It | The same job through the production library, with the trade off named |
| What Breaks First | The three failures a newcomer will actually hit, each linked to an atlas entry |
| Ship It | What graduates into `librc` and where it is used again |

## Nothing is met before it is taught

The floor learner has never programmed. That promise is only kept if a lesson
never uses something no earlier lesson explained, and a prerequisite graph that
does not check this is decoration.

So a lesson declares what it introduces:

```json
"teaches": ["std::string", "std::string_view"]
```

Rule L021 scans each lesson's `exercise/` and `reference/` for a small
vocabulary of tracked facilities and requires each one to be declared by that
lesson or by something in its prerequisite chain, however deep. Tests are
exempt, because they are written by the author rather than completed by the
learner.

The vocabulary is deliberately short. Every entry has one obvious owning lesson,
so the rule stays quiet unless something is genuinely out of order. It was added
after an audit by hand found `std::string` first used in lesson 02-06 and taught
nowhere, and it caught a second case on its first run.

**What Breaks First is the section that makes this curriculum different.**
Writing it forces the author to name the real failures, and each one must
resolve to a catalogued atlas entry. The Failure Atlas is therefore built as a
by product of authoring rather than as a separate heroic project.

## lesson.json

```json
{
  "schema": 1,
  "id": "14-01-pid-from-scratch",
  "title": "PID From Scratch",
  "type": "build",
  "minutes": 120,
  "platforms": ["ubuntu-22.04-gcc11", "ubuntu-24.04-gcc13", "windows-msvc2022"],
  "hardware_tier": 1,
  "requires": ["01-04-arrays-and-spans"],
  "cxx_standard": 17,
  "qt": { "modules": ["Core"], "min_version": "6.2" },
  "ros2": null,
  "artifact": { "module": "rc::control", "note": "what this lesson leaves behind" },
  "breaks_first": ["E-CTRL-0001", "E-CTRL-0002", "E-NUM-0003"]
}
```

Toolchain identifiers are defined in [`platforms.json`](../platforms.json), which
is the single source of truth. `rcpp audit` checks lesson claims against it and
the continuous integration matrix is generated from it by
`rcpp platforms --matrix`, so a lane and a claim cannot drift apart.

**Why that matters more than it looks.** A Qt lesson claiming a toolchain whose
lane has no Qt is not built there at all: `rc_add_lesson` skips it, the lane goes
green, and the claim appears satisfied without a single line having been
compiled. A silent skip is indistinguishable from a pass, which is the worst
failure a gate can have. Rule L019 catches it in the manifest, and
`-DRC_STRICT_CLAIMS=ON`, which CI sets, turns the skip itself into a hard error.
Locally the skip remains, so a learner without Qt can still build everything
else.

Hardware tiers: 0 software only, 1 simulated, 2 cheap hardware under sixty
dollars, 3 a real robot. Tiers 2 and 3 must ship a fallback.

## The language baseline

Lessons are C++17. Facilities from later standards are reached through
`rc/core/compat.hpp`, which supplies `rc::span`, `rc::format` and `rc::expected`
and becomes an alias for the standard type on a toolchain that has it.

This is not conservatism. A learner who has written a span understands
`std::span` completely on the day they meet it, and that is only possible while
it is still missing. Rule L017 enforces the baseline so the teaching device does
not quietly erode.

Modern C++17 style is expected throughout, and rule L018 enforces the floor of
it: `nullptr` not `NULL`, containers and smart pointers not `malloc`, `using`
not `typedef`, `std::cout` or `rc::format` not `printf`, and the `std::` prefix
written out rather than a blanket `using namespace std`.

## Tests

The same suite runs against `exercise/` and against `reference/`. Which one it
sees is decided by the include directory, so both are graded by an identical
standard.

- Tests include `"solution.hpp"`. Both variants provide that file with the same
  interface.
- Use `RC_CHECK` and its relatives to record a failure and keep going.
- Use `RC_REQUIRE` and `RC_REQUIRE_EQ` whenever the lines that follow would be
  meaningless or dangerous if the check failed, such as a size before indexing.
  A learner must see a sentence describing what went wrong, never a crash.
- Compare fractional numbers with `RC_CHECK_NEAR`, never for exact equality.

## Exercises may be broken on purpose

Some lessons ship an exercise that does not compile, because the error is the
lesson. Exercise targets are therefore excluded from the default build, so a
plain `cmake --build` stays green and fast. `rcpp verify` builds them by name.

The reference implementation must always build and always pass, on every
toolchain the lesson claims. That is the gate continuous integration enforces.
