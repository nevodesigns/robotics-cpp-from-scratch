# The Shape of the Arm: Capsules for the Links, Spheres, Boxes and a Floor for the World, and the Pairs of Links That Must Not Be Asked

> A planner never sees the arm, it sees what the collision model says, and the model had better be slightly too big.

**Type:** Build
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 29-01, 28-02, 28-04

## The Problem

Phase 28 told us where the tool is and phase 29 how the arm moves. A planner for an arm needs to know one thing more, over and over, thousands of times a second: if the arm were in these joint angles, would it touch anything? The arm has a body, long links that sweep through space as the joints turn, and the planner does not look at pictures: it asks a collision model.

## The Concept

### A capsule for a link

The cheapest shape that fits a link well is a capsule: a line segment with a radius, a rod with rounded ends. A link of a robot arm is a long thing between two joints, and a capsule from the origin of the frame before the link to the origin of its own frame, with the radius of the link, is a good fit and costs one segment and one number. The last one is carried on to the tip of the tool. Links that have no length (the elbow joint of the typical arm, where `a` and `d` are zero) have no capsule: of six links this arm has four capsules.

### Obstacles are simple too

A scene is made of spheres (a ball, a person's head, a point and a margin), boxes (a table top, a bin, axis aligned) and a floor (a height below which nothing may go). They are rough, and that is the point: a model that is a little too big makes the planner keep a little further away than it needs to, and a model that is too small makes it hit things. See `E-ARM-0052`.

### Distances, signed

What the planner wants is not a yes or a no but a distance: negative when the capsule overlaps the obstacle, positive when clear. The distance between two segments is a standard calculation (the closest points of two segments, clamped to their ends); a capsule to a sphere is that between the segment and the centre, less the two radii. The distance of a segment to a box is the one thing without a closed form, and a small trick does it: the distance of a point to a convex set is a convex function of the point, so along a segment it has one minimum, and a ternary search finds it. Between two crossing segments 1 m long at a height of 0.3 m the distance was 0.300 m; a capsule of radius 0.1 above a sphere of radius 0.2 with centres 1 m apart was 0.700 m clear (1 - 0.1 - 0.2 is 0.7); a segment 0.500 m above a box.

### The floor

The floor is the table the arm stands on, so the first capsule, the base column, is left out of the floor check (it stands on it). With the tool at (0.5, 0, 0.3) pointing down, the arm was 0.140 m above a floor at 0.1 m, and with the floor raised to 0.4 m it was -0.160 m: negative, through it. The signed number says by how much, which is what a planner can use to push the arm out again. See `E-ARM-0053`.

### Which pairs of links to ask

An arm can hit itself: the flange can come down on the base column. But two links that share a joint are always touching (at the joint), so asking about them is a question whose answer is always yes. The rule used here is that only capsules more than one apart in the list are asked: for four capsules, 3 pairs out of the 6 that there are. With the tool brought down beside the base the flange and the base column were -0.120 m apart (they touch). See `E-ARM-0054`.

## Build It

The self-collision pairs of `scene_distance` in `exercise/solution.hpp`.

```
rcpp verify 30-01
```

## Use It

`rc/arm/collision.hpp`.

```cpp
const auto caps = rc::arm::arm_capsules(table, q, radii, tool_length);
const rc::arm::Clearance c = rc::arm::scene_distance(caps, scene, true);
if (c.d < margin) reject(q);
```

**What this does not do.** A capsule is a rough fit: a real link has a shape, a motor sticking out, a cable, a gripper that is wider than the wrist. The radii are the author's and have to be measured or taken from the model of the arm; here they are 4 to 8 cm. The adjacent pairs are never asked, which is right at the joint and wrong where the arm folds right back on itself (the joint limits are what stop that, and they have to be right: 28-07). The obstacles are axis aligned boxes and spheres, no meshes, and nothing moves (30-07, 30-08). The floor is a plane at one height. The distance is a minimum over all pairs, not a map of where it is smallest, which the planner would want for a push away. No speed was measured: for thousands of queries a second the checks have to be fast, and these have not been timed.

## What Breaks First

- **A collision model smaller than the arm.** See `E-ARM-0052`.
- **A model that says yes or no and not how much.** See `E-ARM-0053`.
- **Self-collision of links that share a joint.** See `E-ARM-0054`.

## Ship It

`rc/arm/collision.hpp` opens the planning half of `rc::arm`: `Capsule`, `Sphere`, `Box`, `Scene`, `seg_seg_distance`, `point_box_distance`, `seg_box_distance`, `capsule_sphere`, `capsule_box`, `capsule_floor`, `capsule_capsule`, `arm_capsules`, `Clearance`, `scene_distance`, `collides` and `self_pairs`.
