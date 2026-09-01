# Reaching Without a Formula: The Jacobian, Measured

> Lesson 13-02 solved a two link arm with a page of trigonometry. Nobody has
> written that page for your arm, and for most arms nobody ever will.

**Type:** Build
**Time:** about 180 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 13-02

## The Problem

Closed forms exist for the arms that were **designed** to have them, and a great
many industrial arms were, deliberately, because a closed form is fast, exact,
and enumerates its solutions.

Yours may not be one. A camera on a pan and tilt head on a vehicle, a leg with a
linkage, an arm whose geometry came out of a file this morning: for any of those
the answer is not a formula, it is a search.

The good news is that the search needs almost nothing you do not already have.

## The Concept

### You can measure a derivative

The Jacobian is how the tool moves when each joint moves. One column per joint.

You do not have to derive it. Nudge a joint, see where the tool went, divide by
the nudge:

```cpp
nudged[joint] = held + step;   const Vec3 forward  = arm.tool(nudged);
nudged[joint] = held - step;   const Vec3 backward = arm.tool(nudged);
nudged[joint] = held;

column = (forward - backward) / (2.0 * step);
```

That works for **any** chain, which is the entire reason this method is worth
having. The forward kinematics from 13-01 is the only thing it needs.

Note the third line. Nudging a shared array and forgetting to put it back gives
a Jacobian whose later columns were computed at a configuration that no longer
matches the first, and nothing about the result will look wrong.

### Smaller is not more accurate

This is the part that surprises people, and it is worth having the numbers.

A difference has two errors moving in opposite directions. **Truncation**, the
gap between a difference and a derivative, shrinks as the step shrinks. **Rounding**,
the digits lost when subtracting two nearly equal numbers, grows as the step
shrinks.

Measured against a derivative that can be written down exactly:

| step | error |
|---|---|
| 1e-1 | 1.21e-03 |
| 1e-2 | 1.21e-05 |
| 1e-4 | 1.21e-09 |
| **1e-6** | **3.39e-11** |
| 1e-8 | 4.32e-09 |
| 1e-10 | 6.26e-07 |
| 1e-12 | 1.33e-04 |
| 1e-14 | 4.17e-03 |

**A step of 1e-14 is worse than a step of 1e-1.** The instinct that smaller is
more careful is exactly backwards past the middle of that table.

Use a **central** difference, both nudges. Its error falls as the square of the
step rather than linearly, which at 1e-6 is about six orders of magnitude better
than a forward difference, for one extra evaluation.

### Stepping downhill needs no matrix inverse

The obvious thing to do with a Jacobian is invert it. For a non square one that
means a pseudo-inverse, which means a linear solve, which means a great deal of
code before anything moves.

There is a simpler step that works: move each joint by **how much moving it
would help**, which is the dot product of its column with the error.

```cpp
for (std::size_t joint = 0; joint < columns.size(); ++joint)
  angles[joint] += gain * dot(columns[joint], error);
```

That is the transpose of the Jacobian applied to the error. It is the simplest
method that works, it is not the fastest, and what it buys is that every line of
it can be read.

### It has to be able to give up

An iterative solver improves a guess, and nothing in the arithmetic knows
whether the guess can be improved into a correct answer.

For an unreachable target the error simply stops going down. No exception, no
special value, no signal. `while (error > tolerance)` never leaves.

So: cap the iterations, and **report** what happened.

```cpp
struct ReachResult {
  std::vector<double> angles;
  int iterations = 0;
  double error = 0.0;
  bool converged = false;   // reported, never assumed
};
```

The angles are still returned on failure, and are still useful: a starting point
for next time, something to draw. What they are not is a command, and the only
thing separating them from one is that flag.

Return the error and the iteration count too. Stopping at 1e-7 and stopping at
0.4 are different situations, and a count that has grown tenfold is a warning
about the next section.

### Where it slows down, and then stops

Measured, approaching the edge of the workspace:

```text
target 0.600   390 iterations
target 0.750   1281 iterations
target 0.790   5236 iterations
target 0.799   gave up, error 8.7e-07
```

The arm straightens, the Jacobian columns line up, and the direction that would
take the tool further out stops existing. It is the same singularity lesson
13-02 found when the two closed form answers merged, seen from the solver's side
instead of the geometry's.

Worth knowing for two reasons. It is not a bug, so time spent looking for one is
wasted. And convergence slowing sharply is how it announces itself **before** it
stops working, which is a usable warning if anybody is looking at the iteration
count.

The real fix is damped least squares, which trades a little accuracy for a step
that stays finite. That needs a linear solve, and it is the point at which
bringing in a matrix library stops being laziness.

### Checking a solver that searches

A solver that converges to something is not the same as a solver that converges
to the right thing.

For this arm there is an exact answer from 13-02, so the numerical result can be
compared against it. Measured, across five targets: within **5e-9 radians** of
one of the two closed form solutions, in 249 to 797 iterations.

That is the shape to look for whenever you write something approximate. Find a
case where the exact answer is known, even a special case, and check the
approximation against it there.

## Build It

Implement `jacobian` and `reach` in `exercise/solution.hpp`.

```
rcpp verify 13-03
```

The suite checks a column against the derivative of the closed form, prints the
step size curve, compares the solutions with the exact ones, walks the target
toward the workspace edge, and finishes on a five joint arm that has no formula
at all.

## Use It

Anything that can be evaluated can be inverted this way: a camera pose from
observed points, a calibration from measurements, a set of parameters from a
recorded response. The method never asks what the function is, only what it does
when you nudge it.

Which is also its weakness. It needs many evaluations, so it is far too slow to
put inside a fast control loop, and it is the right tool for planning a move
rather than for executing one.

## What Breaks First

- **A solver with no cap, or one that will not admit it failed.** See
  `E-KIN-0003`.
- **A step chosen by making it as small as possible.** See `E-KIN-0004`.
- **A stall at the workspace edge mistaken for a defect.** See `E-KIN-0002`.

## Ship It

`reach` joins `rc::kin`, and phase 13 can now be asked its question in both
directions on any arm: exactly where a formula exists, and approximately where
one does not, with the approximate answer checked against the exact one where
both are available.
