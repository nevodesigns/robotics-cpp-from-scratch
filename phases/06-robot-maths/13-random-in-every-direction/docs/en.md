# Random in Every Direction: Why Random Angles Are Not Random Directions or Random Rotations

> Testing a sensor model or a filter means throwing random orientations at it. The obvious way to make them gives
> too many near the poles and too few small turns, and the test cannot tell.

**Type:** Build
**Time:** about 150 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 00-12, 06-03

## The Problem

A test of an orientation filter wants a thousand random starting orientations. A Monte Carlo estimate of how much of the sky a sensor sees wants a million random directions. Lesson 00-12 gave an even random number between 0 and 1, so it seems that two or three of them, used as angles, are a random direction or a random rotation.

They are not, and the way they are not is quiet: the numbers look reasonable, their average is about right, and the distribution is wrong.

## The Concept

### A direction from a latitude and a longitude

Choose a latitude evenly between -90 and 90 degrees and a longitude evenly between 0 and 360. Every line of latitude is chosen equally often, however short, so the tiny circles near the poles get as many points as the equator.

Out of 100000 directions, within 10 degrees of the north pole:

| draw | share within 10 degrees | against the true share of 0.0076 |
|---|---|---|
| latitude and longitude | 0.0546 | 7.2 times too many |
| height chosen evenly | 0.0071 | right |

See `E-MATH-0025`.

### The fix is a fact of geometry

The area of a band of a sphere depends only on its height, not on where it is (Archimedes). So choose the height `z` evenly between -1 and 1 and the longitude evenly, and the sphere is covered evenly. Ten bands of height, 100000 directions: the fewest held 9798 and the most 10135, where even is 10000, and the average direction was (0.000, 0.001, 0.000).

### Rotations

A rotation from a roll, a pitch and a yaw each chosen evenly is not an even choice of rotation. A rotation has an axis and an angle, and for rotations chosen evenly the angle is not spread evenly: it has the density `(1 - cos t) / pi` on 0 to pi, a mean of `pi / 2 + 2 / pi`, which is 126.5 degrees, and a share of `(t - sin t) / pi` below `t`.

Over 100000 rotations:

| draw | mean turn | share turning less than 30 degrees |
|---|---|---|
| Shoemake's method | 126.4 | 0.0075 |
| roll, pitch and yaw | 126.0 | 0.0045 |
| expected | 126.5 | 0.0075 |

The average of the bad draw is within half a degree of the right one. A check on the average alone would pass it. The share of small turns is 40 percent too low. See `E-MATH-0026`.

### Shoemake's method

Three numbers evenly in [0, 1) become a unit quaternion that is even over rotations:

```
a = sqrt(1 - u1), b = sqrt(u1)
w = b cos(2 pi u3), x = a sin(2 pi u2), y = a cos(2 pi u2), z = b sin(2 pi u3)
```

The north pole turned by such a rotation fell within 10 degrees of itself 0.0073 of the time, where 0.0076 is the share of the sphere: an even rotation of a direction is an even direction.

### The angle of a rotation

It is 2 times the arc cosine of the scalar part, and the scalar part needs its size taken and held to at most 1: a quaternion and its negative are the same rotation, and rounding can leave the scalar part a hair over 1, for which the arc cosine is not a number. See `E-MATH-0027`.

## Build It

`random_direction`, `random_rotation` and `rotation_angle` in `exercise/solution.hpp`. The naive draws, kept for comparison, and `angle_between_degrees` are given.

```
rcpp verify 06-13
```

## Use It

`rc/math/random_rotation.hpp` joins `rc::math`.

```cpp
rc::core::Rng rng(seed);
const Vec3 dir = random_direction(rng);
const Quat q = random_rotation(rng);
```

**What this does not do.** It uses the plain generator of lesson 00-12, whose period and quality are enough for tests and not for a study that needs billions of draws. It draws rotations evenly, which is the right default and not always the right test: a filter's failures often live near a particular orientation, which a test should visit on purpose. It
does not draw from a distribution concentrated around a given orientation, which is what a noisy sensor would produce. And it tells you the shape of a distribution is right only if the test looks at the shape: the mean is not enough.

## What Breaks First

- **Random latitude and longitude.** The poles are crowded. See `E-MATH-0025`.
- **Random roll, pitch and yaw.** The turns are spread wrongly, and the mean hides it. See `E-MATH-0026`.
- **An arc cosine of a rounded number.** Not a number. See `E-MATH-0027`.

## Ship It

`rc/math/random_rotation.hpp` holds `random_direction_naive`, `random_direction`, `random_rotation_naive`, `random_rotation`, `rotation_angle` and `angle_between_degrees`.

Lesson 06-03 said what a quaternion is. This is how to draw one at random without a bias that no ordinary check would find.
