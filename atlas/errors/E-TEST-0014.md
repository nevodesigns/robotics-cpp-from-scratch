id: E-TEST-0014
title: A hundred percent coverage and the bug still shipped
match: coverage says every line ran
platforms: linux, windows
teaches: 05-07-the-line-nobody-ran
---

## Symptom

The coverage report says 100 percent. A defect ships anyway, in a line the
report says was covered.

Or the softer version: coverage is high, climbing, reported in every status
update, and the defect rate is unchanged.

## Cause

Coverage answers exactly one question: **did this line run.** It does not ask
whether anything checked the result.

Measured for lesson 05-07. A program calling five functions and asserting
nothing at all:

```
Lines executed:100.00% of 5
```

Zero lines never run. And one of those functions contained a deliberate
defect, a `>` where the intention was `>=`, which survived untouched because
nothing ever looked at what it returned.

So coverage is a **floor, not a score**:

| | |
| --- | --- |
| a line that never ran | certainly not tested |
| a line that ran | may or may not be tested |

The first half is genuinely valuable and is the reason to measure it at all. A
report naming lines that never executed is a list of things you have no
evidence about, and that list is usually shorter and more interesting than
anyone expects.

The second half is why a coverage target is a poor goal. There is a short path
from "we need 90 percent" to tests that call everything and assert nothing,
and those tests cost real time to run and maintain while proving nothing.

## Fix

Use coverage to find the lines nobody ran, and something else to decide
whether the lines that ran are tested.

```cpp
#include <rc/test/coverage.hpp>

const rc::test::LineCoverage c = rc::test::parse_gcov(text);
std::cout << c.never_run << " line(s) never ran of " << c.executable << "\n";
```

**Print the executable count beside the percentage, always.** 100 percent of 0
and 100 percent of 600 are the same number and not the same claim, and a file
with no code in it reports perfect coverage.

**Read the never-run list, do not watch the percentage.** It names specific
lines. On lesson 01-13's own artifact it named the `--name=value` branch of a
private lookup and that function's not-found return: both reachable, neither
exercised, because every call in the suite happened to use the other spelling.

**Ask for branch coverage too**, with `gcov -b`. On a line of the form
`return a && b`, measured: lines executed 100 percent of 2, branches executed
100 percent of 6, taken at least once 50 percent of 6. Three numbers from one
run, and the first is the one people quote.

**For whether a test catches anything, change the code and see.** Lesson 05-03
is about that directly: break something on purpose and confirm the suite goes
red. A suite that stays green is a suite that was not testing that line,
whatever the coverage report said.

`E-TEST-0015` is the other half of this: what coverage cannot see at all.
