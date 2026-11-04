id: E-DEBUG-0011
title: A coverage report that disagrees with gcov
match: (Lines executed:|cannot open notes file|No executable lines)
platforms: linux, windows
teaches: 05-07-the-line-nobody-ran
---

## Symptom

A hand-written count of a `.gcov` file does not match the percentage gcov
printed for the same file. Usually the hand count is flattering.

Or gcov refuses to say anything:

```
./t13.gcno:cannot open notes file
./t13.gcda:cannot open data file, assuming not executed
No executable lines
```

## Cause

Two separate problems, and both happened while writing lesson 05-07.

### The counting

The count field has five forms and three of them are easy to get wrong.

| field | meaning |
| --- | --- |
| `-` | the line is not code: a comment, a blank, a declaration |
| `digits` | it ran that many times |
| `digits*` | it ran, but **not every instance** of it did, which happens with templates and inlining |
| `#####` | it is code and it never ran |
| `=====` | it is code and it never ran, because the block holding it was never entered |

Measured on lesson 01-13's artifact, where gcov reports 89.06 percent of 64:

| attempt | executable | never run | reported |
| --- | --- | --- | --- |
| counting `#####` only | 56 | 5 | 91.07% |
| counting `#####` and `=====` | 56 | 7 | 87.50% |
| and counting `digits*` as code | **64** | **7** | **89.06%** |

The third agrees with gcov exactly. Two of the three wrong answers were wrong
in the flattering direction, which is the direction a hand count tends to go.

Also skip lines numbered `0`. Those are gcov's own header, naming the source
file, the `.gcno` and `.gcda` files and the number of runs, and they carry a
`-` in the count field so they are harmless to the never-run count and will
inflate nothing. They matter because a report consisting only of headers is
valid output about a file with no code, which is a different fact from text
that was not a gcov report at all.

### The invocation

gcov wants the **base name of the data file**, not the executable and not the
source. If the compiler produced `t13-test_args.gcda`, then:

```bash
gcov t13-test_args          # right
gcov t13                    # cannot open notes file
gcov -o . t13               # the same, more confidently
```

The naming depends on how the object files were named, which for a
single-translation-unit build is `<binary>-<source>`. `ls *.gcda` is the
reliable way to find out.

## Fix

Parse it once, in code, and compare against gcov's own figure the first time.

```cpp
#include <rc/test/coverage.hpp>

const rc::test::LineCoverage c = rc::test::parse_gcov(text);
// executable, never_run, partly_run, and ok for "was this a report at all"
```

Verified against gcov on four real files: 64 executable with 7 never run, 15
with 0, 142 with 0, and 5 with 0. Same counts, same percentages.

Two notes for a first coverage build.

**Build with `-O0`.** At higher optimisation levels lines are merged, moved
and removed, so the report describes the optimiser's code rather than yours
and the `digits*` form becomes common.

**Delete the `.gcda` files between runs**, or the counts accumulate across
runs and a line that ran only in a previous run looks covered.
