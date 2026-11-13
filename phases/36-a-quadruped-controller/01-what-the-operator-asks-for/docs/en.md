# What the Operator Asks For: A Velocity Command, Clipped to What the Robot Can Do and Eased Toward by a Bounded Acceleration

> The command is a wish; the controller turns it into the nearest thing the legs can do, a little at a time.

**Type:** Build
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 35-09, 35-03, 34-04, 27-02

## The Problem

Phases 34 and 35 gave a quadruped legs, gaits and balance. Phase 36 turns them into a **robot that does what it is told**. What it is told is, for a legged robot as for a wheeled one (27-02), a **velocity**: **forward**, **sideways** and **turn**, from a stick or a planner. The command is a **wish**, and two things stand between it and the legs: what the robot **can** do, and **how fast** it may get there.

## The Concept

### Limits that are not a box

The **forward** and **sideways** speeds are limited **together**: a leg can step further forward than sideways, so the limit is an **ellipse**, **(vx / vx_max) squared plus (vy / vy_max) squared at most 1**, not a rectangle. A command outside it is **scaled back onto it, keeping its direction** (so a diagonal command stays diagonal and does not slide). With limits of **1.0 m/s** forward and **0.4** sideways, a command of **1.0 forward and 0.4 sideways** became **0.71 and 0.28**. The **turn rate** is capped. See `E-LEG-0051`.

### Eased, not jumped

The command in use moves toward the clipped one by **at most an acceleration times the time step**: a **jump** in the stick is a **ramp**. A jump from rest to **0.8 m/s** with an acceleration limit of **1.0 m/s2** took **0.80 s**, in both directions. Without it the body is told to be somewhere it is not and **the feet cannot follow** (34-05, 35-03): **the whole gait of 35 assumes smooth commands**. See `E-LEG-0052`.

### What the legs allow

The **step** is the **speed times the stance time**, and the step must be **within the leg's reach**. The **stance time** cannot be shorter than the **swing** the servos can do allows (34-05), so the fastest speed is **the longest step over the shortest stance**: **0.20 m over 0.125 s is 1.60 m/s**. A speed of **0.5 m/s** needed a step of **0.062 m**: within reach (**1**); **2.0 m/s** was not (**0**), however the stick is pushed. The **top speed of the velocity limit has to be set below this**, from the legs and not from a hope. See `E-LEG-0053`.

## Build It

`limit_command` in `exercise/solution.hpp`.

```
rcpp verify 36-01
```

## Use It

`rc/legged/velocity_cmd.hpp`.

```cpp
rc::legged::Cmd in_use = rc::legged::limit_command(from_stick, in_use, limits, dt);
```

**What this does not do.** The limits are **fixed numbers**: a real robot's depend on the **gait**, the **load** (33-06 for the wheeled one), the **slope** (35-08) and the **battery**. The ellipse is **one shape** and the acceleration limits are **separate for each axis**. **Stopping** is a command of zero and takes the time of the ramp (a **real stop**, an emergency, is the safety layer's, 36-06). **Where the command comes from** (a person, a path follower, 26) is not here, and the **noise** and **delay** of the stick are not modelled. The numbers are **typical** of a 12 kg robot.

## What Breaks First

- **A command not clipped to an ellipse.** See `E-LEG-0051`.
- **A jump in the command passed straight to the legs.** See `E-LEG-0052`.
- **A speed limit not set from the legs.** See `E-LEG-0053`.

## Ship It

`rc/legged/velocity_cmd.hpp` holds `Cmd`, `CmdLimits`, `limit_command`, `max_speed`, `Stride` and `gait_for_speed`.
