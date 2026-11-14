# Keeping the Body Level: A Moment Wanted, Vertical Forces on the Feet That Give It, and the Largest Push That Can Be Answered

> A body is held level by pushing harder on the feet on the side it is falling toward, and a foot can only push, never pull.

**Type:** Build
**Time:** about 110 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 35-04, 35-01, 34-02

## The Problem

35-04 says how the body is tilted. To keep it level the legs must do something about it: when the body rolls to the right, push harder on the feet on the right. A body standing on four feet has more feet than it needs to support its weight and give a roll and a pitch moment (that is three conditions), so the forces on the feet can be chosen, and the choice must obey one rule that the legs cannot break: a foot can only push, never pull.

## The Concept

### What is wanted: a moment

A PD law on the attitude gives the moment wanted: minus kp times the angle and minus kd times its rate, one for roll and one for pitch (the same structure as 29-02's joint). It is a virtual spring and damper on the body (a *virtual model*), and its gains are chosen from the body's inertia: here 100 N m per radian and 4 N m s per radian for a roll inertia of 0.05 kg m2.

### Forces on the feet that give it

For the feet on the ground, with their places (x, y) seen from above about the centre of mass, three conditions: the forces add to the weight (12 kg, 117.7 N), the roll moment is the sum of force times sideways place, and the pitch moment is minus the sum of force times forward place. With four feet there are more unknowns than equations, so the least norm solution is taken (the smallest sum of squares of forces): A transpose times the solution of A A transpose y = w. With three feet the equations have one solution. With no moment wanted each of four feet carried 29.4 N; a roll moment of 10 N m was given exactly with forces from 12.8 N to 46.1 N. See `E-LEG-0036`.

### A foot can only push

If the solution has a negative force the foot would have to pull the ground up. The largest roll moment that gives every force zero or more was 17.7 N m, which is the weight times the half width (17.7): it is the centre of pressure at the edge of the feet (35-06). Beyond it the function scales the moment down, keeping its direction, to that largest value and says false. (Cutting the negative forces to zero without scaling would leave forces that add to more than the weight, a push up that the body would feel and a moment that exceeds what the feet can give.) See `E-LEG-0037`.

### A push and the answer

With the feet's forces in the loop the body (inertia 0.05 kg m2 about the roll axis) was pushed with 10 N m for 0.3 s: the PD law gave the moment back through the feet, the peak roll was 5.7 degrees and the body was level again 0.12 s after the push ended. A push of 25 N m, more than the feet can answer, rolled the body over (1). The most that can be answered is set by the geometry of the feet, not by the gains: no controller can give a moment the feet cannot supply. See `E-LEG-0038`.

## Build It

`foot_forces_raw` in `exercise/solution.hpp`.

```
rcpp verify 35-05
```

## Use It

`rc/legged/balance_force.hpp`.

```cpp
const double want_roll = rc::legged::balance_moment(roll, roll_rate, 100.0, 4.0);
double f[4];  const bool full = rc::legged::foot_forces(feet, on_ground, weight, want_roll, want_pitch, f);
```

**What this does not do.** The forces are vertical only: the sideways forces at the feet (and friction limits, 23-05) are not worked out. The body is a rigid block with a given inertia, and the legs make exactly the forces asked at once (a real leg's torque comes through the joints with delay, 34-02 and 34-06). The least norm is one choice among many (a real controller may prefer to load the feet equally, or to minimise the joint torques or heat of 34-06). The feet's places are exact and the attitude is given (35-04). Only stance feet are used, two feet (a trot) cannot give three conditions, and the controller for a trot is therefore different (35-02). The numbers are typical of a 12 kg robot.

## What Breaks First

- **A moment asked of the feet that they cannot give.** See `E-LEG-0037`.
- **A weight not shared among the feet that are down.** See `E-LEG-0036`.
- **Gains chosen without the feet's limit in mind.** See `E-LEG-0038`.

## Ship It

`rc/legged/balance_force.hpp` holds `balance_moment`, `realized_moments`, `foot_forces_raw` and `foot_forces`.
