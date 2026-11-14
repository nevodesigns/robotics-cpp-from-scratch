# Tipping With the Arm Out: The Centre of Mass of Base, Arm and Load, and How Hard the Base May Brake or Turn

> Every kilogram on the end of the arm moves the centre of mass towards the edge of the wheels.

**Type:** Build
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 33-04, 19-09, 29-01

## The Problem

19-09 asked how hard a vehicle may turn or brake before it tips: g times the distance from the centre of mass to the edge of the wheels, over the height of the centre of mass. For a plain vehicle the centre of mass hardly moves. Put an arm on it and it moves with the arm: a few kilograms at the end of an arm half a metre out shift it, and a load in the hand shifts it more, and the arm's pose changes from moment to moment. The limits have to be worked out for the pose.

## The Concept

### The centre of mass of the whole

The combined centre of mass is the mean, weighted by mass, of the base's centre, each link (taken at the middle of the link, between its frame and the one before) and the payload (at the tool). The base is 20 kg here and the arm's links 9.3 kg together (typical masses). With the arm tucked in the centre was -0.006 m forward of the base's centre at a height of 0.273 m; with the arm reaching 0.65 m forward it was 0.069 m forward and 0.255 m high. See `E-MOB-0013`.

### The limits that follow

With the centre in the base's frame, the base tips forward when braking harder than g (half the wheelbase less the centre's forward offset) over the height, backward when accelerating harder than g (half the wheelbase plus the offset) over the height, and sideways by the same rule with the half track. With the arm tucked the base could brake at 11.01 m/s2; with the arm reaching, 8.88 m/s2; and with 5 kg in the hand 5.19 m/s2. A centre of mass over the middle of the wheels gave exactly 19-09's formula, g times half the track over the height (the test checks it). See `E-MOB-0014`.

### The wheels may not be what limits it

On a dry floor (a friction of 0.6, typical) the wheels let the base brake at 5.89 m/s2 (23-05). The tucked base and the one reaching with no load could brake that hard without tipping, but with the load tipping came first: 5.19 m/s2. So the limit that applies depends on the pose and the load, and a braking limit chosen for the tucked arm is unsafe for the loaded one. From 1.0 m/s the loaded base needed 0.096 m to stop at that limit.

### Reaching sideways

The base is narrower than it is long here (a half track of 0.22 m), and an arm reaching to one side (with 2 kg in the hand) cut the limit on turning towards that side to 4.22 m/s2, from 7.92 m/s2 tucked. A base that turns with the arm out to the side is the one that falls over. See `E-MOB-0015`.

## Build It

`com_world` in `exercise/solution.hpp`.

Open `exercise/solution.hpp`. The part you write is marked `TODO`; the rest of the file is already written, and it is worth reading before you start, because your part is called by it or calls it.

What to write, as the note beside the `TODO` puts it:

- The mean, weighted by mass, of the base's centre (in the world), the middle of each link (between the previous frame, the arm's base for the first, and its own) and the tool for the payload.

As shipped, all 3 of the 3 tests fail. They are, in the order the suite runs them:

- With no arm mass and no load the centre of mass is the base's, and masses pull it by their moments.
- Reaching and carrying take the centre of mass towards the edge and cut the braking limit.
- Reaching to one side cuts the limit on turning towards it, and a centred mass gives the 19-09 value.

```
rcpp verify 33-05
```

A failing check names the test and prints the number it expected beside the one it got, so start from the first failure and work down. When the suite is green, read `reference/solution.hpp` beside your own version: where the two differ and both pass, the difference is worth understanding.

## Use It

`rc/mobile/tip_margin.hpp`.

```cpp
double com[3];  rc::mobile::com_world(m, base, q, masses, com);
const rc::mobile::TipLimits t = rc::mobile::tip_limits(base, com, half_wheelbase, half_track);
```

**What this does not do.** The wheels are taken as a rectangle and the ground as flat and level (19-09's slope is not added). Each link is a point mass at its middle (no rotation inertia, no real mass distribution). The limits are for steady accelerations, and the swing of the arm while the base brakes (it moves the centre of mass as the arm moves) and vibration are not counted. The payload's mass is known. The masses and the wheelbase are typical. There is no margin of safety built in: the limits are where it tips, and a real controller keeps a fraction of them (19-09's safety factor).

## What Breaks First

- **A limit worked out for the arm tucked.** See `E-MOB-0013`.
- **A load that was not counted.** See `E-MOB-0014`.
- **A turn with the arm out to the side.** See `E-MOB-0015`.

## Ship It

`rc/mobile/tip_margin.hpp` holds `MassModel`, `TipLimits`, `com_world`, `tip_limits` and `stopping_distance`.
