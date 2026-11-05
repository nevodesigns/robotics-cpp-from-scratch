# Which Warnings to Believe: The Compiler's Opinions, Counted and Judged

> Every warning named in this lesson is one this curriculum actually hit.
> Two were false. One warned about the safe thing. One was silent about the
> dangerous thing. One was right in a way nobody expected.

**Type:** Build
**Time:** about 115 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 00-03, 01-05, 04-07

## The Problem

Lesson 00-03 was about errors. An error stops the build, so it deals with
itself: you cannot ignore it and you cannot ship past it.

A warning does neither. It is the compiler saying *I think this might be
wrong*, and then compiling it anyway. Which means somebody has to decide, and
the deciding is the whole skill.

Both extremes are expensive. Treat every warning as a defect and you lose days
to false positives. Ignore them and you lose the one in ten that was the bug.
This curriculum has done both, and every case below is its own.

## The Concept

### First, the number is wrong

A build says 98 warnings. That is not 98 defects, and it is usually not close.

A warning is reported per **translation unit**, and a defect in a header is
compiled once per file that includes it. Measured: one bad comparison in one
header, three source files including it, compiled separately.

| | |
| --- | --- |
| warning lines | **3** |
| distinct `file:line:col` sites | **1** |

The line count scales with the size of the project. The site count does not.

This repository's "98 of `-Wclobbered`" came from a single `setjmp` structure
in one header, and its "51 of `-Wmismatched-new-delete`" from one leak checker
included by six lessons.

**Count sites to decide how much work there is. Count lines to decide how
noisy the build is**, which is a real question too, because a wall of repeated
warnings hides the new one.

### The count is not even comparable between compilers

Measured on one file:

| | warning lines | distinct sites |
| --- | --- | --- |
| gcc 11 | **4** | 3 |
| clang 14 | **3** | 3 |

Same source. gcc reports a partially braced struct once per missing member,
clang once per brace.

### Most of the output is not warnings

```
w.cpp: In instantiation of 'int cmp(T, std::size_t) [with T = int]':
w.cpp:3:23:   required from here
w.cpp:2:59: warning: comparison of integer expressions ... [-Wsign-compare]
    2 | template <class T> int cmp(T n, std::size_t m) { return n < m; }
      |                                                         ~~^~~
3 warnings generated.
```

One warning, six lines. The instantiation context, the `required from here`,
the two source echo lines and clang's summary are all easy to count by
accident, and `note:` lines are too.

Keying on the full `file:line:col: warning: ` shape excludes every one of
them, which is why the parser in this lesson insists on all three position
fields.

One detail for Windows: find the marker, then walk **backwards** over two
numeric colon fields. `C:\src\a.cpp` begins with a drive letter, so anything
keyed on the first colon is wrong on one of the four toolchains here.

### Then, the judging

Four cases, all measured in this curriculum.

**Two were false positives.** 51 of `-Wmismatched-new-delete`: gcc traces a
`delete[]` into the replaced global `operator delete[]`, sees `std::free`, and
pairs it with the `new[]` at the call site without modelling that
`operator new[]` was replaced too. The pairing is correct, and a genuine
mismatch in the same code *is* caught, at runtime, by the address sanitizer as
`alloc-dealloc-mismatch`. One `-Wstringop-overread` came from inside
`<bits/stl_algobase.h>` on gcc 11, not from this repository at all.

A `#pragma GCC diagnostic` did not suppress the first one. It is emitted in a
**late pass after inlining**, where the lexical region the pragma describes no
longer exists. It was tried here and reverted, because leaving it would have
implied a suppression that was not happening.

**One warned about the safe thing.** `Pose p = {1.5};` warns as
`-Wmissing-field-initializers` and is completely defined: unmentioned members
are value initialised, so `p` is `{1.5, 0.0, 0.0}`. Give the members default
initialisers and the identical brace is silent on both compilers. So the
warning means *I cannot tell whether you meant this*, and the answer belongs
in the type.

**One was silent about the dangerous thing.** `-Wdelete-non-virtual-dtor`
fires only when the base already has some other virtual function. Measured
silent on both compilers for a base with no virtual functions at all, which is
the shape a handle or a resource wrapper has, and exactly the case that bites.
Lesson 03-10's answer is a `static_assert`, not a warning.

**One was right in a way nobody expected.** clang's
`-Wunused-lambda-capture` fired on a lambda whose body was only
`sizeof(payload)`. The capture was not *required*, because `sizeof` is
unevaluated, so a measurement of what a capture costs rested on a capture the
compiler was entitled to drop. Both compilers kept it and the numbers were
identical either way. The warning was still pointing at a real defect in the
test.

And one more worth knowing: clang suppresses `-Wsign-compare` when the signed
operand is `const`, measured in lesson 01-07. Two compilers, one source, and
the quieter one was not the safer one.

### The three questions

Can this actually happen here? Can the compiler see what you can see? Is the
fix cheaper than the argument?

Then **write the decision down.** Not a `#pragma`, which failed above, and not
a global `-Wno-`, which would hide the warning for learner code as well. A
ledger:

```
docs/PROGRESS.md, "Known warnings":
  51 of -Wmismatched-new-delete, from rc/test/leak_check.hpp ...
   1 of -Wstringop-overread, inside <bits/stl_algobase.h> on gcc 11 ...
```

A warning dismissed in writing costs nothing to carry. A warning dismissed in
your head costs the next person the whole investigation again, and this
repository made that mistake twice before the ledger existed.

### Why -Werror is on checks/ and nowhere else

`checks/` compiles with `-Wall -Wextra -Werror`, and
`checks/a_warning_that_became_an_error.cpp` is one unused parameter. Measured:

| | |
| --- | --- |
| `g++ -Wall -Wextra` | warning, exit **0** |
| `g++ -Wall -Wextra -Werror` | error, exit **1** |

Same compiler, same source, same diagnostic, two outcomes.

The lesson targets deliberately do not carry it. A learner's first encounter
with a warning should be a message they can read and decide about, not a build
that refuses to run; a course that fails your build on a warning in lesson
four teaches you to switch warnings off.

A real project should turn it on. What makes that bearable is everything above:
count sites, judge each one, and keep the ledger so a new warning is an event.

## Build It

Four functions, and the first is the keystone.

`warning_site_length`: find `": warning: "`, then walk backwards over exactly
two numeric colon fields, and insist a file name remains. Return the length of
that prefix, or 0. Backwards, for the drive letter.

`warning_flag`: the bracketed flag, **including its dash**, as the compiler
wrote it. Empty when there is no bracket, because some warnings cannot be
switched off.

`count_warnings`: lines, and distinct sites.

`count_with_flag`, in terms of the other two, so a note mentioning
`-Wsign-compare` cannot be counted as one.

## Use It

```cpp
#include <rc/test/warnings.hpp>

const rc::test::WarningCount c = rc::test::count_warnings(build_log);
std::cout << c.lines << " line(s) at " << c.sites << " site(s)\n";
```

## What Breaks First

`E-BUILD-0004` is the warning that was not a defect, including why the
`#pragma` did nothing.

`E-CPP-0061` is the warning that pointed the wrong way, in both directions.

`E-DEBUG-0012` is ninety eight warnings from one defect.

## Ship It

`rc/test/warnings.hpp`, in `namespace rc::test`.

Verified against real build logs from both compilers, matching an independent
`grep` count on each: gcc 4 lines at 3 sites, clang 3 lines at 3 sites, and
the three-translation-unit header log at 3 lines and 1 site.

Two small things the suite caught while this was written, both kept in the
lesson's comments because they are the ordinary way this goes. A test asserted
a site length of 16 where the answer was 15. And the header's comment promised
the flag without its leading dash while the code returned it with one, which
is the kind of disagreement that survives review and does not survive a test.
