# Standing on Three Legs: The Support Polygon, How Near the Centre of Mass Is to Its Edge, and the Shift of the Body That a Crawl Needs

> A body that lifts a leg must first move its weight off it, and the polygon of the feet left on the ground says how far.

**Type:** Build
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 34-04, 34-03, 33-05

## The Problem

Phase 34 made a leg. A walking robot has four, and the simplest way to walk is one leg at a time: lift a leg, put it down further on, lift the next. While one leg is in the air the robot stands on three, and the first question of balance is the oldest: will it fall over? A robot that moves slowly enough is statically stable if its centre of mass, projected onto the ground, is inside the polygon of the feet that are down. This lesson measures how far inside.

## The Concept

### The support polygon and its margin

The feet on the ground, seen from above in the body's frame, make a convex polygon: a triangle with three feet down. The margin is the distance from the projected centre of mass to the nearest edge: positive inside, negative outside and zero on an edge. It is the room the body has before it starts to tip, the number 33-05's tipping limits were built from. The function takes the corners in either order. See `E-LEG-0024`.

### A centred body has no margin

A robot with its hips 0.19 m fore and aft and its feet 0.15 m out to each side, standing centred, lifts one leg and its centre of mass falls exactly on the diagonal between the two feet that are on the other side of the triangle: the margin with each of the four feet lifted in turn was 0.000 m. Zero is not safe: a push, a bump, the swing of the leg itself tips it. See `E-LEG-0025`.

### Shifting the body

So a crawl moves the body first: before a leg is lifted, the centre of mass is shifted over the middle of the three feet that will stay down. Over the middle of the triangle (its centroid) the margin was 0.078 m, for a shift of up to 8.1 cm. Shifted forward 5 cm with a hind foot lifted the margin was 0.031 m: toward the side that stays down is inside, and the same shift forward with a front foot lifted goes toward the foot that is leaving and is no help. Which way to shift depends on which leg is going up. See `E-LEG-0026`.

### What it costs

A crawl is slow: the body must shift, then lift, then place, then shift back, for each leg, with one foot up at a time so that three carry the weight. Three legs carry a third each and the torque per leg is higher than with four (34-02, 34-06). It is the gait for a rough or slippery floor, or a robot with weak motors, and it is the safe one: the next lesson looks at the trot, which is faster and has no polygon at all.

## Build It

`polygon_margin` in `exercise/solution.hpp`.

```
rcpp verify 35-01
```

## Use It

`rc/legged/static_gait.hpp`.

```cpp
double feet[4][2];  rc::legged::default_feet(0.19, 0.15, feet);
const double m = rc::legged::crawl_margin(feet, lifted_leg, com_xy);
if (m < 0.02) shift_the_body_first();
```

**What this does not do.** The body is taken as static: the margin says nothing about acceleration or the swing of the lifted leg (35-02, 35-06). The centre of mass is a given point (33-05 worked it out for an arm; for a quadruped it is near the middle of the body, and moves with the legs). The ground is flat, the feet do not slip, and the feet are where the model says (34-08). The shift is a number here and its motion (a smooth move of the body that takes time) is not worked out.

## What Breaks First

- **A margin measured for four feet.** See `E-LEG-0024`.
- **A leg lifted with the body centred.** See `E-LEG-0025`.
- **A shift toward the lifted leg.** See `E-LEG-0026`.

## Ship It

`rc/legged/static_gait.hpp` holds `polygon_margin`, `centroid`, `default_feet`, `support_triangle`, `crawl_margin` and `crawl_margin_shifted`.
