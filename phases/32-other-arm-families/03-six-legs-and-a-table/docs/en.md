# Six Legs and a Table: The Stewart Platform, Easy One Way and Iterative the Other, and Where Its Legs Lose Their Grip

> A parallel platform answers 'what lengths for this pose' with a formula and 'what pose for these lengths' with a search.

**Type:** Build
**Time:** about 110 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 32-02, 28-05, 28-06, 28-03

## The Problem

32-02's delta gave three movements, fast. A task that needs all six (a flight simulator seat, a telescope mirror, a part held and moved to a micrometre) wants a parallel platform that has the stiffness of six legs sharing a load, with no joint carrying the weight of the ones beyond it. The classic is the Stewart platform (or hexapod): six legs of changing length from a base to a platform.

## The Concept

### What lengths for this pose: a formula

A serial arm's forward kinematics is easy and its inverse is the hard part (28-04). A parallel platform is the other way round. The platform's pose is six numbers; the platform joints are turned and moved into the base's frame; and each leg's length is the distance from its base joint to its platform joint. No search, no branches. At home (the platform 0.30 m up) all six legs were 0.343 m. See `E-ARM-0109`.

### What pose for these lengths: a search

The other way has no simple formula (the general problem has many solutions). It is done by Newton's iteration (28-05): from a guess, find the six by six matrix of how each length changes with each pose value (by small differences), solve for the correction that would make the lengths right, and repeat. From a guess some way off it found the pose again to 0.000000000 in 4 steps. It returns false if the matrix is singular. The guess is the last pose, in a real controller, and that is why it converges so fast.

### The legs and the range they need

A raise of 50 mm and a tilt of 10 degrees needed legs between 0.366 and 0.410 m: the stroke of the legs is what limits the platform's reach, not a joint angle. A platform has small workspace for its size, like the delta. See `E-ARM-0110`.

### Where the legs lose their grip

The matrix of length changes is the Jacobian of the legs, and where it is nearly singular there is a direction in which the platform can move with the legs hardly changing, which is also a direction in which the legs cannot resist a force. The condition measure (smallest pivot over largest in the elimination) was 0.120 at home, fell to 0.038 with the platform turned 80 degrees about the vertical, and was 0.000, that is singular, at 90 degrees: for this layout of joints, a quarter turn of the platform is a pose to stay clear of. Singularities in a parallel platform are dangerous in a different way from a serial arm's: at one, the platform can give way under a load, not merely refuse to move. See `E-ARM-0111`.

## Build It

`stewart_lengths` in `exercise/solution.hpp`.

```
rcpp verify 32-03
```

## Use It

`rc/arm/stewart.hpp`.

```cpp
rc::arm::Stewart hexapod;
rc::arm::stewart_lengths(hexapod, pose, lengths);
```

**What this does not do.** The joints are points on circles with spherical joints at both ends and no limits on their angles; real ones have a cone, which limits the tilt more than the stroke does. The legs are perfect: no backlash, no compliance, no collisions between legs. The pose is Euler angles (z, y, x), with the gimbal problem at 90 degrees of pitch. The singularity scan is of the one geometry here and one kind of motion; the full set of singular poses is a research subject. The dimensions are typical. Forces (how six legs share a load) go through the transpose of the Jacobian and are not computed.

## What Breaks First

- **A pose sent without checking the leg stroke.** See `E-ARM-0110`.
- **A forward kinematics with no starting guess.** See `E-ARM-0109`.
- **A singular pose that was not looked for.** See `E-ARM-0111`.

## Ship It

`rc/arm/stewart.hpp` holds `StewartPose`, `Stewart`, `stewart_lengths`, `solve6`, `stewart_matrix`, `stewart_condition` and `stewart_pose`.
