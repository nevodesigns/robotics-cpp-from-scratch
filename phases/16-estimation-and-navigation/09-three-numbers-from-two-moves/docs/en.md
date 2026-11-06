# Three Numbers From Two Moves: Calibrating the Odometry You Believed

> Odometry believes three numbers nobody measured. Two moves, a straight drive and a spin,
> find all three, and a square that was 2.5 metres out closes to 2 centimetres.

**Type:** Build
**Time:** about 150 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 16-01, 15-02

## The Problem

Lesson 16-01 built odometry and measured what each of its errors costs, one at a time, and
found that the wheel base was the worst. It said how to *live with* the errors, by correcting
from outside now and then. It did not say how to remove them, and the largest part of them is
the same on every run.

A real robot has all of them at once. The tyres wear and inflate unevenly, and the contact
patch is not where the drawing puts it. A robot told to drive a square comes back somewhere else,
in the same direction every time, and no amount of averaging helps.

## The Concept

### What odometry believes

Three things, none of which it has measured. That the left wheel travels the distance its encoder
says. That the right wheel does. And that the two wheels are as far apart as the drawing says.

A robot told to drive a square four metres on a side, 16 metres in all, with the odometry believing
the drawing:

| the real robot | ends up from where it thinks | heading error |
|---|---|---|
| left wheel 1.2 % big | 132.2 cm | 25.3 deg |
| right wheel 0.6 % small | 75.2 cm | 14.8 deg |
| wheel base 2 % wider | 35.4 cm | 7.1 deg |
| both wheels 1 % big | 17.8 cm | 3.6 deg |
| all of them together | **249.9 cm** | **46.4 deg** |

Two wheels that are equally big do not veer, but they carry the robot one percent further on each
straight and turn it one percent further on each spin: four ninety degree turns, each 0.9 of a degree
long, are 3.6 degrees round by the end. Wheels that differ make every straight line a curve, and that is
what puts the left wheel at a quarter of a turn of heading error. See `E-NAV-0023`.

### Two moves, in order

There are three unknowns and one move does not tell them apart.

A **straight drive** gives two things. How far it went, measured from outside, is the average of the two
wheel scales. How much it veered is their difference over the wheelbase: the wheel that travels further turns
the robot away from itself.

A **spin on the spot**, the wheels travelling opposite ways, gives the third. The rotation depends on how far
apart the wheels are, and it is the only move that does.

They have to be found in order, because each needs the one before:

```text
mean scale   = arc / nominal distance                          the straight drive
wheelbase    = 2 * mean * spin travel / (rotation * base)      the spin, and the mean
difference   = veer * real wheelbase / nominal distance        the straight drive again
left, right  = mean -/+ difference / 2
```

What is measured on a straight drive is the chord of an arc, and the two differ by the sinc of half the veer.
It is about a fifth of a percent at these sizes and costs nothing to take out.

With exact measurements the three came back to within a part in a hundred thousand for five different robots, from a
left wheel 1.2 % big and a wheelbase 2 % wide to both wheels 5 % big. See `E-NAV-0024`.

### What the measurement limits

The robot cannot measure how far it went, since that is the thing it is wrong about. The moves are measured from
outside, with a tape and a protractor, or a camera.

Twenty calibrations of that robot, with a tape good to 3 mm on five metres and a protractor good to 0.2 degrees:

| scale | rms error | worst |
|---|---|---|
| left wheel | 0.0006 | 0.0015 |
| right wheel | 0.0006 | 0.0013 |
| wheel base | 0.0007 | 0.0016 |

Each is right to about a tenth of a percent. And the square:

| odometry believing | position error | heading error |
|---|---|---|
| the drawing | 249.9 cm | 46.4 deg |
| the calibration | **2.3 cm** | **0.4 deg** |

A hundred times better in position. What is left is the noise in the three numbers. Making the moves long makes a
fixed measurement error a smaller fraction: 3 mm on five metres is 0.06 percent, and a fifth of a degree spread over
four turns is a thousandth. See `E-NAV-0025`.

## Build It

`calibrate` and `corrected` in `exercise/solution.hpp`. The two test structures and the result are given. The
comments set out the three steps and the one thing that has to be guarded.

```
rcpp verify 16-09
```

The truth in the tests is a robot written into the test with its own scales and wheel base, moved in a thousand pieces so
that the way it is integrated is not what is being measured, and measured with noise.

## Use It

`rc/nav/calibrate.hpp` joins `rc::nav`, beside `odometry.hpp`.

```cpp
const rc::nav::WheelErrors e = rc::nav::calibrate(0.4, straight, spin);
rc::nav::Odometry odometry(0.4 * e.wheelbase_scale, start);
// every step, from the encoders
const rc::nav::WheelTravel t = rc::nav::corrected(e, left, right);
odometry.update(t.left, t.right);
```

**What this does not do.** It finds the errors that are the same every time. A wheel that slips on a wet floor is a
different robot each run, and a calibration of a dry one does not help; that is what correcting from outside is for.
It assumes the error is a scale and not an offset: a wheel with a flat spot, or an encoder with uneven slots, has a
travel that depends on where the wheel is in its turn. It is a fit at one load and one speed, and a robot that carries
a payload has a different contact patch. And it needs something outside the robot to measure against, which is a
tape measure on a bench and a camera or a motion capture system in a lab.

## What Breaks First

- **Odometry that believes the drawing.** A square does not close, by a lot, in a direction that does not change. See
  `E-NAV-0023`.
- **Three unknowns from one move.** The fit is perfect for the move it was made from. See `E-NAV-0024`.
- **A calibration trusted past its measurements.** It is as good as the tape and the protractor. See `E-NAV-0025`.

## Ship It

`rc/nav/calibrate.hpp` holds `WheelErrors`, `StraightTest`, `SpinTest`, `calibrate`, `WheelTravel` and `corrected`.

Lesson 16-08 gave the robot a better heading from a compass. This gives it a better one without any outside sensor at
all, because the errors that made the heading wrong were in the robot.
