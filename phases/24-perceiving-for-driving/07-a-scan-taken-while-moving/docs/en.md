# A Scan Taken While Moving: Every Beam Is Fired From a Different Place, and the Wall That Bends in the Point Cloud Is the Vehicle's Own Motion

> A scan is not a photograph. It is a sweep, and the sensor moves while it sweeps.

**Type:** Build
**Time:** about 80 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 24-01, 22-02, 23-01, 21-10

## The Problem

Lesson 24-01 treated a scan as if all its beams were fired at one instant from one place. A real lidar sweeps: it fires its beams one after another as a mirror turns, and a sweep of 360 beams takes tens of milliseconds, 0.1 s at 10 Hz. A vehicle that is moving or turning moves during that time, and beam by beam the sensor is somewhere else. A cloud put together as if it were not is a distorted one: the straight wall bends, the corner opens, and a vehicle that is turning sees a world that has been twisted by its own motion. This is the commonest cause of a map that smears when a robot turns, and it is cured by arithmetic.

## The Concept

### Where the sensor was when each beam fired

If the vehicle has a constant velocity during a sweep, its pose at any moment of it is on the way from its pose at the start to its pose at the end. `interpolate_pose` gives the pose at a fraction `u` of the way: the position linearly, the heading by the **shorter way round** (a sweep from 3.0 rad to -3.0 rad turns by 0.28 rad, not 6). Beam `k` of `N` is fired at `(k + 1/2) / N` of the sweep.

### Deskewing

`deskew` takes the scan and the pose of the sensor at the start and at the end, and for each valid beam puts the point in the world with the pose **its beam was fired from** and then into the frame of the start pose. That is the frame in which the cloud is meant to be seen: as the world looked from where the sweep began.

### What it fixes

A vehicle turning at 1 rad/s turns the sensor **5.7 degrees** across a sweep of 0.1 s. A straight wall 5 m ahead, in the cloud made as if the sensor had not moved, was **0.119 m rms** from straight; after `deskew` it was **0.0000 m**. A vehicle driving at the wall at 2 m/s moves 0.2 m during the sweep, and the cloud had the wall **0.101 m** off straight and, deskewed, **0.0000 m**. A sensor that did not move was not changed. The truth of the test is a wall cast from the sensor's actual pose at the time of each beam, so it is a check of the algebra, not of itself. See `E-VEH-0046`.

### How much it matters

The skew is the speed times the sweep time: at 5 m/s, **0.50 m**. A turn is worse, because it moves a point by the range times the turn rate times the sweep time, and the far points move most: a point 20 m away moves **2.00 m** for a turn of 1 rad/s. A vehicle that turns slowly at low speed does not need it, and one that turns quickly or goes fast cannot do without. See `E-VEH-0047`.

## Build It

`deskew` in `exercise/solution.hpp`.

```
rcpp verify 24-07
```

## Use It

`rc/perceive/deskew.hpp` opens nothing new in `rc::perceive`.

```cpp
const auto cloud = rc::perceive::deskew(scan, pose_at_sweep_start, pose_at_sweep_end);
```

**What this does not do.** It assumes a constant velocity during the sweep, and a vehicle that starts a turn in the middle of one has an error left over, smaller than the one removed. The two poses must come from something good enough to be trusted over 0.1 s, which is an inertial sensor or the wheel odometry, not a localiser at 1 Hz. It does nothing for the motion of other things: a person who walks during the sweep is distorted and stays so. And it needs the time of each beam, which some drivers give and some do not; one that does not is assumed to fire at an even rate.

## What Breaks First

- **A cloud made as if the sensor had not moved.** See `E-VEH-0046`.
- **A sweep time that nobody checked.** See `E-VEH-0047`.
- **Poses for the sweep from a source that is too slow.** See `E-VEH-0048`.

## Ship It

`rc/perceive/deskew.hpp` holds `interpolate_pose`, `deskew`, `skew_distance`, `skew_angle` and `far_displacement`.
