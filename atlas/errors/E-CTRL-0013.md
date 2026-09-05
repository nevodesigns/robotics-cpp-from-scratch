id: E-CTRL-0013
title: A three phase profile on a move too short for three phases
match: the three phase assumption on a move too short for three phases
match: a long move has three phases and a short one has two
platforms: linux, windows
teaches: 14-06-a-profile-the-machine-can-follow
---

## Symptom

Long moves are smooth and short ones bang. The machine handles a metre
beautifully and jerks on a ten centimetre nudge, and the shorter the move the
worse it is.

Every test passes: the moves all arrive in exactly the right place, and stop.

## Cause

A trapezoid has three phases only when there is room for three. Speeding up to
the top speed and slowing down again costs `top_speed^2 / acceleration`, and a
move shorter than that never reaches the top speed at all.

A planner that assumes three phases regardless computes a **negative** cruise
time and carries on. With a top speed of 0.5 m/s, an acceleration of 1 m/s^2 and
a move of 0.10 m:

```
  ramp 0.5000 s, cruise -0.3000 s, duration 0.7000 s
```

| t | naive velocity | correct velocity |
|---|---|---|
| 0.3000 | 0.300000 | 0.300000 |
| 0.4000 | 0.400000 | 0.232456 |
| 0.4999 | **0.499900** | 0.132556 |
| 0.5001 | **0.199900** | 0.132356 |
| 0.6000 | 0.100000 | 0.032456 |

The commanded velocity drops by 0.3 m/s between two adjacent instants: an
infinite acceleration, and precisely the thing a profile exists to prevent.

And it arrives at 0.100000 m with a velocity of exactly zero. **An endpoint test
passes.** The fault is entirely in the middle.

## Fix

Check whether the move fits, and plan a triangle when it does not.

```cpp
const double needed = top_speed * top_speed / acceleration;
if (distance >= needed) {
  peak = top_speed;
  ramp = top_speed / acceleration;
  cruise = (distance - needed) / top_speed;
} else {
  peak = std::sqrt(acceleration * distance);   // never reaches the top speed
  ramp = peak / acceleration;
  cruise = 0.0;
}
```

The boundary is exact rather than approximate: at `top_speed^2 / acceleration`
the cruise time is zero, just below it there is no cruise, just above it there
is.

**Test the middle, not only the ends.** The check that finds this is one loop:
sample the commanded velocity finely and assert that no two adjacent samples
differ by more than the profile's own acceleration times the interval between
them. Anything larger is an acceleration the profile never promised.

**And test a short move.** Every profile is correct on a long one. The move that
finds this is shorter than `top_speed^2 / acceleration`, which for most machines
is a few centimetres, and which is most of the moves a robot actually makes.
