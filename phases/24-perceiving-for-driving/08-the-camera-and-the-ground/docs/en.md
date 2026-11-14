# The Camera and the Ground: Where on the Floor a Pixel Is, How the Answer Falls Apart Towards the Horizon, and What a Degree of Pitch Does to It

> A camera tells you which way, and the floor tells you how far, but only where the floor is flat and the camera's height and tilt are known.

**Type:** Build
**Time:** about 85 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 22-02, 24-02, 21-10

## The Problem

A lidar measures distance. A camera does not: it measures direction, a ray for each pixel, and a distance has to come from somewhere else. For a vehicle that drives on a floor there is a place it comes from that costs nothing: **the floor itself**. A ray that hits the floor hits it at a distance fixed by the height of the camera and the tilt of the ray. Everything on the floor, a line, a kerb, the foot of a wall, a puddle, can be placed on the map from one image. That is the **ground plane**, the commonest assumption in the vision of a driving robot, and it is only as good as three numbers: the height, the pitch, and the flatness of the floor.

## The Concept

### A pixel is a ray

A pinhole camera has a focal length in pixels in each direction and a principal point. A pixel `(u, v)` is the ray `(1, (cx - u) / fx, (cy - v) / fy)` in a frame that is level: forward, left, up. Tipped down by the `pitch` the ray is `(cos(pitch) + up sin(pitch), left, -sin(pitch) + up cos(pitch))` in the vehicle's frame. It meets the ground when its height has fallen by the height of the camera: `t = -height / z`, and the point is `t` times the other two. `pixel_to_ground` does this, and says false for a pixel at or above the horizon, which looks at no ground at all. `ground_to_pixel` is the inverse, and a grid of pixels went to the ground and back to the same pixel to 1e-6. The left of the image is the left of the vehicle.

For a camera **0.30 m up, tipped down 15 degrees**, with a focal length of 500 pixels in an image of 640 by 480, the horizon was at **row 106**, the centre row looked at the ground **1.12 m** ahead, row 140 at **4.65 m** and row 120 at **11.42 m**. The distance of a row is the height over the tangent of the angle it looks down by, which is checked.

### The camera cannot tell distance well far away

One row at the centre of the image spans **0.0090 m** of ground. At row 140 it spans **0.139 m**, and at row 120, **0.82 m**: a single row of the picture is **most of a metre** of floor at 11 m, and it grows as the square of the range, so that a few rows nearer the horizon are tens of metres. A camera on the ground is accurate to a centimetre at a metre and no use at all at fifty. Ranges that matter to a vehicle that is moving quickly are the ranges at which the camera is poorest. See `E-VEH-0049`.

### Three numbers must be right

The height, the pitch and the flat floor. A height **10 percent wrong** put every distance **10 percent** wrong, in proportion. A pitch **1 degree wrong** put row 140 **37 percent** wrong in range, because far rays are almost level and a degree is a large part of their angle to the ground; near the camera the same error mattered less than 10 percent. And the vehicle itself pitches as it speeds up and brakes, which is a pitch error that changes from moment to moment; and a floor that slopes up or down is a different plane. See `E-VEH-0050` and `E-VEH-0051`.

## Build It

`pixel_to_ground` in `exercise/solution.hpp`.

Open `exercise/solution.hpp`. The part you write is marked `TODO`; the rest of the file is already written, and it is worth reading before you start, because your part is called by it or calls it.

What to write, as the note beside the `TODO` puts it:

- Tipped down by the pitch the ray is (cos(pitch) + up sin(pitch), left, -sin(pitch) + up cos(pitch)) in the vehicle's frame; it meets the ground when its height has fallen by `height`, at t = -height / its z; the point is t times its x and y.

As shipped, all 5 of the 5 tests fail. They are, in the order the suite runs them:

- A pixel looks at a point of the ground, and the point falls back on the same pixel.
- The horizon is above the centre of the image by the focal length times the tangent of the pitch, and nothing above it is ground.
- The distance of a row is the height over the tangent of the angle it looks down by.
- One row spans more ground the further away it looks, and nearly all of it at the horizon.
- A pitch a degree wrong moves the far ground a lot, and a height 10 percent wrong moves it by 10 percent.

```
rcpp verify 24-08
```

A failing check names the test and prints the number it expected beside the one it got, so start from the first failure and work down. When the suite is green, read `reference/solution.hpp` beside your own version: where the two differ and both pass, the difference is worth understanding.

## Use It

`rc/perceive/ground_camera.hpp` opens nothing new in `rc::perceive`.

```cpp
rc::perceive::Camera cam;
double x = 0.0, y = 0.0;
if (rc::perceive::pixel_to_ground(cam, 0.30, pitch, u, v, x, y)) map.mark(x, y);
```

**What this does not do.** There is no lens: the pixels are those of an image with its distortion already removed, which is what a calibration (a later phase of the course) gives. The camera is taken to be level about its own axis, with no roll, and straight ahead with no yaw. The floor is level and flat. Anything above the floor, such as a person's head, is placed on the floor behind where it really is, which is why this is for the things *on* the ground, and why a wall is placed at its foot. And it does not tell what a thing is.

## What Breaks First

- **A distance read from a far row.** See `E-VEH-0049`.
- **A camera pitch that was never measured.** See `E-VEH-0050`.
- **A floor that is not flat.** See `E-VEH-0051`.

## Ship It

`rc/perceive/ground_camera.hpp` holds `Camera`, `pixel_to_ground`, `ground_to_pixel`, `horizon_row` and `ground_step`.
