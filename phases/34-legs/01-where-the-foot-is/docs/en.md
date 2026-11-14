# Where the Foot Is: A Leg of Three Joints, Its Forward and Inverse Kinematics, the Two Knees, and the Straight Leg

> A leg is a two link arm hung from a hip that can also lean sideways, and it has the same two answers to every question.

**Type:** Build
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 28-04, 32-01, 28-06

## The Problem

Phases 28 to 33 gave the course arms and wheeled bases. A legged robot stands and walks on legs, and every question about a leg starts the same way as an arm's: where is the foot, and what joint angles put it there? A quadruped has four legs of three joints each, and a walking gait is a list of places for four feet. So the first lesson of the legged track is the geometry of one leg, in closed form, with its two answers.

## The Concept

### Three joints

A typical small quadruped leg has a hip that leans the leg sideways (about the forward axis), a hip that swings it forward and back (about the sideways axis) and a knee. The pitch axis is offset sideways from the lean axis by a distance (here 0.08 m), and the thigh (0.21 m) and shank (0.21 m) are typical of a 12 kg robot. The numbers are typical, not one maker's. See `E-LEG-0001`.

### Forward kinematics

The thigh and the shank give a place in the plane of the leg: forward l1 sin q1 + l2 sin (q1 + q2) and down l1 cos q1 + l2 cos (q1 + q2). The lean turns that plane about the forward axis, which carries the sideways offset round with it. The result is the foot in the frame at the hip: x forward, y left, z up.

### Inverse kinematics, in closed form

Put the foot's place in the plane across the body: its distance from the lean axis is the square root of y squared plus z squared. The foot's depth in the leg's plane is what is left after the offset, d = sqrt(y squared + z squared less offset squared), and the lean is the direction of the foot less the direction of the offset and that depth. Then the leg's plane holds the two link problem of 28-04: cos q2 = (x squared + d squared less l1 squared less l2 squared) over 2 l1 l2, and q2 is plus or minus its arccosine: the knee bent one way or the other, as an elbow is. A quadruped's knees bend backward, so a robot uses one sign for good. The foot came back to the place asked to 0.000000000 m over a grid of 756 places, of which 680 were reachable (a place nearer the lean axis than the offset is refused, and so is one beyond the leg's length, 0.50 m below the hip), and the two knees gave the same foot to 0.000000000 m. See `E-LEG-0002`.

### Standing

With the foot 0.30 m straight below the hip the lean is zero and the knee is bent -89 degrees, the thigh forward and the shank back, which is the pose of a standing dog. A leg that stands lower bends more.

### The straight leg

At full stretch (0.42 m) the knee is straight: its angle's sine, the `straightness`, was 0.000. A straight leg is a singularity, as 32-01's arm was: the foot cannot move farther along the leg, and the joint rates for a foot velocity grow without bound near it. The same leg is also the strongest for standing (34-02): the force goes along the bones. A walking robot keeps its knees bent to keep control, and 34-02 says what that costs. See `E-LEG-0003`.

## Build It

`leg_ik` in `exercise/solution.hpp`.

```
rcpp verify 34-01
```

## Use It

`rc/legged/leg_kin.hpp`.

```cpp
rc::legged::LegGeom leg;  double q[3];
if (!rc::legged::leg_ik(leg, x, y, z, -1, q)) refuse_the_foot_place();
```

**What this does not do.** The leg is rigid and exact: no backlash, no sag of the links, and the zeros of the joint angles are taken as true (34-08 measures them). The hip is a point with the lean and pitch axes meeting at the offset; real hips have more offsets. Joint limits are not applied: a real knee cannot straighten fully or fold flat. The numbers are typical of a small quadruped.

## What Breaks First

- **The wrong knee sign.** See `E-LEG-0002`.
- **A leg offset forgotten.** See `E-LEG-0001`.
- **A leg driven to full stretch.** See `E-LEG-0003`.

## Ship It

`rc/legged/leg_kin.hpp` holds `LegGeom`, `FootPos`, `leg_fk`, `leg_ik` and `straightness`.
