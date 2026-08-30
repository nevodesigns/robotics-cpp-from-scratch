id: E-MATH-0003
title: A quaternion rotation of twice the angle, or an orientation that will not compare equal
match: expected .*rotate.* to be
match: expected this to be true: same_rotation
platforms: linux, windows
teaches: 06-03-quaternions
---

## Symptom

Two shapes, both from quaternions and both common enough to be worth naming
together.

Every rotation comes out at twice or half the angle intended. A ninety degree
turn produces a hundred and eighty, or a forty five.

Or two orientations that are plainly the same compare as different, and the
disagreement appears only sometimes, usually after an object has turned past
some particular attitude.

## Cause

The first is the half angle. A quaternion stores cos of half the angle and the
axis scaled by sin of half the angle. Building one from the full angle doubles
every rotation, and reading one back as though the scalar part were cos of the
full angle halves it.

The second is the double cover. Negating all four numbers gives a different
quaternion describing exactly the same rotation, so comparing the numbers
directly reports a difference that does not exist. Interpolating between q and
minus q is worse: it takes the long way round the sphere when the two ends are
the same orientation.

## Fix

For the angle, halve it on the way in and double it on the way out:

```cpp
const double half = angle / 2.0;
return Quat{std::cos(half), axis.x * std::sin(half), ...};
```

For comparison, allow for the sign:

```cpp
bool same_rotation(const Quat& a, const Quat& b, double tolerance);
```

For interpolation, check the sign of the dot product first and negate one end
when it is negative. Every slerp implementation begins with exactly that, and it
is the reason it does.
