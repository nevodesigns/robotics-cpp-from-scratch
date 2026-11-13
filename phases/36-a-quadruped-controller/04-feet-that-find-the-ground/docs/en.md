# Feet That Find the Ground: Reaching Down Until Contact, a Ground Height Learned From Each Landing, and a Hole Told From a Step

> A foot cannot know the ground ahead, so it lands where it expected and, if the ground is not there, keeps looking, a little at a time.

**Type:** Build
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 36-03, 34-07, 34-05, 35-08

## The Problem

A walking robot plans where its foot should come down (34-04) and how (34-05), **assuming the ground is where it expected**. It often **is not**: a kerb, a step down, a stone, a hole. The robot **has no camera for the ground** in these lessons, so it cannot know ahead. What it has is the **contact sense of 34-07**: the foot **knows** when it is down. The strategy is old and works: **land where you expected and, if the ground is not there, keep looking**, a little at a time.

## The Concept

### Reach down until contact

After the swing the foot **goes on reaching down** at a bounded rate until the contact sense says it is down. On **level ground** the swing ends **1 cm above** the ground (the clearance 34-05 leaves) and the search took **0.010 m** and **0.04 s** to close it. A **bump** of **3 cm** is **met before the swing ends**: the foot touches early (34-07) and the search is **not needed** (**0.000 m**), but the swing must **stop**, or the foot would **push into the bump**. A **step down** of **5 cm** took an extension of **0.060 m** and **0.24 s** at **0.25 m/s**. See `E-LEG-0060`.

### A hole is not a step

The search must **stop** somewhere: the leg can only reach so far (**10 cm** below its planned place here). A drop of **15 cm**, **deeper than that**, was reported as a **hole** after **0.40 s**: the foot went as far as it may and **found nothing**. A hole means **do not put weight on this leg** (the other three carry the body, 35-01), **bring it back** and step **elsewhere**. A search with **no limit** would **drop the leg into the hole and the body after it**. See `E-LEG-0061`.

### Learning the ground

Each landing says **how high the ground is under that foot**. An estimate of the ground height under each foot moves toward the height at which it touched by a **gain** (here **0.2**): after **one landing** on a bump of **3 cm** it had moved **6.0 mm**, after **ten** **26.8 mm**. The **gain** keeps **one stone** from lifting the body, while **a real change** (a ramp, a step that lasts) is followed. The **body's height** is the **nominal height plus the mean** of the estimates of the feet that are **down**: with two feet on a 3 cm rise and two on the level, the body followed the mean by **15 mm**. See `E-LEG-0062`.

## Build It

`reach_down_step` in `exercise/solution.hpp`.

```
rcpp verify 36-04
```

## Use It

`rc/legged/terrain_adapt.hpp`.

```cpp
rc::legged::ReachState search;
rc::legged::reach_down_step(search, contact_sense, dt, 0.25, 0.10);
if (search.hole) lift_and_step_elsewhere();
```

**What this does not do.** The **ground is only known by touching it**: nothing looks ahead (a camera or a depth sensor would, and lets the robot **choose** the foot place, as stairs need). The search is **vertical** only, and the **contact sense is taken as exact** (34-07 has a delay of a few readings, during which the foot goes on down: a **small overshoot**). The **body's height and attitude** follow the feet's mean and the plane (34-03) but **the legs' own limits** (35-08) are not worked into it. **Slippery** ground, **soft** ground (sand, snow) where **contact is not firm**, and **moving** ground are not covered. The numbers are **typical**.

## What Breaks First

- **A swing that does not stop at an early contact.** See `E-LEG-0060`.
- **A search with no limit.** See `E-LEG-0061`.
- **A ground estimate that follows every stone.** See `E-LEG-0062`.

## Ship It

`rc/legged/terrain_adapt.hpp` holds `ReachState`, `reach_down_step`, `update_ground` and `body_height_from`.
