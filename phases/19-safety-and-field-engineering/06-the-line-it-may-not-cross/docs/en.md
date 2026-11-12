# The Line It May Not Cross: A Geofence, the Point in Polygon Bug at a Vertex, and a Buffer That Grows With the Drift

> A fence is a promise that the robot will be stopped before it crosses the line, not after.

**Type:** Build
**Time:** about 110 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 19-05, 19-04, 16-01, 03-03

## The Problem

G7 in lesson 19-01 was "leaves the area it is allowed in", scored 9. The sensor-based limiter of 19-05 only sees what is in front of the robot. A fence is the other kind of limit: a boundary on a map, and the robot's own position estimate against it. A field robot with a fence does not hit a road because it saw one. It never gets near the road because the map said not to.

A fence is a polygon and a position, and three things in it go wrong: the test for "inside" fails at a vertex, "on the line" is treated as allowed, and the position is treated as exact when it is a guess that gets worse the further the robot goes.

## The Concept

### Strictly inside

`inside` casts a ray from the point to the right and counts the edges it crosses; an odd count is inside. On a square, a point at (5, 5) is inside, (11, 5) and (-0.1, 5) are not, and an L-shaped area gives the right answers in its upright, in its foot, in the notch and at the reflex corner. A point **on an edge or a vertex** is **not inside**: the line is where the robot is not allowed to be. A NaN position and a "polygon" of two points are both not inside.

### The vertex that is counted twice

The first version everyone writes counts an edge when the point's height lies between its ends, inclusive. When the ray goes **through a vertex**, both edges that meet there are counted, and a crossing is counted twice. On the diamond (2,0), (4,2), (2,4), (0,2), the point (1, 2) is plainly inside and on the same height as the right corner at (4,2): the naive count said **outside**, and the half open rule said **inside**. The half open rule counts an edge only when **exactly one** of its ends is above the point, `(a.y > p.y) != (b.y > p.y)`, and so counts a vertex on the ray once. The bug appears only for points at exactly the height of a vertex, which in a test with random points is almost never, and in a field laid out on a grid is always. See `E-SAFE-0016`.

### The distance to the edge is to the segment

`distance_to_edge` is the smallest distance to any **segment**, not to the line through it. A point at (15, 0) is 5 m from the segment from (0,0) to (10,0), not 0, which is the distance to its line. In the L shape, at (4, 4) next to the reflex corner at (5, 5), the nearest edge was **1.414 m** away, the corner itself; the line of the edge x = 5 would say **1.000**. At (6, 6) in the notch, square on to the edge, it was 1.000. A distance to the line is too small in some places and, for a point beyond the end of a segment, can say nothing about where the segment is. See `E-SAFE-0017`.

### The buffer: stop before the line

A fence that stops the robot **at** the line stops it **over** the line, by its stopping distance. So the fence has a **stop buffer**: the robot is `outside` when it is not strictly inside, or nearer than the buffer. With a 20 m square, a stop buffer of 2 m and a warning of 1 m more:

| position | state |
|---|---|
| centre, 10 m from the edge | inside |
| 5 m from the edge | inside |
| 2.5 m from the edge | warning |
| 1.5 m from the edge | outside |
| on the edge | outside |
| outside the square | outside |
| position not a number | unknown |

`unknown` is a stop too. A position that is a NaN is not "somewhere inside".

### The position is a guess, and it gets worse

A robot that locates itself by dead reckoning (phases 16 and 17) is wrong by a fraction of the distance it has travelled since its last fix. `required_buffer` is the stopping distance plus that error. With a stopping distance of 1.2 m and a drift of 2 percent:

| metres since the fix | buffer |
|---|---|
| 0 | 1.20 m |
| 50 | 2.20 m |
| 100 | 3.20 m |
| 200 | 5.20 m |

In a corridor 6 m wide, 3 m from each side in the middle, the robot was **inside** after 10 m of dead reckoning and **outside** after 100 m: the buffer, 3.2 m, was bigger than the room. A robot that cannot trust its position has **no inside left**, and the right thing for the fence to do is to say so and stop, so that a fix can be taken. The fence is only as good as the localisation behind it. See `E-SAFE-0018`.

### A speed, as well as a state

`fence_speed` uses the distance to the edge in place of the sensor's range of 19-05, with the position error taken off it. With a deceleration of 2, a latency of 0.5 s, a margin of 0.3 m and an error of 0.5 m:

| distance from the edge | allowed speed |
|---|---|
| 20.0 m | 7.820 m/s |
| 10.0 m | 5.148 m/s |
| 4.0 m | 2.715 m/s |
| 2.0 m | 1.408 m/s |
| 1.0 m | 0.342 m/s |
| 0.8 m | 0.000 m/s |

It falls smoothly to zero at the margin, and is zero outside and for a NaN. The nearest edge is used whatever the heading, so a robot driving along the fence is limited as if it were driving at it: that is conservative, and a known cost.

## Build It

`distance_to_segment`, `distance_to_edge`, `inside`, `fence_state`, `required_buffer` and `fence_speed` in `exercise/solution.hpp`.

```
rcpp verify 19-06
```

## Use It

`rc/safety/geofence.hpp` joins `rc::safety`.

```cpp
const double buffer = rc::safety::required_buffer(stop_dist, 0.02, metres_since_fix);
if (rc::safety::fence_state(area, position, buffer, 1.0) != rc::safety::FenceState::inside) stop_or_slow();
const double vmax = rc::safety::fence_speed(area, position, position_error, decel, latency, 0.3);
```

**What this does not do.** It is two dimensional and flat, with a polygon with no holes: a no-go area inside the field is a second polygon the robot must be outside of, which is the same test turned over and is yours to add. Coordinates are metres on a local plane, and a GPS position in degrees has to be converted first (lesson 16-01 is where the robot's position comes from). The drift rate is a number to be measured for the real robot. **A fence in software on the robot's computer is not a physical barrier**, and where the boundary is a road, water or a drop, it needs a physical one as well.

## What Breaks First

- **A vertex on the ray.** See `E-SAFE-0016`.
- **A distance to the line and not the segment.** See `E-SAFE-0017`.
- **A buffer that stays the same as the position gets worse.** See `E-SAFE-0018`.

## Ship It

`rc/safety/geofence.hpp` holds `Point`, `distance_to_segment`, `distance_to_edge`, `inside`, `FenceState`, `fence_state`, `required_buffer` and `fence_speed`.
