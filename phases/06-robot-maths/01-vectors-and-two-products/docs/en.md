# Vectors: The Dot Product, the Cross Product, and Cross Track Error

> Two vectors have exactly two useful products. One tells you how much they agree, the other tells you which way one is turned from the other, and a robot following a path needs both.

**Type:** Build
**Time:** about 105 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 01-06

## The Problem

A robot is meant to drive along a line from A to B. It is not on the line.

To steer back it needs two numbers: how far off it is, and **which side** it is
on. Distance alone is useless, because turning left and turning right both reduce
a distance and only one of them is correct.

That signed distance is called **cross track error**, and it is the input to
every path following controller ever written. Computing it needs the two vector
products, and computing the angle it implies needs one of them used in a way that
is not the formula in most textbooks.

## The Concept

### A vector is a displacement, not a place

A `Vec2` holds an x and a y, and it means an arrow rather than a point: how far
across and how far up, with no opinion about where it starts. Subtracting two
positions gives the displacement between them, which is the operation almost
every formula below begins with.

### The dot product measures agreement

```
dot(a, b) = a.x * b.x + a.y * b.y
```

It is largest when the two point the same way, zero when they are perpendicular,
and negative when they point in opposing directions. That sign test alone is
worth the whole operation: **is this thing in front of me or behind me** is
`dot(heading, to_target) > 0`.

Geometrically it is the length of one vector multiplied by how much of the other
lies along it, which is why it is written as lengths and a cosine:

```
dot(a, b) = |a| |b| cos(angle)
```

### The cross product measures turning

In two dimensions the cross product is one number:

```
cross(a, b) = a.x * b.y - a.y * b.x
```

It is zero when the vectors are parallel, positive when b is turned
anticlockwise from a, and negative when it is turned clockwise. That sign is
exactly the which side question, and it is what makes cross track error signed.

Its magnitude is lengths and a sine:

```
cross(a, b) = |a| |b| sin(angle)
```

### The angle: use both, never acos alone

The textbook formula for the angle between two vectors is:

```cpp
const double angle = std::acos(dot(a, b) / (length(a) * length(b)));   // do not
```

It has three faults and a robot meets all of them.

It **loses precision near zero**. Cosine is flat where the angle is small, so a
tiny error in the dot product becomes a large error in the angle, which is
exactly the region a path follower spends its life in. This is not a small
effect. For two directions genuinely one hundred millionth of a radian apart,
measured on the machine this lesson was written on:

```text
true angle   1e-08
acos         0.000e+00      the angle is gone entirely
atan2        1.000e-08      correct
```

It **leaves the domain**. Rounding can make that ratio 1.0000000000000002, and
`acos` of anything above one is not a number. From there the NaN spreads into
every later calculation silently.

It **has no sign**. `acos` returns zero to pi, so it cannot tell left from right,
which is the one thing the robot needed.

The robust form uses both products and has none of those problems:

```cpp
const double angle = std::atan2(cross(a, b), dot(a, b));
```

`atan2` is well conditioned everywhere, cannot leave its domain, and returns a
signed angle from minus pi to pi, already in the range lesson 01-03 taught you to
keep angles in. Whenever you want an angle between two directions, this is the
expression.

### Cross track error

Now the whole thing. Given a segment from `a` to `b` and a robot at `p`:

```
along  = b - a          the direction of the path
offset = p - a          from the start of the path to the robot
error  = cross(along, offset) / |along|
```

The cross product gives the signed area of the parallelogram those two make, and
dividing by the base length turns an area into a height, which is the
perpendicular distance. The sign survives the division, so positive means one
side and negative the other.

Which side is which is a convention you must write down and never change. Here,
with x forward and y left, a **positive error means the robot is to the left of
the path**, and the controller steers right to reduce it.

A path segment of zero length has no direction, so there is no error to report.
That case has to be answered deliberately rather than by dividing by zero.

## Build It

Implement in `exercise/solution.hpp`:

- `add`, `subtract`, `scale`, and `length`.
- `dot(a, b)` and `cross(a, b)`.
- `normalized(v)`, the same direction with length one, and the zero vector
  unchanged when there is no direction to give.
- `angle_between(a, b)`, signed, using `atan2` of the cross and the dot.
- `cross_track_error(a, b, p)`, signed, positive to the left, and 0.0 when the
  segment has no length.

```
rcpp verify 06-01
```

## Use It

Eigen is what production code uses, and its `Vector2d` has all of this with
`a.dot(b)` and `a.norm()`. It also brings expression templates, which fuse a
chain of operations into one loop, and fixed size types the compiler can put in
registers.

Writing these six functions first is worth the hour because Eigen's names then
mean something. `norm` is the length you just wrote, and `normalized` is the one
that had to guard against zero, which Eigen does not do for you either.

Cross track error appears in phase 16 as the input to a path follower, and the
same signed distance is what a docking controller and a wall follower steer on.

## What Breaks First

- **An angle that is not a number.** `acos` was given a value slightly above one
  by rounding. Use `atan2` of the cross and the dot. See `E-NUM-0012`.
- **A normalized zero vector full of NaN.** Dividing by a length of zero. Guard
  it and decide what no direction means. See `E-NUM-0013`.
- **The robot steers the wrong way.** The sign convention was reversed, or the
  arguments to the cross product were swapped, which negates it. See
  `E-NUM-0004`.

## Ship It

`Vec2` and these operations become the foundation of `rc::math`. Everything in
phases 13 and 16 is built from them, and `cross_track_error` is the number the
path follower in phase 16 steers on directly.
