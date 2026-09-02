# Two Guesses Are Better: Combining Estimates That Disagree

> The odometer says the robot is at 12.4 metres. The sensor says 12.9. Neither
> is right, and there is an answer better than both of them.

**Type:** Build
**Time:** about 150 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 16-01, 03-01

## The Problem

Lesson 16-01 ended with a robot that knows where it thinks it is and has no way
to be right. Everything that fixes that has the same shape: get a second opinion
from something that does not drift, and combine.

The naive combinations are both wrong. Taking the newer one throws away
everything the odometry knew between measurements. Averaging them treats a
precise estimate and a rough one as equals.

What you want is to lean toward whichever you have more reason to believe, and
that turns out to have an exact answer.

## The Concept

### An estimate is two numbers

```cpp
struct Estimate {
  double value = 0.0;
  double variance = 1.0;
};
```

The second number is the one people skip and it is the one that does the work.

It is the **variance**, the square of the typical error, not the error itself. A
sensor described as accurate to five centimetres has a standard deviation of
0.05 and a variance of 0.0025. Passing one where the other belongs is wrong by a
factor of twenty and nothing will tell you.

### Lean toward whichever you trust

```cpp
gain = prior.variance / (prior.variance + measurement.variance);
value = prior.value + gain * (measurement.value - prior.value);
```

The gain is the prior's **share of the total uncertainty**. Equal uncertainties
give a half, and the answer lands in the middle. Nine times as uncertain as the
measurement gives nine tenths, and the answer lands nearly on the measurement.

That is the whole of it. Everything larger, a full Kalman filter over many
variables, is that formula with matrices where the numbers are.

### The result is always more certain than either input

```cpp
variance = (1.0 - gain) * prior.variance;
```

Always. Not "usually", and not "if the second opinion is good".

Which is worth sitting with, because it has a practical consequence people find
surprising: **a bad sensor is worth having**, as long as you are honest about
how bad it is. Combining an estimate you trust to a centimetre with one you
trust to a kilometre leaves the value almost unchanged and the certainty very
slightly improved. It costs nothing and it never hurts.

What hurts is claiming the kilometre is a centimetre.

### Predicting has to add uncertainty

```cpp
void predict(double motion) {
  estimate_.value += motion;
  estimate_.variance += motion_variance_;   // this line
}
```

Every correction reduces the variance. If nothing ever increases it, the
variance falls toward zero, the gain falls with it, and the filter stops
listening. It has concluded it is certain, and no measurement can talk it out of
that.

The physical claim that line makes is that motion is imperfect, which lesson
16-01 measured. Leaving it out claims the odometer is exact.

### Does any of this actually help?

Measured, over two thousand steps, with a drifting odometer and a noisy absolute
sensor arriving every tenth step:

```text
dead reckoning alone   0.0579 m
the sensor alone       0.0976 m
the two combined       0.0237 m
```

Better than the better of them by a factor of two and a half, out of two inputs
that are each worse than the answer.

### What the filter believes matters as much as what it measures

Same data, same sensors. Only what the filter was told about them changes:

| what the filter believes | error |
|---|---|
| the truth | 0.0237 m |
| its odometry is ten times better than it is | 0.0484 m |
| its odometry is ten times worse than it is | 0.0530 m |
| **its sensor is ten times better than it is** | **0.0530 m** |

Two things to take from that.

Being wrong in **either** direction roughly doubles the error, and the filter is
still better than either input, which is why nothing looks broken. You lose most
of what fusion was worth and the system keeps working.

And the last two rows are the **same number**. That is not a coincidence: the
gain depends only on the *ratio* of the two variances, so scaling both changes
nothing. "My odometry is worse than I thought" and "my sensor is better than I
thought" are the same statement.

Which is a useful thing to know when tuning. There is one quantity to get right,
not two. Fix the sensor variance at its measured value and adjust the motion
variance alone; turning both knobs is how filter tuning becomes folklore.

## Build It

Implement `gain_toward`, `fuse` and `Filter1D::predict` in
`exercise/solution.hpp`.

```
rcpp verify 16-02
```

The suite checks the arithmetic on hand worked cases, requires the combination
to be more certain than both inputs across a grid of variances, requires a
filter given no measurements to become less sure without limit, and then runs
the comparison above and prints it.

## Use It

Measure the variances rather than guessing them. For a sensor: hold the robot
still, record a few hundred readings, take their variance. That is a minute of
work and it is the difference between the first row of that table and the
others.

For odometry, drive a known distance repeatedly and take the variance of the
error per metre, on a path that exposes the errors you have. Lesson 16-01 is
about which path that is.

This is one variable. A real robot filters a pose, which is three, and the
formula becomes the same formula with matrices. The ideas do not change: a state
and its uncertainty, a prediction that adds uncertainty, a correction that
removes it, and a gain that is the prior's share.

## What Breaks First

- **A filter that has stopped listening.** Its variance only ever falls. See
  `E-NAV-0003`.
- **A filter told the wrong thing about its own sensors.** It works, and gives
  away most of what it was worth. See `E-NAV-0004`.
- **A standard deviation where a variance belongs.** See `E-NUM-0003`.

## Ship It

`Filter1D` joins `rc::nav` beside the odometry it corrects. Between them the
robot has an estimate, an honest opinion of how good it is, and a way to improve
it whenever anything else is willing to say where it is.
