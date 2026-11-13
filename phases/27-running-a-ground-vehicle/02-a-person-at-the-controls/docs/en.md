# A Person at the Controls: The Speed They Can Drive at When They See the Past, the Dead Man's Timeout, and a Vehicle That Slows Itself for the Wall

> A person driving over a link is driving a picture of where the vehicle was; the vehicle has to stand for the difference.

**Type:** Build
**Time:** about 90 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 27-01, 21-06, 19-04

## The Problem

Lesson 21-06 drove a robot from a chair and asked what the stop path should do when the link goes. This lesson is about the **person**: what they can be allowed to do when what they see is the past, what the vehicle does when they go quiet, and what it does when they ask for something the vehicle can see is wrong.

## The Concept

### The operator sees the past

What the person sees is a picture that is already old when it arrives (the **video delay**); they take time to react; their command takes time to get back (the **command delay**); and only then does the vehicle start to brake. The vehicle goes on at its speed for the whole of that, and then needs its braking distance. For a speed `v` that stops within a sight line `d`, with a total delay `T` and a deceleration `a`:

```
v T + v squared / (2 a) = d      so       v = a ( sqrt(T squared + 2 d / a) - T )
```

For a sight line of 6 m and a deceleration of 2 m/s2, the speed a person could be allowed was **4.00 m/s** with 0.5 s of delay in all, **3.29 m/s** with 1.0 s and **2.32 m/s** with 2.0 s. Stepping a vehicle at those speeds, with the delay and then the brakes, it stopped at the sight line to 2 cm. See `E-VEH-0115`.

### The dead man's timeout

When the commands **stop arriving** (the link has gone, the person has let go) the vehicle must stop. The governor treats a request older than the timeout as zero, and then ramps down at the braking limit. After the commands stopped, a vehicle at 2 m/s went on **1.99 m** with a timeout of 0.5 s and **4.99 m** with 2.0 s: `v times timeout`, plus `v squared over 2a`. The timeout is part of the stopping distance. See `E-VEH-0116`.

### A request is not a speed

A request for 2 m/s took **1.33 s** to be reached, because the governor ramps at the acceleration limit: a step in the request is not a step in the wheels (a person at a stick moves fast, and a vehicle that jumps with the stick throws its load, and spins its wheels).

### The vehicle slows itself for the wall

The vehicle has its own sensor, and the person has the delay. The governor caps the speed so that the vehicle could stop within the distance to the nearest thing it senses, minus a margin of 0.5 m, whatever is asked. With an obstacle 20 m ahead and the operator holding full speed, the governed vehicle stopped **0.50 m** short of it. With no cap on the obstacle, the same operator drove **58.7 m** past the place of it. This is **shared control**: the person steers and chooses, the vehicle says no to what it can see is wrong. See `E-VEH-0117`.

## Build It

`TeleopGovernor::step` in `exercise/solution.hpp`.

```
rcpp verify 27-02
```

## Use It

`rc/drive/teleop_governor.hpp`.

```cpp
rc::drive::TeleopGovernor gov(limits);
gov.request(now, operator_speed);                 // when a packet arrives
const double v = gov.step(now, dt, nearest_obstacle_ahead);
```

**What this does not do.** It controls the speed only: the steering is the person's, and a vehicle that is steered into a wall at a speed it can stop from is stopped, but a vehicle steered along a drop is not (a cliff sensor is the vehicle's own: 24-06). The obstacle distance is a single number straight ahead; the stopping distance in a turn, or on a slope or wet ground (19-04), is not worked out here. The delays are numbers you give it, not measured, and a link whose delay varies needs the worst case (21-06 shows what the stop path does with loss). A person who knows the vehicle is capped may stop looking.

## What Breaks First

- **A speed limit that forgets the delay.** See `E-VEH-0115`.
- **A vehicle that goes on when the link goes.** See `E-VEH-0116`.
- **A vehicle that does as it is told.** See `E-VEH-0117`.

## Ship It

`rc/drive/teleop_governor.hpp` holds `stop_speed`, `operator_speed_limit`, `TeleopLimits` and `TeleopGovernor`.
