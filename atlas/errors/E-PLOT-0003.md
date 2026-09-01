id: E-PLOT-0003
title: A plot that comes out blank
match: a path with no height still produces a picture
match: a single point does not divide by zero
platforms: linux, windows
teaches: 00-07-seeing-it-move
---

## Symptom

The plot is empty. Not partly wrong: nothing is drawn at all, and no error is
reported, because as far as the program is concerned everything succeeded.

It usually appears the first time something drives in a straight line, having
worked perfectly on every curved path.

## Cause

A path along one axis has **no extent on the other**, and the scale is computed
by dividing by that extent:

```cpp
const double scale = usable_rows / height;   // height is 0.0
```

Dividing a positive number by zero gives infinity rather than an error. Every
coordinate then becomes infinity, converting infinity to an integer is
undefined, and the values that come out are outside the grid, so every point is
skipped by the bounds check that was there to keep the drawing safe.

The bounds check does its job perfectly and the picture is blank.

Which case actually needs the guard is narrower than it looks, and worth
knowing, because the obvious test does not catch it.

Taking the **smaller** of the two candidate scales is already a partial guard.
An axis with no extent produces infinity, and infinity is never the smaller of
two numbers, so a path that is straight along one axis is saved by the other
axis without any guard at all. Remove the guard and that test still passes.

What defeats it is a path where **every** axis is degenerate: a single point, or
any path that has not moved yet, which is every path on its first frame. Then
both candidates are infinity, the smaller of them is infinity, and every
coordinate follows.

So the test that catches this is the single point, not the straight line. Both
are worth having, and only one of them fails when the guard is missing.

## Fix

Decide what a degenerate axis means before dividing by it. A path with no height
does not constrain the vertical scale at all, so the other axis should decide:

```cpp
const double by_height = height > 1e-12 ? usable_rows / height : usable_rows;
const double by_width  = width  > 1e-12 ? usable_columns / (width * 2.0)
                                        : usable_columns;
return by_height < by_width ? by_height : by_width;
```

Compare against a small number rather than against zero exactly. A path that is
straight to within a nanometre has a height that is not zero and is close enough
to produce a scale of ten billion, which fails in exactly the same way and is
harder to reproduce.

Then keep the range check on the drawing as well. It is not redundant: it is
what stops a coordinate that is merely large from writing outside the grid, and
`E-CPP-0007` is what happens without it.

Worth testing both degenerate shapes explicitly, since neither appears in a
normal path and both appear on day one of using the plot for real:

```cpp
RC_CHECK(std::isfinite(scale_to_fit(single_point_bounds, columns, rows)));   // this one
RC_CHECK(std::isfinite(scale_to_fit(straight_line_bounds, columns, rows)));  // and this
```

The first is the one that fails without the guard. The second passes either way,
and is worth keeping because it is the case a reader expects to be the dangerous
one, and a comment saying it is not is cheaper than someone rediscovering it.
