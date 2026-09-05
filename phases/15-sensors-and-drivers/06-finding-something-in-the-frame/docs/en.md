# Finding Something in the Frame: Thresholds, Blobs and Where a Thing Is

> With the lens covered, the vision found the marker: one blob, two hundred
> pixels, dead centre. The failure was not that it found nothing.

**Type:** Build
**Time:** about 150 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 15-05, 07-03

## The Problem

Lesson 15-05 read the frame correctly. This one does something with it: find the
bright marker and say where it is.

That is three decisions, each of which looks like a fact:

- what counts as bright,
- what counts as one thing,
- where a thing is.

Getting each of them wrong produces a confident answer rather than an error.

## The Concept

### A fixed threshold is a claim about the lighting

A marker at 200 on a background at 40, as the lights come up:

| extra light | background | marker | fixed 128 finds | Otsu finds |
|---|---|---|---|---|
| 0 | 40 | 200 | 20 px | 20 px |
| 40 | 80 | 240 | 20 px | 20 px |
| 80 | 120 | 255 | 20 px | 20 px |
| 120 | **160** | 255 | **200 px** | 20 px |

The marker is 20 pixels. At the last row the background is above 128, so
everything passes and the "marker" is the whole picture, whose centre is the
middle of the frame, which is where the robot then goes.

Read the first three rows again. It worked, exactly, three times. **That is how
it ships.**

So take the threshold from the picture:

```cpp
const int threshold = otsu_threshold(histogram_of(image));
```

Otsu's method tries every threshold and keeps the one that puts the most
variance between the two groups it makes. It is one pass over 256 histogram
buckets, which costs nothing next to reading the frame, and it found exactly the
marker at every exposure above.

The histogram is worth noticing on its own. A million pixels become 256 numbers,
and every question about the overall brightness of a frame is answerable from
those 256 rather than from the million.

### What counts as one thing

Two pixels touching only at a corner are the same region, or they are not.
Nothing in the picture says which.

```
  .......
  .#.....
  ..#....
  ...#...
  ....#..
  .....#.
  .......
```

| connectivity | blobs |
|---|---|
| four | **5** |
| eight | **1** |

A crack in a surface wants one answer. Two markers that happen to line up at a
corner want the other. Choose deliberately:

**Eight** joins things that touch at a corner, which is usually right for a
physical object photographed at an angle, where a thin part of it may come out
one pixel wide.

**Four** keeps them apart, which is right when the count matters more than
completeness: separate markers, holes, defects.

A solid shape is one blob under both, so a test on a square cannot tell them
apart.

### Where a thing is

The centre of mass and the centre of the bounding box are different points for
anything not symmetric. An L six pixels tall:

| | position |
|---|---|
| centre of mass | 2.36, 4.64 |
| centre of the box | 3.50, 3.50 |
| apart | **1.61 pixels** |

A quarter of the object.

**The centroid is where the thing is**, and it is what a gripper or a heading
should aim at. **The box centre is where the space it occupies is**, and it is
what a collision check or a crop wants. Different questions, and the code should
say which is being asked.

On a rectangle they agree exactly, which is once again what a test on a square
marker shows.

### The threshold answers the wrong question

Here is the one that matters most, and it is not the failure people expect.

An evenly lit wall, every pixel 90, nothing in the scene at all:

| | |
|---|---|
| Otsu says | **0** |
| blobs found | 1 |
| pixels in it | **200 of 200** |

With no two groups to separate, the best split is at the bottom of the range and
every pixel is above it. The vision does not report nothing. It reports
**everything**, as one blob covering the frame, centred in the middle of the
picture, which looks exactly like a marker sitting straight ahead.

Covered lens, failed light, runaway exposure: all the same answer, all
confident.

An automatic threshold answers **where to split**, not **whether there is
anything to split**. Something after it has to decide that, and it is cheap:

```cpp
const double fraction = double(blob.pixels) / (image.width * image.height);
if (fraction > 0.5) return NotFound{};   // that is the room, not the marker
```

At the other end of the same problem, one hot pixel on the sensor is also a
perfectly good blob, of one pixel. So a size range in pixels, from how large the
object is and how far away it can be, rejects both ends with two comparisons.

**A size range is not an optimisation. It is part of saying what you are looking
for.**

And the function needs a way to say no. A vision routine that always returns a
position is one that will eventually return a wrong one with no way to tell,
which is exactly the argument the sensor channel made in lesson 15-04.

## Build It

Implement `histogram_of`, `otsu_threshold` and `find_blobs` in
`exercise/solution.hpp`.

```
rcpp verify 15-06
```

The suite raises the lights on a marker four times, counts a diagonal chain both
ways, measures an L, and points the camera at a blank wall.

## Use It

**Threshold from the picture, then decide whether the picture had anything in
it.** Two steps, and the second is the one people leave out.

**Write the connectivity and the centre choice down** with the reason, at the
call site. Both read like implementation details and both change the answer.

**Give every detection a size range**, and make "nothing found" a value the
caller has to handle.

**Test with a diagonal, an asymmetric shape, two exposures and a blank frame.**
A square marker at one exposure passes every fault in this lesson.

**Control the lighting where you can.** An automatic threshold is a defence
against variation, not a substitute for a scene you designed, and a marker much
brighter or much darker than anything else in the frame makes both the threshold
and its failures obvious.

## What Breaks First

- **A threshold that was a claim about the lighting.** See `E-VIS-0004`.
- **One object or five, and the centre that is not the centre.** See
  `E-VIS-0005`.
- **A threshold on a picture with nothing in it.** See `E-VIS-0006`.

## Ship It

`histogram_of`, `otsu_threshold` and `find_blobs` join `rc::vision` beside the
image view. A camera is now a sensor like the others in this phase: it produces
a measurement, that measurement has a way of being absent, and the caller has to
handle it.
