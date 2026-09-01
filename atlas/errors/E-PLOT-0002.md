id: E-PLOT-0002
title: A circle drawn as an ellipse
match: a circle is drawn round, not stretched
platforms: linux, windows
teaches: 00-07-seeing-it-move
---

## Symptom

A robot driving in a circle is drawn as an oval. A square path is drawn as a
rectangle. Worse than either: a robot that is genuinely drifting sideways looks
identical to one that is not, because the picture distorts the shape by more
than the drift does.

## Cause

Each axis was scaled to fill the space available:

```cpp
const int column = (x - min_x) / width  * (columns - 1);    // the bug
const int row    = (y - min_y) / height * (rows - 1);       // and again
```

That fills the grid, which looks like the goal, and it destroys the one thing a
plot is for. Once the axes have different scales, the shape of the path is a
fact about the window rather than about the robot, and no shape in the picture
can be trusted.

There is a second, smaller version of the same mistake specific to terminals. A
character cell is roughly **twice as tall as it is wide**, so a metre across
costs about twice as many columns as a metre up costs rows. Using one scale
without allowing for that draws every circle squashed, even though the
arithmetic used one number.

## Fix

One scale for both axes, chosen as the tighter of the two fits, and the
character shape allowed for:

```cpp
const double by_height = usable_rows / height;
const double by_width  = usable_columns / (width * 2.0);   // 2.0 is the cell shape
return by_height < by_width ? by_height : by_width;
```

Then the picture has empty space on one side, and that empty space is correct:
it is what tells you the path is taller than it is wide.

The test that catches this cannot be a test of one point, because a single wrong
scale still maps every point consistently. Drive a circle, find the extent of
the drawn points in each direction, and check the ratio:

```cpp
RC_CHECK_NEAR(drawn_width / drawn_height, 2.0, 0.2);
```

Twice as wide as tall in characters is what round looks like in a terminal. In a
window with square pixels the same check expects a ratio of one.
