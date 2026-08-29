id: E-MATH-0001
title: Gimbal lock, two orientations that will not stay apart
match: expected this to be true: !same
match: gimbal
platforms: linux, windows
teaches: 06-02-rotations-and-their-limits
---

## Symptom

Near one particular attitude, usually pointing straight up or straight down, an
orientation held as roll, pitch and yaw behaves strangely. Two of the three
numbers swing wildly while the machine barely moves, a controller trying to hold
that attitude oscillates, and two descriptions that ought to differ turn out to
describe the same orientation.

## Cause

At pitch of ninety degrees the roll axis has been turned onto the yaw axis, so
rolling and yawing do the same thing and only their difference has any effect.
Three numbers have become two.

Measured with the rotation code in this lesson, adding 0.37 radians to both roll
and yaw at that pitch changes the largest matrix entry by 1.11e-16, which is
zero to the last bit a double holds.

This is not a mistake in the arithmetic and no choice of axis order avoids it. It
is a property of describing orientation with three numbers, and reordering the
axes only moves where it happens.

## Fix

Do not store or interpolate an orientation as three angles. Store a quaternion
or a rotation matrix, and convert to angles only at the boundary where a person
reads them.

Where angles are unavoidable, choose the convention so the singularity sits
somewhere the machine does not go. An arm that regularly points straight up
cannot use a convention whose singularity is straight up.

Rotation matrices have no singularity, and quaternions have none either, which
is the argument for both.
