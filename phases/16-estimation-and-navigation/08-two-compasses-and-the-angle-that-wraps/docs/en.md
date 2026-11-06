# Two Compasses and the Angle That Wraps: Fusing a Gyro and a Compass

> An angle is a direction on a circle, and 3.1 and -3.1 are 0.083 apart, not 6.2. Every
> subtraction and every average of angles has to know that, and a filter that does not
> turns the whole way round.

**Type:** Build
**Time:** about 150 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 16-02

## The Problem

Lesson 16-02 combined two estimates of a distance into one better than either. A robot's
heading is the same problem with two differences. The sensors fail in opposite ways: a gyro
is smooth and drifts, and a compass is noisy and does not. And the quantity is an angle,
which is not a number on a line.

That second difference is where a filter that works for a distance fails. A distance of 3.1
and one of -3.1 are 6.2 apart. A heading of 3.1 radians and one of -3.1 are pointing almost the
same way.

## The Concept

### An angle is a direction

3.2 and -3.08 radians are the same direction. So the first job is to bring an angle into one
turn, and the simple ways are wrong in different ways. A loop that subtracts a turn at a time
never finishes for a big angle. `atan2(sin(a), cos(a))` loses digits as the angle grows.

`wrap_pi` subtracts the whole number of turns at once, from the floor of `(angle + pi) / 2pi`.
It is exact for an angle of any size, forty turns round or a thousand, and it finishes in one
step. The range is (-pi, pi], so that -pi and pi are one answer and not two, and the test
checks that every angle comes out in range pointing the same way it went in.

### The difference is the short way round

How far to turn to get from `b` to `a`:

| | |
|---|---|
| 3.1 minus -3.1, subtracted | 6.200 |
| `angle_difference(3.1, -3.1)`, the short way round | -0.083 |

A controller or a filter that uses the 6.2 turns the whole way round to get 0.083. This is the
same defect lesson 13-05 found in joint limits and lesson 08-12 in a wrapping timestamp: a
circular quantity treated as a linear one.

### The average is the direction between

The arithmetic mean of 3.1 and -3.1 is zero, which points the opposite way from both of them.
Angles are averaged as the directions they are: add the unit vectors, each times its weight, and
take the direction of the sum. For 3.1 and -3.1 that is 3.142, straight back.

There is a case a real averager has to admit. Opposite directions, 0 and pi, cancel, and the
sum has length zero, and `atan2` of zero and zero is not an answer. `circular_mean` reports that
there is no mean direction and returns a flag, and does not return a zero that looks like a
heading. See `E-NAV-0021`.

### The filter

A gyro measures how fast the robot is turning, so its error accumulates: a bias of 0.02 rad/s
becomes 2.4 radians in two minutes. A compass measures the heading directly, with scatter. Each
covers the other's fault. The gyro carries the estimate between compass readings, and each
reading pulls it a fraction of the way towards the truth, which is the **gain**.

The estimate advances by the gyro and is kept in range, so it does not grow without limit as
the robot goes round. A correction pulls it by `gain * angle_difference(compass, estimate)`.

### What the plain difference costs

A robot turning at 0.2 rad/s for two minutes, nearly four turns, so that the heading crosses
the back of the circle several times. The gyro reads 0.02 rad/s high. The compass has 0.15 rad
of noise and is read at 5 Hz. The error is the short way round.

| gain | plain difference rms | worst | short way round rms | worst |
|---|---|---|---|---|
| 0.005 | 1.264 | 2.174 | 0.609 | 0.759 |
| 0.020 | 0.518 | 1.346 | 0.189 | 0.230 |
| 0.100 | 0.709 | 3.139 | 0.048 | 0.162 |
| 0.500 | 0.455 | 3.081 | 0.091 | 0.302 |

The gyro alone, with no compass, has an rms error of 1.461 and a worst of 2.416.

At gains of 0.1 and above the plain difference is wrong by 3.1 radians at worst: half a turn,
pointing straight away. And at a small gain it is no better than the gyro it was meant to
correct. The short way round at a gain of 0.1 is 30 times better than the gyro alone. See
`E-NAV-0020`.

### The gain is a trade

Read down the short way round column: 0.609, 0.189, 0.048, 0.091. The error falls as the gain
rises, while the gyro's bias is the problem, bottoms out, and rises again when the compass's noise
is. A gain of 0.5 is twice as bad as the best.

The best gain is a property of the two sensors, not of the filter: their errors and the rates
they are read at. On a robot with a different gyro it is somewhere else. See `E-NAV-0022`.

## Build It

`wrap_pi`, `angle_difference`, `circular_mean` and the two methods of `HeadingFilter` in
`exercise/solution.hpp`. The comments give the formulas.

```
rcpp verify 16-08
```

The first four tests are the arithmetic. The last two are the tables above, with a baseline
filter that uses the plain difference written into the test, so that the damage it does is
measured and not asserted.

## Use It

`rc/nav/heading.hpp` joins `rc::nav`.

```cpp
rc::nav::HeadingFilter heading(0.1);
// every gyro sample
heading.predict(gyro_rate, dt);
// every compass reading
heading.correct(compass_angle);
```

**What this does not do.** The gain is fixed, and the gyro's bias is assumed constant and is
fought by the compass for ever. A better filter estimates the bias as part of its state, which
removes the first half of the trade, and is the next step: it is what a Kalman filter on an inertial
unit does. A compass is also wrong in a way that is not noise: a magnetised object near it, or the
robot's own motors, shifts it by a different amount at each heading, which this lesson does not
model and which no gain removes. And the heading here is one angle in a plane: three dimensional
orientation is a rotation and needs the quaternions of phase 06.

## What Breaks First

- **Subtracting angles.** The filter turns the long way round at the back of the circle. See
  `E-NAV-0020`.
- **Averaging angles as numbers.** Two headings that agree average to the opposite. See
  `E-NAV-0021`.
- **A gain chosen from one side.** It is a trade, with an interior best. See `E-NAV-0022`.

## Ship It

`rc/nav/heading.hpp` holds `wrap_pi`, `angle_difference`, `circular_mean` and `HeadingFilter`.

The robot of lesson 16-07 plans, drives, fits and replans. This is a better answer to the first
of those questions it has to ask, which way it is facing.
