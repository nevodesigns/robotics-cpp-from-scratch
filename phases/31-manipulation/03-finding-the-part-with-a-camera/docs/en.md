# Finding the Part With a Camera: From a Pixel and a Depth to a Point in the Arm's Frame, and Measuring Where the Camera Is

> A camera sees the part in its own frame; the arm needs it in the base frame, and the transform between them has to be measured.

**Type:** Build
**Time:** about 110 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 31-02, 28-09, 28-02

## The Problem

31-02's machine goes to "above the part", a number. Where the part is comes from a **camera**, and a camera says where the part is **in its own frame**. The arm's targets are in the **base frame**. Between them is a transform that nobody knows until it has been **measured**, and an error in it is an error in every pick. This lesson is the geometry of that, for a camera that is **fixed** in the cell and looks at the arm's workspace.

## The Concept

### A pixel is a ray; a depth makes it a point

A pinhole camera has focal lengths **fx, fy** and a principal point **cx, cy**, in pixels. A pixel (u, v) at a depth z (the distance along the optical axis, which a depth camera gives) is the point x = (u - cx) z / fx, y = (v - cy) z / fy. A pixel **60** pixels right of the centre at **0.5 m** was **0.050 m** to the side with a focal length of **600** pixels. See `E-ARM-0082`.

### The camera in the base frame is a measured transform

The point is in the camera frame. The base frame's point is that point moved by the transform from camera to base: a rotation and a translation. Nobody gets this from a ruler. It is **measured**: hold the tool's tip (whose place the arm's kinematics say, 28-02 and 28-09) at many places where the camera sees it, and collect pairs of the same point in the two frames.

### Fitting a rigid transform to points

With pairs of points a (camera) and b (base), the best rotation and translation (least squares) is found by taking the **centres** away, forming the **cross covariance** of the two clouds, and taking its **polar factor** (the rotation nearest to it, here by Newton's iteration of x going to half of x plus the inverse transpose of x). The translation then puts the centres together. With no noise the transform came back to **below a micrometre** and a degree-millionth. With **2 mm** of noise on what the camera saw, **5 points** gave **3.5 mm** and **0.64 degrees** and **40 points** gave **1.1 mm** and **0.20 degrees**: more points average the noise out. See `E-ARM-0083`.

### Points that lie in a plane give no fit

If the points all lie in **a plane** (or a line) the rotation about the axis normal to them is not determined, and the fit says so and returns no transform. Touch points must be **spread in three dimensions**.

### A small tilt is a large error at the end of a reach

A rotation error of **1 degree** put a part **10.5 mm** out at **0.6 m**, because an angle error is multiplied by the distance. A millimetre of noise in a point is a millimetre; a degree of error in the rotation is ten. See `E-ARM-0084`.

## Build It

`back_project` in `exercise/solution.hpp`.

```
rcpp verify 31-03
```

## Use It

`rc/arm/hand_eye.hpp`.

```cpp
rc::arm::FitResult f = rc::arm::fit_rigid(seen_by_camera, from_kinematics);
if (!f.ok || f.rms > 0.003) recalibrate();
```

**What this does not do.** The camera is **fixed** (eye to hand). A camera on the arm's wrist (eye in hand) is calibrated from **motions** and an equation of the form A X = X B, which is not here, and is a harder fit. The lens is a **pinhole** with no distortion, the depth is exact along the axis, and the arm's own kinematics are taken as exact (28-09 is why they are not). There is no way here to pick out the tool in the image; the points are given.

## What Breaks First

- **A camera's frame used as the base's.** See `E-ARM-0082`.
- **A calibration from too few or too flat points.** See `E-ARM-0083`.
- **A degree of tilt and a long reach.** See `E-ARM-0084`.

## Ship It

`rc/arm/hand_eye.hpp` holds `Intrinsics`, `back_project`, `Cloud3`, `FitResult`, `fit_rigid` and `camera_to_base`.
