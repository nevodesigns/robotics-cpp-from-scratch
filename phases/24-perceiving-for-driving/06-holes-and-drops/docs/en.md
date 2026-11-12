# Holes and Drops: A Lidar Tipped Down Sees the Ground at a Known Range, So Ground That Is Not There Is a Hole, and How Fast It Can Be Driven at Depends on How Far Away the Ground Is Seen

> You cannot see a hole, only the ground that should be there and is not.

**Type:** Build
**Time:** about 85 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 24-02, 22-02, 22-03, 21-10

## The Problem

A lidar that sees obstacles sees what is **there**. A hole, a ditch or the edge of a platform is what is **not**: nothing to return a beam, so no point to mark. A vehicle that drives only by avoiding points drives into the first hole it meets, and falls into it, which is why this is its own lesson. The way to see a hole is to expect the ground. If the sensor knows where flat ground should be, ground that is not there is a hole.

## The Concept

### Where flat ground should be

A planar lidar tipped down by an angle `pitch` at a height `h` sends its beams in a plane that meets level ground along a **straight line across the front of the vehicle**. In the sensor's frame a beam at azimuth `az` is `(cos az, sin az, 0)`, so tipped down its downward part is `cos(az) * sin(pitch)`, and it meets the ground at a range of `h` over that. `expected_ground_range` is that, and infinity for a beam that does not point down. For a lidar 0.25 m up and tipped down 5 degrees, the beam straight ahead had a range of **2.868 m**, a beam 30 degrees to the side **3.312 m**, and both met the ground at the same distance ahead, **2.86 m**, which is `h / tan(pitch)`: the line is straight. Tipped down 10 degrees the ground was **1.42 m** ahead. How far ahead the ground is seen is a design choice, made by two numbers, and it sets everything that follows.

### Ground, hole, raised

`classify_beam` compares what a beam returned with that. Within a margin of the expected range it is **ground**. Longer, or **nothing at all**, is a **hole**: the ground is lower than it should be, or not there. Shorter is **raised**: something is standing on it, and is an obstacle for the layers of lesson 24-04. `nearest_hole_x` gives how far ahead the first hole beam is. The ground of the test was seen at the same distance in every direction, and a hole 0.3 m deep put the beams that fall in it at the expected `(h + depth) / down`. See `E-VEH-0043`.

### A hole can fall between two scans

The sensor sees the ground along **one line** at a time, and the vehicle moves it. A hole is seen only by the scans that happen to fall on it. At 1 m/s and a scan every 0.1 s, a hole 0.4 m long was seen by **4 scans**. At 5 m/s the vehicle drives **0.5 m** between scans, and a hole of 0.3 m can be missed by all of them: it was seen by **0** scans at one phase and **1** at another. `max_undetected_width` is the speed over the scan rate. Whether the vehicle sees a hole is a matter of luck for any hole narrower than that, and a fast vehicle with a slow sensor will drive into narrow ditches. See `E-VEH-0044`.

### How fast, if the ground is first seen there

The ground is first seen **2.86 m** ahead, so a hole can be first seen that far ahead, and the vehicle has to stop before it. During the latency it keeps going, then it brakes: `distance - margin = v * T + v*v / (2 a)`. With a braking of 1.5 m/s2, 0.2 s of latency and a margin of 0.3 m, the fastest the vehicle could go was **2.49 m/s**. More braking and less latency raise it; tipping the sensor down further, which brings the ground nearer, lowers it. A speed limit that does not know how far ahead the ground is seen is a guess. See `E-VEH-0045`.

## Build It

`expected_ground_range` in `exercise/solution.hpp`.

```
rcpp verify 24-06
```

## Use It

`rc/perceive/drops.hpp` opens nothing new in `rc::perceive`.

```cpp
const double x = rc::perceive::nearest_hole_x(azimuths, ranges, height, pitch, 0.1);
const double v = std::min(v_cruise, rc::perceive::max_safe_speed(x, 0.3, decel, latency));
```

**What this does not do.** It assumes the ground in front is level, and a slope changes where the line falls (and a vehicle that pitches as it speeds up moves the line by itself). The margin of 0.1 m is for noise of about that size. It uses one tilted scan; a 3D lidar sees the ground on many lines at once and does not have the gap between scans. It cannot tell a hole from a patch of black or wet ground that returns nothing, or from a step down that is shallow enough to drive off. And water that returns a ranging beam far away reads as a hole.

## What Breaks First

- **A robot that avoids only what it sees.** See `E-VEH-0043`.
- **A ditch narrower than the distance between scans.** See `E-VEH-0044`.
- **A speed limit with no idea how far the ground is seen.** See `E-VEH-0045`.

## Ship It

`rc/perceive/drops.hpp` holds `expected_ground_range`, `ground_x`, `Beam`, `classify_beam`, `nearest_hole_x`, `max_undetected_width` and `max_safe_speed`.
