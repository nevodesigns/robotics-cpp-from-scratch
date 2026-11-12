id: E-SAFE-0018
title: A fixed buffer around a position that keeps drifting
match: a position that drifts needs a bigger buffer, and when it is bigger than the area there is no inside left
match: the state is stop inside the buffer, a warning before it, and unknown when the position is not a number
match: the speed near the edge falls to zero at the margin, and is zero outside
platforms: linux, windows
teaches: 19-06-the-line-it-may-not-cross
---

## Symptom

The robot passes the fence acceptance test near the start of a run and leaves its area at the end of a long one.

## Cause

With a stopping distance of 1.2 m and 2 percent drift the buffer was 1.20 m at the fix, 2.20 m after 50 m, 3.20 m after 100 m and 5.20 m after 200 m. In a corridor 6 m wide the robot in the middle was inside after 10 m and outside after 100 m. A NaN position was unknown, and the speed at the edge fell to 0 at 0.8 m.

## Fix

Grow the buffer with the distance since the last fix, and stop when it is bigger than the room.

```cpp
const double buffer = required_buffer(stop, drift, metres_since_fix);
```
