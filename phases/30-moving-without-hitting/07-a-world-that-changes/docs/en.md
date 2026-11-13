# A World That Changes: A Planning Scene With a Version, Objects That Grow Less Certain as They Age, a Part in the Gripper, and Plans That Know They Are Stale

> A plan is a promise about the world as it was; the scene has to say when the world has stopped being that.

**Type:** Build
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 30-06, 30-04, 30-02

## The Problem

30-04 built a roadmap for a scene that stands still, and said what happens when it does not: the map lies. A cell where the arm works is **not** still: a part is brought in, a bin is moved, a person puts a hand in. The collision model has to be a **scene that changes**: objects that come and go and move, a way to say **what has changed since the plan was made**, and a way to say **how sure** the arm can be of something it has not seen for a while. And when the arm **picks something up** the arm itself changes shape.

## The Concept

### A scene with a version

A **planning scene** is a list of objects (spheres and boxes, each with an identifier) and a **version** that goes up with every addition, move or removal. A plan records the version it was made at. If the scene's version is not the plan's, the plan is **stale**: the world may have changed in a way that matters. Adding, moving and removing took the version from **0** to **4** (four changes), and a plan made at version 0 plus the first two additions was stale after the first move. Staleness does not mean the plan is wrong: when the object was moved **0.5 m** away the path was **still free**, and when it was moved **onto** the path it was **not**. The version says **recheck**; the check says whether. See `E-ARM-0070`.

### What has not been seen lately

A thing seen a moment ago is where it was seen. A thing **seen two seconds ago** might be anywhere within the distance it could have moved. Each object has a **time last seen** and a **speed it might move at** (zero for furniture), and the scene presented to the planner makes it **bigger by speed times age**, up to a cap. An object seen **2.0 s** ago that might move at **0.5 m/s** was **0.50 m** bigger in the radius (the cap was 0.5 m), and a pose that was **0.31 m** clear of a slower one was **0.11 m** clear once it had grown. Seen again, the object is back to its size; the furniture never grows. The growth is a statement of **ignorance**, and it makes the planner give a wide berth to what it cannot see, which is what ought to happen. See `E-ARM-0071`.

### A part in the gripper

When the gripper holds something, the arm is **longer and fatter at its tool**. A part **0.25 m** long, held in the gripper, took a pose from **0.090 m** clear of an obstacle (the arm alone) to **-0.090 m**: through it. Planning for the arm alone with a part in its hand is planning for an arm that is not there. The simplest way to carry the part is to lengthen the tool by the part's length and make the last link at least as fat as the part; a part thinner than the wrist changes only the length. See `E-ARM-0072`.

## Build It

The growth in `scene_at` in `exercise/solution.hpp`.

```
rcpp verify 30-07
```

## Use It

`rc/arm/scene_monitor.hpp`.

```cpp
rc::arm::PlanningScene scene;
scene.add(object);   scene.move(id, by, now);
const rc::arm::Scene model = rc::arm::scene_at(scene, now, 0.5);
if (rc::arm::plan_is_stale(plan_version, scene)) recheck_or_replan();
const rc::arm::HeldArm h = rc::arm::held_arm(radii, tool_length, part_length, part_radius);
```

**What this does not do.** The growth is **isotropic** (a ball growing in every direction) and the speed is a number you give, not an estimate from what was seen: a **tracker** (a velocity from two sightings) is what would do better, and 30-08 has a simple one. The part in the gripper is a **longer tool**, not a shape: a part that is wide, or that sticks out sideways, or that **is picked up and put down** (the version should go up at that moment) needs a capsule of its own. A **removed** object is gone at once: a thing that has gone out of view and may still be there is the case the growth is for, and the code has no rule for when to forget. There is **no locking**: a scene shared between a thread that sees and a thread that plans needs one. And a **plan in progress** does not stop because the scene changed: that is 30-08.

## What Breaks First

- **A plan used after the world changed.** See `E-ARM-0070`.
- **An object trusted for ever where it was last seen.** See `E-ARM-0071`.
- **Planning for an arm with nothing in its hand.** See `E-ARM-0072`.

## Ship It

`rc/arm/scene_monitor.hpp` holds `SceneObject`, `PlanningScene`, `scene_at`, `plan_is_stale`, `HeldArm` and `held_arm`.
