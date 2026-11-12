# Watching a Run: The Robot as a Moving Picture, an Animated SVG Made From the Scenario and the Log, Opened in Any Browser

> A table of numbers says the run was wrong; a picture shows where, in one look.

**Type:** Build
**Time:** about 70 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 22-07, 22-08, 21-10

## The Problem

A log is numbers, and numbers hide the thing a person would see at once. A robot that drifted left on the sand, that clipped a post, that circled its goal and did not reach it, is a picture first and a table of errors much later. The same scenario and the same log that the earlier lessons write can be drawn, and the drawing is cheap: an **SVG**, which is text, which any browser opens and animates, with no library and no window system. This lesson draws it.

## The Concept

### What is drawn

`to_svg` takes the scenario and the log and writes one SVG: the terrain **patches** in a colour each (tarmac grey, gravel tan, grass green, sand yellow, ice pale blue), the **walls** as lines, the **posts** as discs, the **goals** as dashed rings of their tolerance, the **path** of the `pose` channel as a line, and a **triangle for the robot** that moves along that path in step with the log's own times, repeating. A 10 s run of **500 poses** came to **24.5 kB**, about 50 times smaller than a video and with every number still in it.

### The playback speed

The animation lasts as long as the run, **9.98 s**, at normal speed, and **2.50 s** at four times. The key times are taken from the log's own time stamps, not from the index of the sample, so a log with a gap in it plays with a gap in it. See `E-FIELD-0028`.

### Up is up

A robot's y axis points up. An SVG's points down. A picture that ignores this is the mirror image of the run: a left turn on the field is a right turn on the screen, and nothing about it looks wrong until someone compares it with the robot. The world is therefore drawn inside a group with `scale(1,-1)`, and the view box starts at **minus the top** of the world. See `E-FIELD-0026`.

### Everything in the frame

`bounds_of` finds the rectangle that holds the start, the patches, both ends of each wall, each post and goal with its radius, and every pose, and the view adds a margin to it. The picture of the yard was **16.0 m by 9.0 m**, and every pose of the run lay inside the bounds. A view sized to the walls alone cuts off a robot that leaves the room, which is exactly the run that most needs to be seen. See `E-FIELD-0027`.

### Text in the file

Names are escaped (`<loop>` becomes `&lt;loop&gt;`), a number is never written as `nan`, and the same input gives the same text, so a picture can be kept next to the log it came from and compared with `diff`.

## Build It

`bounds_of` in `exercise/solution.hpp`.

```
rcpp verify 22-09
```

## Use It

`rc/field/watch.hpp` opens the module `rc::field`.

```cpp
rc::field::ViewOptions view;
view.speed = 4.0;
const std::string svg = rc::field::to_svg(scenario, log, view);
write_file("run.svg", svg);   // then open run.svg in a browser
```

**What this does not do.** The triangle takes its heading from the log and does not know the robot's real outline or wheel base. There is no scrubber and no pause: the browser plays the animation, nothing more. Other channels (the lidar, the estimated pose beside the true one) are not drawn, and the terrain is rectangles of a colour, with no slope shown. A real viewer is an application; this is a picture a lesson can check.

## What Breaks First

- **A picture that is the mirror image of the run.** See `E-FIELD-0026`.
- **A view sized to the walls.** See `E-FIELD-0027`.
- **An animation whose speed is the rate of the log.** See `E-FIELD-0028`.

## Ship It

`rc/field/watch.hpp` holds `Bounds`, `ViewOptions`, `bounds_of`, `xml_escape`, `surface_colour` and `to_svg`.
