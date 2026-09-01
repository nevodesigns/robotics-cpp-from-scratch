# Seeing It Move: A Picture Made of Numbers

> Six lessons of arithmetic, and you have never once seen the robot. That ends
> here, and it takes no libraries, no window and nothing installed.

**Type:** Build
**Time:** about 90 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 00-06

## The Problem

Your robot drives a quarter circle. You know because a test says the final
heading is 1.5708 radians, which is a number you have to trust rather than
something you have seen.

Now here are two paths. Both start at the same place, both end within a
centimetre of each other, and both pass every check in the previous lesson:

```text
  S...............................................E

  .......
 ...     ...
..         ..
.           .
..         ..
 E       ...
      S...
```

One drove straight. The other drove most of a circle and came back. The numbers
at the end barely separate them, and one glance does.

That is the whole argument for this lesson. A picture is not decoration. It is
the fastest bug detector you own, and it is the only one that shows you a
mistake you were not looking for.

## The Concept

### A picture is a mapping, and the mapping is where the bugs are

Drawing is two separate jobs, and only one of them is interesting.

The **arithmetic** turns metres into rows and columns: where does a robot at
(2.4, 1.1) go in a grid of 60 by 20? That is the part that is wrong when a plot
is wrong, and it is pure arithmetic that can be tested without drawing anything.

The **drawing** puts characters into a grid and prints it. That part is short,
dull and rarely wrong.

This lesson gives you the drawing and asks you to write the arithmetic, which is
the same split the Qt plotter uses in phase 10: two thirds of that file is not
Qt at all.

### Fit the path, not the origin

The rectangle a path occupies has to be measured from the path. The obvious
version starts at zero:

```cpp
PlotBounds bounds;               // min_x = 0, min_y = 0
for (const Pose& pose : path) bounds = include(bounds, pose);
```

A path that runs from (10, 20) to (12, 24) now has a rectangle from (0, 0) to
(12, 24), so the interesting two metres are drawn in a corner of a picture that
is mostly empty space around an origin the robot never visited.

The first pose seeds the rectangle. Every pose after it grows it.

### Screens count downward and robots count upward

A robot's `y` grows upward. A screen's rows grow downward, because text is
written from the top of a page.

So the distance **above the bottom of the path** becomes a distance **below the
top of the grid**:

```cpp
const double from_bottom = (y - bounds.min_y) * scale;
return (rows - 1) - static_cast<int>(from_bottom + 0.5);
```

Leave the flip out and every picture is a mirror image. That is worse than it
sounds, because a mirror image of a plausible path is another plausible path: a
robot curving left is drawn curving right and nothing looks broken.

Test it with a direction rather than by looking:

```cpp
RC_CHECK(row_for(1.0, ...) < row_for(0.0, ...));
```

### One scale, or the shape stops meaning anything

It is tempting to stretch each axis to fill the space. It fills the grid nicely
and it destroys what the plot was for: with different scales on the two axes, a
circle is an oval, a square is a rectangle, and the shape of the path has become
a fact about the window rather than about the robot.

Use one scale, the tighter of the two fits, and accept the empty space on one
side. That empty space is information: it says the path is taller than it is
wide.

There is one more twist, particular to terminals. A character cell is roughly
**twice as tall as it is wide**, so a metre across costs about twice as many
columns as a metre up costs rows. Ignore that and every circle comes out
squashed even though the arithmetic used a single number.

In a window with square pixels the factor is one. The idea does not change: a
picture is only evidence if both axes mean the same thing.

### The blank plot

The first time you draw a robot going in a straight line, the plot will be
empty, and nothing will tell you why.

A straight path along `x` has **no extent in `y`**. The scale divides by that
extent. Dividing by zero gives infinity rather than an error, every coordinate
becomes infinity, and the range check that keeps the drawing inside the grid
then skips every single point. The check does its job perfectly and the picture
is blank.

Decide what a flat axis means before dividing by it: it does not constrain the
scale, so let the other axis decide.

## Build It

Implement in `exercise/solution.hpp`:

- `include`, growing a rectangle to contain one more pose, seeded by the first.
- `bounds_width` and `bounds_height`.
- `scale_to_fit`, one number for both axes, allowing for the character shape and
  for an axis with no extent.
- `column_for` and `row_for`, the mapping, including the flip.

```
rcpp verify 00-07
```

The suite checks the arithmetic without drawing anything, and then draws your
robot from lesson 00-05 and prints it. That last test is the one to run for
yourself.

## Use It

Print the picture whenever something moves and you are not certain why. It costs
one function call and it catches the class of mistake that checks do not: the
ones you were not looking for.

The same arithmetic reappears in phase 10, drawn in a Qt window instead of a
terminal. What changes there is the surface, not the idea, and by then
installing Qt is a reasonable thing to ask. Here it would not be, which is why
this phase ends in a terminal.

## What Breaks First

- **A plot that comes out upside down.** The flip is missing, and a mirror image
  looks plausible. See `E-PLOT-0001`.
- **A circle drawn as an ellipse.** Each axis was stretched to fill the grid.
  See `E-PLOT-0002`.
- **A plot that comes out blank.** A straight path has no extent on one axis and
  the scale divided by it. See `E-PLOT-0003`.

## Ship It

The mapping joins `rc::sim`, and phase 00 is finished. A learner who arrived
having never programmed has now written a function, understood a build, read an
error, met the arithmetic that lies, driven a robot, questioned an answer that
compiled, and drawn the result.

Nothing was installed to get here beyond a compiler.
