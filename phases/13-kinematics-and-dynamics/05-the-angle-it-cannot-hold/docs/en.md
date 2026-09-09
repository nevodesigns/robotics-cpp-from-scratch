# The Angle It Cannot Hold: Joint Limits and the Reachable Set

> An arm whose links allow 25140 cells of a 1 cm grid can hold poses in 13878 of
> them. A solver that computes one elbow and stops finds 9734.

**Type:** Build
**Time:** about 150 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 13-02

## The Problem

Lesson 13-02 solved the arm exactly. Given a point, it returns the two ways of
holding the arm that put the tool there, or says the point is beyond the links.
Every one of those answers is correct.

Not every one of them is a pose the arm can take up. Real joints have stops.
Cables have a number of turns in them. A link that folds far enough runs into
the base, or into the second link, or into the operator. The set of poses a
machine can actually hold is smaller than the set the geometry allows, and the
part it loses is not a rim around the outside.

This lesson measures what the limits take, and then deals with three ways of
getting them wrong, each of which produces a machine that works and is wrong
rather than a machine that stops.

## The Concept

### An angle is a direction, not a quantity

Start with the smallest of the three, because everything else is built on it.

`std::atan2` answers in `(-pi, pi]`. The closed form solver builds the shoulder
angle by subtracting one `atan2` from another, so what comes back covers roughly
`(-2pi, 2pi)`. The value `-6.10` and the value `0.183` describe the same
direction and the same physical pose. A joint that can sit at one can sit at the
other, because they are the same place.

So the obvious limit check is wrong:

```cpp
if (angle < limit.lo || angle > limit.hi) reject();   // wrong for angles
```

against a shoulder limited to `[-0.6, 2.2]` this refuses `-6.10`, a pose the arm
may be sitting in as you read the rejection.

The repair is to bring the angle to its equivalent inside the range before
comparing, which is one `fmod` and one conditional. The interesting part is what
the same mistake does to a clamp. Six directions, all of them outside that
shoulder's range except the last:

| wanted | as number | as angle | number off | angle off |
|---|---|---|---|---|
| -3.000 | -0.600 | 2.200 | 2.400 | 1.083 |
| -2.600 | -0.600 | 2.200 | 2.000 | 1.483 |
| -2.200 | -0.600 | -0.600 | 1.600 | 1.600 |
| 2.800 | 2.200 | 2.200 | 0.600 | 0.600 |
| 3.100 | 2.200 | 2.200 | 0.900 | 0.900 |
| -6.100 | -0.600 | 0.183 | 0.783 | 0.000 |

Clamping the number picked the further end three times out of six. Going the
other way round the circle is often shorter, and a comparison of magnitudes
cannot see that.

Note that the number clamp is not a bad function. It is the right function for
a voltage, a duty cycle, a velocity, a setpoint in metres. This curriculum keeps
both, under names that say which is which, because the bug is never the clamp
itself, it is reaching for it on a quantity that wraps.

### What the limits actually take

Now the measurement. A 0.5 m and 0.4 m arm, shoulder held to `[-0.6, 2.2]`,
elbow to `[-2.4, 2.4]`, every cell of a 1 cm grid tested against both branches:

| | cells | share |
|---|---|---|
| reachable by geometry alone | 25140 | |
| legal within the joint limits | 13878 | 55.2% |
| both elbow branches legal | 5592 | 22.2% |
| elbow-up branch only | 4142 | 16.5% |
| elbow-down branch only | 4144 | 16.5% |

Nearly half the workspace is gone, which is unsurprising. The two "only" rows
are the finding. They are almost exactly equal, and together they are more than
half the legal area. There is no side of the workspace that belongs to one elbow
and no side that belongs to the other. The regions interleave.

Which means a solver that computes elbow-up and returns it finds
`5592 + 4142 = 9734` of the 13878 legal cells. **70.1 percent.** Almost a third
of the machine's usable area refused by code that had already computed the
answer and did not look at it.

This is the same conclusion lesson 13-02 reached, from a different direction.
There the argument was that choosing between the elbows is a decision about the
world, which table to avoid, which way the operator is standing, and so belongs
to the caller. Here it is narrower and harder to argue with: choosing early
throws away a third of the arm.

### Clamping is not a repair

The third mistake is the expensive one, and it is what people write when they
have understood the first two and want the problem to go away.

The solver returns a pose. The limits refuse it. So clamp both joint angles into
range and send that. It always produces a legal pose, it never fails, and every
move reports success.

Four targets where the first branch tried was the illegal one:

| target | clamped error | other branch |
|---|---|---|
| 0.574, -0.483 | 0.4540 | 0.0000 |
| 0.466, -0.587 | 0.5949 | 0.0000 |
| 0.340, -0.668 | 0.7294 | 0.0000 |
| 0.201, -0.723 | 0.1384 | no branch left |

Up to 0.73 m of error on an arm with a 0.9 m reach, and on three of the four the
target was reachable the whole time.

The reason is that the two joint angles are not independent. Move the shoulder
0.6 rad and the tool sweeps along an arc. Nothing recomputed the elbow, so the
clamped pose answers a question nobody asked: where does the tool end up if the
shoulder stops here and the elbow does what the old solution said.

The fourth row is the one target the arm genuinely cannot reach, and it is the
only one where "how close can we get" was worth asking. The clamp gave the same
kind of answer to all four and distinguished none of them.

### The nearest pose it can hold

For that fourth target there is a real question with a real answer: of every
pose the arm can hold, which one puts the tool closest.

Written directly that is a search over two joints, and a 601 by 601 grid is
2889608 evaluations to get an answer good to about a millimetre. It does not
have to be a search over two joints.

Fix the elbow. The tool now sits at a fixed radius from the base, and the
shoulder only turns that radius to a bearing. So the best shoulder angle for
that elbow is the target's bearing minus the arm's own offset:

```
offset   = atan2(l2 sin(q2), l1 + l2 cos(q2))
shoulder = clamp_angle(limits.shoulder, atan2(y, x) - offset)
```

which is exact, not searched. One variable is left to scan. On eight targets,
against that 601 by 601 grid:

| | evaluations | worst it trailed the grid |
|---|---|---|
| grid over both joints | 2889608 | |
| scan over the elbow | 16008 | 0.00e+00 m |

Never worse, using 180 times fewer evaluations, because solving the shoulder
exactly beats snapping it to a grid line. And the clamp in that snippet has to
be the angle clamp. The first version of this scan used a number clamp and lost
to the grid by 0.159 m on a target behind the shoulder stop, which no amount of
extra scan resolution fixed, because the error was not resolution.

The step count buys accuracy and nothing else:

| steps | worst gap |
|---|---|
| 8 | 4.23e-02 m |
| 32 | 5.02e-03 m |
| 128 | 3.09e-03 m |
| 512 | 2.28e-06 m |
| 2048 | 0.00e+00 m |

Pick it from the tolerance you need. It is a number to choose, not a constant to
inherit from whoever wrote the function.

## Build It

Implement `holds`, `clamp_angle`, `legal_solutions` and `nearest_reachable` in
`exercise/solution.hpp`.

```
rcpp verify 13-05
```

The suite clamps six directions both ways and compares them as angles, sweeps a
1 cm grid over the whole workspace counting both branches, clamps a refused
branch on four targets and measures where the tool lands, and checks the elbow
scan against a grid over both joints.

One thing that suite learned the hard way: it also recomputes the distance from
the joint angles the solver returned. A stub that reports distance 0 without
moving passed four of these checks until it did.

## Use It

**Return both solutions with a verdict on each.** Not one solution, and not a
boolean. `LegalSolutions` carries both poses and whether each is holdable, and
the caller decides.

**Keep unreachable and illegal apart.** A point beyond the links needs a
different target. A point the limits refuse needs a different elbow. One boolean
covering both is what makes a refusal impossible to act on.

**Map your machine's reachable set once, before anything is mounted around it.**
The table above is a grid scan and a few seconds. The answer for a real arm is
frequently not the disc everyone assumes, and it is much cheaper to learn now.

**If you clamp, say so.** A caller that is not told the tool is somewhere other
than where it was sent is about to close a gripper, place a part, or start the
next move from a pose it believes it knows.

## What Breaks First

- **Joint angles clamped into range, and the move reported as done.** See `E-KIN-0008`.
- **An angle clamped as though it were a number.** See `E-KIN-0009`.
- **One branch solved, and a third of the workspace lost.** See `E-KIN-0010`.

## Ship It

`Limit`, `holds`, `clamp_angle`, `legal_solutions` and `nearest_reachable` join
`rc::kin` beside the chain, the closed form solver and the singularity measures.
The arm can now be asked whether it is able to do what it was told, and answer
with a distance rather than a guess.
