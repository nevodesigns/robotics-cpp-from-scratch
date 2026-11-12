# A Speed the Sensor Can Vouch For: A Speed Limit from Range, and Readings That Are Not Evidence of an Empty Path

> A range is what the sensor said. It is not what is there.

**Type:** Build
**Time:** about 110 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a range sensor, simulated)
**Prerequisites:** 19-04, 19-02, 15-03, 01-06

## The Problem

Lesson 19-04 gives the speed a sight line allows. A distance sensor gives the sight line. Between the two is the most common way a robot that "has obstacle avoidance" runs into things: the sensor says something that is **not a distance**, and the code treats it as one.

A lidar or an ultrasonic sensor reports a number when it gets an echo, and when it does not, it reports something else: infinity, zero, a negative number, the maximum range, NaN, or the last value again. Each of those means "I do not know", and the number most often read out of them is "nothing is there".

## The Concept

### What a reading is evidence of

A `Reading` is a range and an age. `trusted_range` says how far the path is clear **as far as this reading can vouch for it**, and the rule is that **what it cannot vouch for is treated as blocked**:

| reading | trusted as |
|---|---|
| 2.0 m | 2.0 m |
| not a number | 0.0 m |
| 0.0 m | 0.0 m |
| 0.05 m, inside the blind zone | 0.0 m |
| -1.0 m | 0.0 m |
| no return (infinity) | 1.5 m |
| 50 m, beyond the sensor's reach | 1.5 m |
| 2.0 m but 0.5 s old | 0.0 m |
| age not a number | 0.0 m |

A reading closer than the blind zone is not "very close": the sensor cannot see there, and it is exactly where things you are about to hit are. A negative number or zero is an error code. An old reading is a picture of where the robot was. See `E-SAFE-0013`.

The two **no return** lines are not zero, and not the sensor's reach. A glossy or dark or steeply angled surface returns nothing, and so does an empty hall, and the sensor cannot tell them apart. So "nothing" is taken as `no_return_range`, **1.5 m** here, a distance chosen to be far less than the reach: the robot may go on, slowly, and cannot go fast on the evidence of silence. See `E-SAFE-0014`.

### The obvious code lets NaN through

The first way to write this is `std::min(top_speed, max_speed(range, ...))`. With a range that is not a number, `max_speed` returns a NaN, and `std::min(2.0, NaN)` returns **2.0**: a comparison with NaN is false, so the first argument wins. A front reading that was not a number, with a command of 2 m/s, gave **2.0 m/s** through that expression and **0** through the limiter. The same happens with infinity. This is not rare: a NaN is what a division by zero in a driver, an uninitialised field and a failed parse of a message all produce. See `E-SAFE-0014`.

### The direction of travel

A forward command is limited by the front range and a reverse command by the rear. With a front clear to 5.0 m and a rear at 0.5 m, a forward 2 m/s was allowed **2.0**, and a reverse 2 m/s was allowed **-0.3**: the sign is kept, the speed is not. A command of zero stays zero, a command that is not a number gives zero, and a slow command that is allowed passes unchanged. At the margin, a range of 0.3 m, the limit is **0**.

### At the wall

A robot driven at a wall 10 m away at a command of 2 m/s, a margin of 0.3 m, a deceleration of 2 and a latency of 0.5 s. The limit is recomputed every tick, and the robot follows it, braking no harder than its deceleration. With the limiter, the closest it came was **0.30 m**: it slowed to a stop at the margin. Without it, it hit the wall at **2.00 m/s**. The limiter does not "see" the wall coming: the limit is simply low where the room is short. See `E-SAFE-0015`.

### When the sensor goes quiet

A good reading of 4.0 m, 0.05 s old, allowed **2.00 m/s**. The same distance 0.25 s old allowed **0.00**, one that is not a number allowed **0.00**, and one exactly at the maximum age of 0.2 s was still fresh. A sensor that stops publishing (a cable, a driver that died, a full queue) must leave the robot unable to go fast, and it does so only because the **age is part of the reading**. It is the same idea as the heartbeat of lesson 19-02, applied to the sensor: silence is a signal. See `E-SAFE-0015`.

## Build It

`trusted_range` and `limit_speed` in `exercise/solution.hpp`. The configuration and `Reading` are given.

```
rcpp verify 19-05
```

## Use It

`rc/safety/limiter.hpp` joins `rc::safety`.

```cpp
rc::safety::LimiterConfig config;           // set decel, latency and margin from lesson 19-04's numbers
rc::safety::Reading front{front_range_m, now - front_stamp};
const double v = rc::safety::limit_speed(command, front, rear, config);
```

Put it between the planner and the stop path of lesson 19-02, in the robot's own loop.

**What this does not do.** It uses **one range per direction**, the nearest thing the sensor sees along its axis. A real vehicle needs the range along the path it will sweep, which is wider than the sensor's beam, so a lidar's scan must be reduced to the nearest point in a corridor first. It does not know if the thing it sees is a person, a wall or a leaf, and it does not predict a moving thing. A single sensor is a single point of failure, and **the way to cover that is a second, different kind of sensor** (a bumper that touches, a cliff sensor) and a cross-check, which comes later in this phase. And a limiter built from a range protects only against what the sensor can see: it does nothing for a drop, a glass door or a low bar.

## What Breaks First

- **A reading that is not evidence taken as one.** See `E-SAFE-0013`.
- **`std::min` with a NaN.** See `E-SAFE-0014`.
- **A limit with no age.** See `E-SAFE-0015`.

## Ship It

`rc/safety/limiter.hpp` holds `LimiterConfig`, `Reading`, `trusted_range` and `limit_speed`.
