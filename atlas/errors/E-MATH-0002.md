id: E-MATH-0002
title: A rotation that has stopped being a rotation
match: expected this to be true: is_orthonormal
match: off unit length
platforms: linux, windows
teaches: 06-02-rotations-and-their-limits
---

## Symptom

Over hours of running, a simulation or a dead reckoning estimate slowly distorts.
Distances that should be preserved change, transformed shapes shear slightly, and
transposing the matrix no longer quite undoes it.

## Cause

Rotations composed repeatedly accumulate rounding. Each product is very slightly
off, and the errors do not cancel. The columns stop being exactly unit length and
stop being exactly perpendicular, so the matrix stops being a rotation and starts
scaling and shearing what it should only turn.

Measured in this lesson, a hundred thousand compositions leave the columns off
unit length by about 4e-12. That is small, growing, and one directional: it never
recovers on its own. A loop at a kilohertz reaches that count in under two
minutes.

## Fix

Renormalise periodically. Force the columns back to unit length and back to
perpendicular, using Gram Schmidt or by converting to a quaternion, normalising
that, and converting back.

Quaternions are cheaper to renormalise, needing one division by the norm rather
than an orthogonalisation, which is one of the practical reasons long running
attitude estimators hold their state as a quaternion.

Assert on the property rather than assuming it. is_orthonormal with a tight
tolerance, checked at a stage boundary, catches this while it is still small.
