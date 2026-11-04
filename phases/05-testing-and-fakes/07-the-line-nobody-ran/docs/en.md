# The Line Nobody Ran: Coverage as a Floor, Not a Score

> The author counted one real file by hand three times before agreeing with
> gcov. Two of the three wrong answers were wrong in the flattering direction.

**Type:** Build
**Time:** about 115 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 01-05, 04-07, 05-01, 05-03

## The Problem

Lesson 04-07 ended on a sentence that was left hanging:

> A sanitizer only ever reports what the test actually ran, so the lane is
> worth exactly as much as the suite's coverage.

So what is it? You have 138 reference tests in this repository and no idea
what fraction of the code they touch. This lesson finds out, and the answer
about one of them is uncomfortable.

The tool is `gcov`, which ships with gcc, so there is nothing to install.
Compile with `--coverage`, run the program, and gcov writes an annotated copy
of the source with a count against every line.

## The Concept

### The headline, before the mechanics

**Coverage answers exactly one question: did this line run.** It does not ask
whether anything checked the result.

Measured: a program calling five functions and asserting nothing at all.

```
Lines executed:100.00% of 5
```

Zero lines never run. One of those five functions contained a deliberate
defect, a `>` where the intention was `>=`, and it survived untouched because
nothing ever looked at what it returned.

So coverage is a **floor, not a score**:

| | |
| --- | --- |
| a line that never ran | certainly not tested |
| a line that ran | may or may not be tested |

The first half is why it is worth measuring. A list of lines that never
executed is a list of things you have no evidence about at all, and it is
usually shorter and far more interesting than anyone expects.

The second half is why a coverage target makes a poor goal. There is a short
path from "we need 90 percent" to tests that call everything and assert
nothing, and those cost real time to run and maintain while proving nothing.

### The uncomfortable measurement

Lesson 01-13's suite, against its own artifact:

```
Lines executed:89.06% of 64
```

Seven lines never ran, and they are specific. Two of them are the
`--name=value` branch of a private lookup and that function's not-found
return. Both reachable. Neither exercised, because every call in the suite
happened to use the other spelling and every name it looked up existed.

That is the useful output of a coverage run: not the percentage, the list.

### Five count forms, three of them traps

```
        -:    0:Source:/repo/librc/include/rc/core/args.hpp
        -:   12:// a comment, which is not code
        1:   20:  return value;
       1*:   27:  table[i] = compute(i);
    #####:  159:      rest.push_back(args_[i]);
    =====:   63:  }
```

| field | meaning |
| --- | --- |
| `-` | not code: a comment, a blank, a declaration |
| `digits` | ran that many times |
| `digits*` | ran, but **not every instance** did, which happens with templates and inlining |
| `#####` | code, never ran |
| `=====` | code, never ran, because the block holding it was never entered |

And lines numbered `0` are gcov's own header, naming the source and the data
files. They carry a `-`, so they inflate nothing, but the line number has to
be parsed to skip them.

### Three attempts to agree with gcov

This is the part worth copying, because it is how the format was actually
learned. The suite prints it:

```
  attempt                             executable  never run  covered
  ##### only                                  56          5   91.07%
  ##### and =====                             56          7   87.50%
  and digits* counted as code                 64          7   89.06%
  gcov's own figure                           64          7   89.06%
```

Counting only `#####` gave five never-run lines where the answer is seven.
Counting both markers fixed the never-run count and still disagreed on the
total, because `digits*` was not being counted as code at all.

**Two of the three wrong answers were wrong in the flattering direction**,
which is the direction a hand count tends to go. That is the argument for
parsing it in code and checking against gcov's own figure the first time.

### Lines, branches, and outcomes are three numbers

Ask for branch coverage with `gcov -b`. On a single line, `return a && b`,
exercised with one input:

```
Lines executed:100.00% of 2
Branches executed:100.00% of 6
Taken at least once:50.00% of 6
```

Three numbers, one run, and the first is the one people quote.

The distinction between the second and third is easy to misread.
**Branches executed** means the branch instruction ran. **Taken at least
once** means both of its outcomes occurred. Half the outcomes of that `&&`
never happened, and line coverage had nothing to say about it.

### What coverage cannot see

| blind spot | the instrument that does see it |
| --- | --- |
| the input you never thought of | property tests, 05-04 |
| the order you never ran | running the suite backwards, 05-06 |
| the interleaving you never hit | the thread sanitizer, phase 07 |
| the defect no tool reports | design, 02-09 and 03-11 |
| **the line that should exist and does not** | **a review, and a specification** |

The last row is the largest blind spot and the only one with no tool in it.
Coverage can only report on code that was written. A missing bounds check, a
missing timeout, a case the author never considered: none of these appear in
any report, because there is no line to be uncovered.

Lesson 05-06's measurement makes the second row concrete. Two probes sharing
one `double` scored 1 of 2 forwards and 2 of 2 backwards, and both orders
execute the identical lines. Coverage is the same for a suite that is order
dependent and one that is not.

## Build It

Two functions.

`parse_gcov`, one line at a time, splitting on the first two colons. Skip
lines numbered 0. Then decide what the count field says, and all five forms
matter. Set `ok` once any line parses, so that text which was never a gcov
report can be told from a valid report about a file with no code: those are
different facts, and a tool that confused them would report perfect coverage
of a file it had failed to read.

`percent_covered`, to gcov's own definition. Mind the division: integer over
integer is lesson 00-04's trap, and here it would report 0 percent for
everything short of perfect.

Then run it against a real file and compare with what gcov printed. Agreeing
on the first try would be unusual.

## Use It

```bash
g++ -std=c++17 -O0 --coverage test_args.cpp -o t && ./t
gcov t-test_args          # the base name of the .gcda, not the binary
```

```cpp
#include <rc/test/coverage.hpp>

const rc::test::LineCoverage c = rc::test::parse_gcov(text);
std::cout << c.never_run << " line(s) never ran of " << c.executable << "\n";
```

Two notes for a first coverage build. **Use `-O0`**, or the report describes
the optimiser's code rather than yours and `digits*` becomes common. **Delete
the `.gcda` files between runs**, or counts accumulate and a line that ran
only in a previous run looks covered.

And print the executable count beside the percentage, always.

## What Breaks First

`E-TEST-0014` is a hundred percent coverage with the bug still shipping.

`E-TEST-0015` is what coverage cannot see, with the instrument for each.

`E-DEBUG-0011` is a count that disagrees with gcov, including the invocation
that produces `cannot open notes file`.

## Ship It

`rc/test/coverage.hpp`, in `namespace rc::test`.

Verified against gcov on four real files: 64 executable with 7 never run, 15
with 0, 142 with 0, and 5 with 0. Same counts, same percentages.

A practical sequence that works: get the never-run list down to something you
can read and justify line by line, then stop looking at the percentage and
spend the effort on the rows of that blind-spot table instead. A suite at 85
percent whose uncovered lines you can each name and dismiss is in better
shape than one at 98 percent that nobody has read.
