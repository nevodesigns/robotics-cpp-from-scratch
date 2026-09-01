# It Compiled, and the Answer Is Wrong

> The build is clean. The robot ends up two metres from where it should be. The
> code looks correct, and it looks correct because you wrote it.

**Type:** Build
**Time:** about 90 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 00-05, 00-03

## The Problem

Lesson 00-03 was about the build failing. This is the other state, and it is the
one you will spend far more time in: everything compiles, everything runs, and
the number at the end is wrong.

There is no caret pointing at the mistake. There is no message naming a line.
There is a robot that ended up in the wrong place, and five hundred steps of
arithmetic between the start and the answer.

The instinct is to reread the code until the mistake becomes visible. That works
occasionally, and it fails in exactly the case where you need it, because the
reason the mistake is invisible is that you already believe the code is right.

The alternative is to stop reading and start asking questions the code has to
answer.

## The Concept

### The shape of a wrong answer names the bug

Before looking anywhere, look at *how* it is wrong. A wrong number is not just
wrong; it is wrong in a way, and the way is a short list:

| what you see | what it usually is |
|---|---|
| exactly zero, always | integer division, which lesson 00-04 is about |
| out by a factor of two | something averaged, or applied twice |
| out by a factor of ten, or a thousand | a unit: degrees for radians, millimetres for metres |
| the sign is flipped | a subtraction the wrong way round |
| correct at the start, drifting later | an error that accumulates each step |
| correct for small inputs, wrong for large | a range or an overflow |
| out by about 1.57, or 3.14 | radians, and something is a quarter or half turn out |
| correct except at the boundary | a comparison that should be `<=` and is `<` |

That table is worth more than any debugger. It turns "it is wrong" into a
hypothesis, and a hypothesis is something you can test in one minute.

### Ask questions that have to be true

A robot driving with both wheels at the same speed **cannot** change heading.
Not "should not": cannot, by the definition of the model. So:

```cpp
if (!same_heading(pose.theta, start.theta, 1e-9)) { /* the turn rate is wrong */ }
```

That is a different activity from reading the code. It is checking a property,
and each property that holds removes a whole region of the code from suspicion.

Four properties cover most of a motion model:

- Both wheels equal: the heading does not change.
- Wheels equal and opposite: the position does not change.
- Forward, then back the same amount: it returns to where it started.
- Distance covered equals speed times time, whatever the heading.

When the third one fails and the first two pass, the bug is in how a step is
undone, not in the turning and not in the geometry. That is most of the search
done, without reading a line.

### Cut the problem in half

When no property is obviously broken, the fastest route is bisection.

Five hundred steps produce a wrong answer. Does one step? If one step is
correct and five hundred are not, the mistake accumulates, and you can stop
looking at the geometry. If one step is already wrong, you have reduced five
hundred steps of arithmetic to one, and you can compute the right answer by
hand.

Halve the input, not the code. The smallest input that still fails is the
cheapest thing you will ever debug.

### Two comparisons that lie

Both of these produce a false report of a bug, which is worse than a missed one,
because you go looking for something that is not there.

**Fractional numbers are not exactly equal.** Adding 0.1 ten times does not give
1.0, and it never will, on any machine. Compare with a tolerance:
`std::fabs(a - b) <= 1e-9`. Lesson 00-04 explains why.

**Angles are not numbers.** A heading of `3.14159` and a heading of `-3.14159`
point the same way, and subtracting them gives 6.28, which is a full turn and
means nothing. Take the difference through `atan2(sin(d), cos(d))`, which throws
away the whole turns, and then compare.

### Printing is still allowed

None of this replaces looking at a value. Print the pose every fifty steps and
the drift is visible immediately.

Two habits make printing worth the interruption. Print at the **boundary**, the
inputs and the result, before printing anything in the middle: half the time the
input was already wrong and nothing between them mattered. And print the value
you are **not** suspicious of, because that is where it will be.

## Build It

Implement six checks in `exercise/solution.hpp`:

- `distance_between`, how far apart two poses are.
- `heading_difference`, the shortest signed turn from one heading to another.
- `same_position`, `same_heading`, `same_pose`, each within a tolerance.
- `moved_expected_distance`, whether the ground covered matches speed and time.

```
rcpp verify 00-06
```

The suite uses them twice. First on a real trajectory from the model you wrote
in lesson 00-05, asking the four questions above. Then on three trajectories
that are deliberately wrong, requiring your checks to notice, because a check
that cannot fail is not a check.

## Use It

These are the questions to ask of anything that moves, and later phases ask them
of a controller, an estimator and a real robot. The habit generalises well
beyond robots: for any calculation you do not trust, there is usually something
that must be true about its answer, and testing that is faster than reading it.

## What Breaks First

- **Comparing fractional numbers exactly.** They will not be equal, and the
  report of a bug is itself the bug. See `E-NUM-0003`.
- **Subtracting two headings.** Angles wrap, and the arithmetic difference is
  the long way round. See `E-NUM-0006`.
- **Reading the code instead of questioning it.** See `E-DEBUG-0002`.

## Ship It

The checks join `rc::sim` beside the model they inspect. From here, any lesson
that produces a trajectory can say whether it is believable, rather than whether
it looks about right.
