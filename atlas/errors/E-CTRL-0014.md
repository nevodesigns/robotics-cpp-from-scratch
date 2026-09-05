id: E-CTRL-0014
title: Slowing one axis of a move instead of the move
match: stretching the move without changing its shape
match: a profile of nothing, and a move backwards
platforms: linux, windows
teaches: 14-06-a-profile-the-machine-can-follow
---

## Symptom

A path that has to be slowed, because a joint is near its limit or an arm is
near a singularity, comes out the wrong shape. A straight line bows. A circle
becomes an egg. The tool arrives at the right place having gone somewhere else
on the way.

## Cause

One axis was limited and the others were not, so the parts of the motion stopped
keeping time with each other.

The shape of a path is the relationship between its axes at each instant. Scale
one of them and the relationship changes, which is a different path with the
same endpoints.

Lesson 13-04 is where this usually arrives from: near a singularity the joint
rate needed for a given tool speed grows without bound, so something has to give
way, and the tempting thing to give way is the direction that is expensive.

## Fix

Slow the whole profile in time.

```cpp
const rc::control::Trapezoid slower = profile.scaled_to(seconds);
```

Stretching time by a factor divides every velocity by it and every acceleration
by its square, and the shape is preserved exactly. Measured, a one metre profile
stretched to twice its duration is at the same position at every fraction of the
way through, with half the peak speed.

For a multi-axis move, compute the time each axis needs on its own, take the
longest, and stretch every axis to that. The slowest axis sets the pace and the
shape survives.

Three details.

**Refuse a duration shorter than the profile's own.** The machine cannot do it,
and returning something it cannot follow only moves the failure downstream, to
a place with less information about why.

**Handle a move of zero.** A profile over no distance has no duration and no
direction, and dividing by either is the usual way this crashes on the day a
target happens to equal the current position.

**Handle a move backwards.** It is the same profile with every sign turned
round, and writing it as a positive distance with a separate direction keeps the
arithmetic in one place. Getting this wrong produces a machine that only works
in one direction, which is a bug people find by driving into something.
