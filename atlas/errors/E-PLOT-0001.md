id: E-PLOT-0001
title: A plot that comes out upside down
match: a point higher up the field is drawn nearer the top
platforms: linux, windows
teaches: 00-07-seeing-it-move
---

## Symptom

The picture looks entirely reasonable and the robot is doing the opposite of
what it should. A path that curves left is drawn curving right. A robot climbing
away from the start appears to be descending toward it.

Nothing looks broken, which is why this survives so long. A mirror image of a
plausible path is another plausible path.

## Cause

Two coordinate systems disagree about which way is up, and nothing in the code
says so.

A robot's y grows **upward**: north, away from the ground, the direction the
maths in every textbook assumes. A screen's rows grow **downward**: row 0 is at
the top, because text is written from the top of a page.

So the obvious mapping is wrong:

```cpp
const int row = static_cast<int>((y - min_y) * scale);   // the bug
```

That puts the lowest point of the path at row 0, which is the top of the
picture.

## Fix

Turn the distance above the bottom of the path into a distance below the top of
the grid:

```cpp
const double from_bottom = (y - bounds.min_y) * scale;
return (rows - 1) - static_cast<int>(from_bottom + 0.5);
```

`rows - 1` rather than `rows`, because the last row is one less than the count,
and getting that wrong writes one past the end of the grid.

Test it with a direction rather than with a picture, because a picture of a
mirror image looks fine:

```cpp
RC_CHECK(row_for(1.0, bounds, scale, rows) < row_for(0.0, bounds, scale, rows));
```

A point higher up the field must have a smaller row number. One assertion, and
it cannot be satisfied by an upside down plot.

The same flip appears again in phase 10 when the path is drawn in a window
rather than in a terminal, for exactly the same reason.
