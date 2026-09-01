# Where It Thinks It Is: Odometry, and Which Error Costs What

> Every robot in this curriculum has known exactly where it was, because the
> simulation told it. Real ones do not. They add up what the wheels did and
> hope.

**Type:** Build
**Time:** about 150 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 00-05, 01-03

## The Problem

A robot is switched on and told where it is. From then on the only thing it
knows is how far each wheel turned.

Adding that up is **dead reckoning**, and it has a property nothing else in this
curriculum has had: it has no way to be right. Every step adds to the estimate
and nothing ever subtracts, so every error that enters stays in for ever. The
pose is only as good as the last time somebody told it the truth.

That is not a defect to be fixed. It is what odometry is, and it is still the
thing you build everything else on, because it keeps working in a tunnel, under
a gantry, with the camera blinded, at whatever rate the encoders report.

What matters is knowing **how wrong it is by now**, and that turns out to depend
on something people do not expect.

## The Concept

### The estimate is three lines

```cpp
const double forward = (left + right) / 2.0;
const double turn = (right - left) / wheel_base_;

pose_.x += forward * std::cos(pose_.theta);
pose_.y += forward * std::sin(pose_.theta);
pose_.theta = wrap_angle(pose_.theta + turn);
```

Two details worth pausing on.

**The wheel distances are rim travel, not ground travel.** An encoder counts
wheel rotation and cannot tell the difference. That is exactly why slip is
invisible to it: the wheel turned, the encoder counted, and the robot did not
move.

**The wheel base is believed, not measured.** It comes off a drawing or a tape
measure, and it is the only parameter here that nothing checks.

### Which error costs what, and it depends on the path

This is the measurement the lesson exists for. The same robot, a hundred metres,
changing only what it drives:

| error | driving straight | with turns |
|---|---|---|
| wheel radius 1% out | 1.0000 m | 1.1235 m |
| **wheel base 1% out** | **0.0000 m** | **1.0580 m** |
| initial heading 1 degree out | 1.7453 m | 0.1115 m |
| wheel slip, 1% typical | 1.5107 m | 0.9051 m |

Read the second row twice. A wheel base that is one percent wrong costs
**nothing at all** driving straight. Not "a little": nothing, exactly, because
the base only ever divides the *difference* between the wheels, and driving
straight that difference is zero.

Drive the same hundred metres with turns in it and the same error costs over a
metre.

The third row is the mirror image. A heading that starts one degree out costs
1.75 metres over a straight hundred, which is just distance times the tangent of
one degree, and it costs almost nothing around a closed loop, because the offset
largely cancels on the way back.

So:

- A calibration that drives in a straight line measures the wheel radius
  perfectly and says **nothing whatsoever** about the wheel base.
- A robot tested on a straight track can be badly wrong the first time it is
  asked to turn, and nothing in the test could have told you.

Calibrate each parameter on the path that exposes it. Wheel radius from a long
straight run against a tape measure; wheel base from turning in place or driving
a square. Then test on a path that is not the calibration path, or the test only
confirms the fitting.

### Every step cuts the corner

The position is advanced along the heading held at the **start** of the step, so
the estimate draws a straight line where the robot drove an arc.

How much that costs depends only on how often you update. Measured on a quarter
circle of one metre radius:

| steps | step length | error |
|---|---|---|
| 8 | 0.1963 m | 0.138914 m |
| 32 | 0.0491 m | 0.034711 m |
| 128 | 0.0123 m | 0.008677 m |
| 1000 | 0.0016 m | 0.001110 m |

Halve the step, halve the error, and the error is roughly the length of one step.

That is the whole argument for integrating often rather than for a cleverer
formula. There is an exact arc formula and it is worth knowing, and at a
centimetre per update it buys you under a centimetre per quarter turn.

### Wrap the heading, and mean the sign

Two small things that are large when they go wrong.

An unwrapped heading grows without limit, so after a few minutes of turning it
compares wrongly against everything, in a way that depends on how long the robot
has been switched on. That is `E-CPP-0004`.

And the turn direction has to be pinned by a test. **The right wheel going
further is a turn to the left.** A sign error there is invisible to any test that
compares an estimate against a truth computed with the same code, because both
turn the wrong way together. It took a test asserting the direction outright to
catch it.

## Build It

Implement `Odometry::update` in `exercise/solution.hpp`.

```
rcpp verify 16-01
```

The suite checks the arithmetic, pins the turn direction, prints the two tables
above, and asserts the relationships in them: that a wheel base error is
invisible driving straight and costs over half a metre with turns, and that a
heading error is worst straight and largely cancels around a loop.

## Use It

Report `travelled()` next to the pose, always. It is the honest confidence
measure, and it is more useful than a time, because the error grows with
distance and a robot parked for an hour has drifted by nothing.

Everything else in this phase is a way of correcting this estimate from
something absolute. None of them replaces it: they all correct it, and between
corrections this is what the robot has.

## What Breaks First

- **An estimate trusted long after it stopped being true.** See `E-NAV-0001`.
- **A calibration error invisible on the path you tested.** See `E-NAV-0002`.
- **A heading that grows without limit.** See `E-CPP-0004`.

## Ship It

`Odometry` opens `rc::nav`. It is the last thing in this curriculum that a robot
believes without evidence, and the rest of the phase is about finding some.
