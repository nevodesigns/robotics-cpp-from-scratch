id: E-MATH-0026
title: Random roll, pitch and yaw taken for a random rotation
match: random roll, pitch and yaw give the right average turn and the wrong spread of turns
match: expected this to be true: static_cast.*small_naive
platforms: linux, windows
teaches: 06-13-random-in-every-direction
---

## Symptom

A test of an orientation filter feeds it random starting orientations. It never finds the failure that a nearly aligned start causes, because nearly aligned starts almost never come up.

## Cause

Three angles chosen evenly are not an even choice of rotation. Rotations are spread differently from the angles that describe them, and the pitch near plus or minus 90 degrees is crowded in the same way the poles were.

Measured over 100000 rotations: the mean angle of the turn was **126.0** degrees for roll, pitch and yaw and 126.4 for an even choice, against the right 126.5: a test of the average alone would pass the bad draw. The share of turns under 30 degrees was **0.0045** against **0.0075** expected, 40 percent too few.

## Fix

Draw rotations evenly with Shoemake's method, three numbers in [0, 1) placing a point evenly on the four dimensional sphere of unit quaternions:

```cpp
return Quat{b * std::cos(kTwoPi * u3), a * std::sin(kTwoPi * u2), a * std::cos(kTwoPi * u2), b * std::sin(kTwoPi * u3)};
```

And test a sampler on the shape of a distribution, not only its mean.
