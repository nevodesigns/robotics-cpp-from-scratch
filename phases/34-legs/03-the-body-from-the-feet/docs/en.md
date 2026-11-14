# The Body From the Feet: A Plane Through the Stance Feet, and the Height, Roll and Pitch of the Body Above It

> A body on its feet knows how it is tilted by how the ground looks from where the feet are.

**Type:** Build
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 34-02, 34-01, 25-09

## The Problem

A walking robot needs to know how its body is tilted and how high it is. A good estimate comes from an IMU (35-04), and the IMU drifts and is jolted by every step. The feet give a second opinion that does not drift: the legs' joint angles (34-01) give the places of the feet in the body's frame, and if the feet are on level ground they all lie in one plane. The plane, seen from the body, is the body's tilt and height.

## The Concept

### A plane through the stance feet

The feet on the ground (the stance feet, 34-04) have places in the body's frame. Fit the plane `z = c0 + c1 x + c2 y` through them by least squares: the normal equations have three unknowns and are solved by Cramer's rule. It needs three feet that are not in a line; with fewer, or with all in a line, there is no plane and the function says so. A foot that is in the air (a swing foot) must be left out: it is not on the ground, and putting it in would tilt the plane. See `E-LEG-0007`.

### From the plane to the pose

The plane's unit normal, seen from the body, is (minus sin of the pitch, sin of the roll times cos of the pitch, cos of the roll times cos of the pitch), so the pitch is the arcsine of minus its x and the roll is the arctangent of its y over its z. The height is the distance of the body's origin from the plane. A body 0.30 m up, rolled 5 degrees and pitched -3 degrees was found again from its four feet to 0.000000000 m (the height is the distance of the body's origin from the plane, which for level ground is its height whatever the tilt), 0.000000000 degrees and 0.000000000 degrees. Three feet gave the same answer when the places were exact.

### Noise in the feet

The feet's places come from the joint angles, which have noise and, if the zeros are not true, a bias (34-08). With 5 mm of noise in each coordinate of each foot, the height was uncertain by 2.5 mm, the roll by 0.92 degrees and the pitch by 0.65 degrees with four feet on the ground, and by 3.6 mm, 1.29 degrees and 0.95 degrees with three: a fourth foot averages some of the noise out. The tilt is uncertain by about a degree from noise of 5 mm, because the feet are only about 0.4 m apart: a small base makes a poor tilt sensor. See `E-LEG-0008`.

### What it assumes

The ground is flat and level under all the stance feet, and the feet do not slip. On a slope the plane is the slope's, and what the robot learns is its tilt relative to the ground, which is what the legs need and is not its tilt relative to gravity. 35-04 brings in the IMU for that, and the two are combined (the feet to correct the drift, the IMU to see through a slope).

## Build It

`fit_ground_plane` in `exercise/solution.hpp`.

```
rcpp verify 34-03
```

## Use It

`rc/legged/body_pose.hpp`.

```cpp
const rc::legged::BodyPose p = rc::legged::body_pose_from_feet(feet_in_body_frame, on_ground);
if (!p.ok) use_the_imu_alone();
```

**What this does not do.** It finds height, roll and pitch and not the heading or the position along the ground (a foot that is on the ground does not tell where on the ground it is). The ground must be level, the feet not slipping, and which feet are on the ground is given (34-07 senses it). The noise is independent and the same on all feet, which joint zero errors are not. The hip places in the body's frame are exact. The numbers are typical of a 12 kg robot.

## What Breaks First

- **A swing foot left in the fit.** See `E-LEG-0007`.
- **A tilt taken from feet that are close together.** See `E-LEG-0008`.
- **Three feet in a line.** See `E-LEG-0007`.

## Ship It

`rc/legged/body_pose.hpp` holds `BodyPose`, `fit_ground_plane` and `body_pose_from_feet`.
