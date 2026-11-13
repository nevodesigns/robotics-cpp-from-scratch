# What the Arm Reaches From Here: The Ring Around the Arm, Where the Base May Stop, and How Much Room It Leaves

> From a given place the arm reaches a ring, and the width of the ring is how badly the base is allowed to park.

**Type:** Build
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 33-01, 28-08, 28-04

## The Problem

33-01 got a world point into the arm's frame. The next question is the one a planner asks first: **from where can the arm reach it?** A fixed arm has one answer (a volume, 28-08). A mobile manipulator has a **family** of answers, one for each place the base can be, and the useful thing to know is the **region of base places that work**.

## The Concept

### The arm reaches a ring

Pointing the tool **down** at a point at some height in the arm's own frame, the arm can do it for points between an **inner radius** and an **outer radius** from its own vertical axis, and no others (the shoulder turns all the way round, so the direction does not matter). At a target **-0.10 m** below the arm's mount the reach ran from **0.00 m** to **0.72 m** from the arm's axis; at a target **0.20 m** lower, from **0.00 m** to **0.63 m**. There is an **edge** (the arm's length, shorter for a target that is far below, since the arm must bend down to it) and, for some heights, a **hole in the middle** (too close to the arm's own axis for the tool to be put straight down). The word ring is used even where the inner radius is 0.00. See `E-MOB-0004`.

### Finding the ring

`reach_annulus` steps outward along a line from the arm's axis and asks the **inverse kinematics** at each radius: the first radius reached is the inner one and the last the outer one. It is the same question as 28-08's, put along one line instead of through a volume, which is possible **because of the symmetry about the vertical axis**.

### A quick test for a base pose

With the ring known, whether a base pose reaches a target is **two comparisons**: the arm's axis is the base's place moved by the mount (33-01), and the target's **ground distance** from that axis must lie between the radii. The quick test agreed with the full inverse kinematics on **100 percent** of **576** base places (varied in place and heading). A planner that tries thousands of base places (33-03) uses the quick test and the full one for the final check. See `E-MOB-0005`.

### How badly the base may park

The ring has a **width**, and half of it is the **tolerance**: **36.0 cm**. A base that stops within that of the ideal middle still reaches. At a target **0.20 m** lower the tolerance was **31.5 cm**: the ring narrows as the target gets farther below the arm. The tolerance is what the base's **stopping accuracy** (25, 26) has to meet, and it is **larger than a base usually needs to be accurate to**, which is why a mobile manipulator can work at all. See `E-MOB-0006`.

## Build It

`reach_annulus` in `exercise/solution.hpp`.

```
rcpp verify 33-02
```

## Use It

`rc/mobile/base_reach.hpp`.

```cpp
const rc::mobile::Annulus ring = rc::mobile::reach_annulus(m, z_in_arm_frame, 0.01);
if (rc::mobile::annulus_can_reach(m, ring, base, target)) try_the_full_test();
```

**What this does not do.** The ring is for the tool **pointing down** at **one height**; a target on a vertical shelf needs the tool horizontal, which is a different ring (a different `tool_down_reachable`). The **whole path** to the point (the arm swinging past the shelf edge, 30) is not checked: only the end pose. The ring is taken **from the arm's own axis**, and its shape is that of **one typical arm**. Joint limits are the arm table's, and the shoulder turning all the way round is what makes the direction free.

## What Breaks First

- **Reach taken from the base's centre and not the arm's axis.** See `E-MOB-0004`.
- **A base pose tested only at the ideal place.** See `E-MOB-0006`.
- **A ring worked out at the wrong height.** See `E-MOB-0005`.

## Ship It

`rc/mobile/base_reach.hpp` holds `Annulus`, `arm_can_reach`, `reach_annulus`, `annulus_can_reach` and `park_tolerance`.
