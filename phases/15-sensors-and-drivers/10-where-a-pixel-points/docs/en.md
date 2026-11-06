# Where a Pixel Points: The Camera as a Function, and What Its Lens Does to Straight Lines

> A camera sends a point to a pixel by dividing by its depth. A pixel is a direction and not
> a position, and a real lens bends it more the further it is from the centre.

**Type:** Build
**Time:** about 150 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 15-06

## The Problem

Lesson 15-06 found a blob in a frame and said where it is, in pixels. A robot wants to know where it is in
the world, and a pixel does not say. It says which way to look, not how far.

Getting from one to the other is a function, and it is a function with a lens in it. The lens is what makes a
position found near the middle of the picture accurate and the same position near the edge wrong by tens of
centimetres.

## The Concept

### The pinhole

A point at `(X, Y, Z)` in front of the camera lands at

```text
u = fx * X / Z + cx
v = fy * Y / Z + cy
```

The focal length is in pixels, and the centre is where the optical axis meets the sensor. A point at
(0.5, 0.2, 2.0) with a focal length of 500 lands at (445, 290). Twice as far away is half as far from the
centre of the picture.

Dividing by the depth is the whole of it, and it has a trap. A point behind the camera has a negative depth, and
the division gives a perfectly good pixel: the mirror image of the point the same distance in front. A point
behind the lens does not appear mirrored, it does not appear. `project` says so with a flag and returns no pixel
for it. See `E-VIS-0007`.

### A pixel is a direction

The inverse of projection is not a position. Everything along a line from the camera lands on one pixel, so a pixel
says only where the line goes: `X / Z` and `Y / Z`. How far along it is not in the picture, and has to come from
somewhere else: the size of something, a second camera, the floor.

### The lens

A pinhole has no lens. A real one bends light more towards the edge of the picture than the middle, and the usual model
scales a point's distance from the centre by `1 + k1 r^2 + k2 r^4`, where `r` is the distance, in the normalised
coordinates, from the centre. A wide lens has a negative `k1`, barrel distortion, which pulls the edges in.

The bend goes as a high power of the radius, so it is invisible in the middle and large at the edge. A lens with
`k1 = -0.25` and `k2 = 0.05` on a 640 by 480 picture with a focal length of 500 pixels, how far a point's pixel is from
where a pinhole would put it:

| normalised radius | pinhole | with the lens | moved |
|---|---|---|---|
| 0.1 | 50.0 px | 49.9 px | 0.1 px |
| 0.2 | 100.0 | 99.0 | 1.0 |
| 0.4 | 200.0 | 192.3 | 7.7 |
| 0.6 | 300.0 | 274.9 | 25.1 |
| 0.8 | 400.0 | 344.2 | 55.8 |

At the corner that is 56 pixels. At two metres a pixel is 4 mm, so ignoring the lens puts the thing **22.3 cm**
from where it is. And a straight edge, a row of points one metre above the axis two metres away, bowed by **15.9
pixels** off the line through its ends. See `E-VIS-0008`.

### Undoing the lens

Projection is easy and its inverse is not. The bend depends on the radius of the point being looked for, which is the
thing not yet known. So it is iterated: guess that the point is where the pixel says, work out the bend there, correct the
guess, repeat. Each pass is nearer.

The corner pixel, taken back to its direction, how far out after `n` passes:

| passes | error |
|---|---|
| 1 | 14.5 px |
| 2 | 3.94 px |
| 3 | 1.08 px |
| 5 | 0.083 px |
| 10 | 0.000134 px |
| 20 | 3.5e-10 px |

One pass, which is what a quick correction does, leaves the corner 14.5 pixels out. The other quick fix, applying the lens
model again with its coefficients negated, which looks like an inverse, is out by 18.9. After the full inverse, the
straight edge is level to 1.8e-11 of a pixel, and across 143 directions over the whole picture the worst round trip missed by
1.5e-10 of a pixel. See `E-VIS-0009`.

### Range from size

If a target's real width is known, similar triangles give how far away it is: `Z = f * width / pixels`. It is simple and it has
a weakness that is the same fact as everything above, that a pixel is a pixel.

A target 0.2 m wide, measured one pixel too wide:

| true distance | appears | error |
|---|---|---|
| 1.0 m | 100.0 px | -0.010 m |
| 2.0 m | 50.0 px | -0.039 m |
| 5.0 m | 20.0 px | -0.238 m |
| 10.0 m | 10.0 px | -0.909 m |

The target gets smaller as it recedes and the pixel does not, so the error goes as the square of the distance: a centimetre at a
metre and nine tenths of a metre at ten. A focal length that is two percent out makes every distance two percent out.

## Build It

`project`, `unproject` and `distance_from_width` in `exercise/solution.hpp`. The comments give the formulas and the iteration.

```
rcpp verify 15-10
```

The test for the lens is written from the formula in the test and not from your code, so a wrong sign or a wrong power shows in
the table.

## Use It

`rc/vision/camera.hpp` joins `rc::vision`.

```cpp
rc::vision::Intrinsics camera;
camera.fx = camera.fy = 500.0;
camera.k1 = -0.25;
const rc::vision::Direction d = rc::vision::unproject(camera, rc::vision::Pixel{u, v});
// a ray from the camera: (d.x, d.y, 1), to be scaled by a depth found some other way
```

**What this does not do.** The numbers are not on a datasheet. They come from photographing a pattern of known geometry and
fitting, which is a calibration of its own, and a lens changes with temperature and focus. It has two radial coefficients and no
tangential ones, which a lens that is not centred on its sensor has, and it assumes square pixels. And it is a model of a lens
that is at most mildly wide: a fisheye of 180 degrees does not fit a polynomial in the square of the radius and has its own models.

## What Breaks First

- **A point behind the camera.** It appears mirrored. See `E-VIS-0007`.
- **A lens ignored.** Centimetres of error at the edge and lines that bow. See `E-VIS-0008`.
- **An inverse applied once, or a range from a tiny target.** See `E-VIS-0009`.

## Ship It

`rc/vision/camera.hpp` holds `Intrinsics`, `Pixel`, `Direction`, `project`, `unproject` and `distance_from_width`.

Lesson 15-06 said where in the picture. This says where in the world the line through that place goes, which is the last thing
between a blob and a robot that can drive to it.
