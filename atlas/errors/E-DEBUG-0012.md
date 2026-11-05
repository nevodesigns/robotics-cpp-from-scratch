id: E-DEBUG-0012
title: Ninety eight warnings from one defect
match: [0-9]+ warnings generated
platforms: linux, windows
teaches: 04-08-which-warnings-to-believe
---

## Symptom

A build reports a number of warnings that looks like a crisis. Ninety eight.
Fifty one. The same message, over and over, from the same file.

Fixing one of them removes thirty.

## Cause

A warning is reported per **translation unit**, and a defect in a header is
compiled once per file that includes it.

Measured for lesson 04-08: one bad comparison in one header, three source
files including it, compiled separately.

| | |
| --- | --- |
| warning lines | **3** |
| distinct `file:line:col` sites | **1** |

So the line count scales with the size of the project and the site count does
not. This repository's own "98 of `-Wclobbered`" came from a single
`setjmp`/`longjmp` structure in one header, and its "51 of
`-Wmismatched-new-delete`" from one leak checker included by six lessons.

Two further reasons the headline number misleads.

**Compilers disagree about the count for identical source.** Measured on one
file: gcc 4 warning lines at 3 sites, clang 3 lines at 3 sites, because gcc
reports a partially braced struct once per missing member and clang once per
brace.

**Most lines in the output are not warnings.** gcc surrounds each one with
context that a naive count will include:

```
w.cpp: In instantiation of 'int cmp(T, std::size_t) [with T = int]':
w.cpp:3:23:   required from here
w.cpp:2:59: warning: comparison of integer expressions ... [-Wsign-compare]
    2 | template <class T> int cmp(T n, std::size_t m) { return n < m; }
      |                                                         ~~^~~
3 warnings generated.
```

One warning, six lines. The context lines, the source echo, and clang's
summary line are all easy to count by accident, and `note:` lines are too.

## Fix

Count distinct sites, and key on the full shape rather than the word.

```cpp
#include <rc/test/warnings.hpp>

const rc::test::WarningCount c = rc::test::count_warnings(build_log);
std::cout << c.lines << " line(s) at " << c.sites << " site(s)\n";
```

Keying on `file:line:col: warning: ` excludes every one of the context forms
above, because none of them has all three position fields followed by that
marker.

One detail that matters on Windows: find the marker first and then walk
**backwards** over two numeric colon fields. A path like `C:\src\a.cpp` starts
with a drive letter, so anything keyed on the first colon is wrong on one of
the four toolchains this curriculum claims.

Then use the right number for the right question. **Sites** to decide how much
work there is. **Lines** to decide how noisy the build is, which is a real
question too: a wall of repeated warnings hides the new one, which is the
argument for a written ledger of the ones already dismissed
(`E-BUILD-0004`).
