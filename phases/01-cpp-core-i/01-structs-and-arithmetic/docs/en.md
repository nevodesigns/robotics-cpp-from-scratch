# Structs: Giving a Group of Numbers One Name

> Three loose numbers that always travel together are a type waiting to be named.

**Type:** Build
**Time:** about 75 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 00-04

## The Problem

In lesson 00-04 you passed a pose around as one value. Imagine you had not. Every
function would take three separate numbers:

```cpp
double distance(double x1, double y1, double theta1,
                double x2, double y2, double theta2);
```

Call that with the arguments in the wrong order and it still compiles. It just
returns a wrong answer forever. Robot code is full of triples and quadruples that
belong together, and keeping them loose is how position becomes velocity and
metres become millimetres in the same afternoon.

## The Concept

A **struct** groups values under one name and makes them one thing:

```cpp
struct Pose {
  double x = 0.0;
  double y = 0.0;
  double theta = 0.0;
};
```

Three things follow immediately.

**It is one value.** You can pass a `Pose` to a function, return one, and store
one in a container. It travels as a unit and cannot be split up by accident.

**The pieces have names.** `p.x` says what it is. There is no argument three to
count on your fingers.

**The defaults mean it starts sane.** Writing `= 0.0` after each member means
`Pose p;` is a real pose at the origin, not three pieces of leftover memory
holding whatever was there before. Uninitialised values are one of the genuine
dangers of this language, and giving members defaults removes the danger at the
point where the type is defined, once, for everybody.

### Distance between two poses

The distance between two points is the straight line between them, from
Pythagoras:

```
distance = sqrt((x2 - x1) squared + (y2 - y1) squared)
```

Notice that heading plays no part. Two robots at the same spot facing opposite
directions are zero metres apart. That is a decision about what the word distance
means here, and writing it as a named function is how that decision gets recorded
somewhere other than in your head.

## Build It

Open `exercise/solution.hpp`. Implement:

- `distance(const Pose& a, const Pose& b)`, the straight line distance in metres.
- `midpoint(const Pose& a, const Pose& b)`, the pose halfway between the two
  positions. Set its heading to zero, and the tests will tell you if you forget.
- `translate(const Pose& start, double forward)`, the pose you reach by driving
  `forward` metres along the direction `start` already faces. This one reuses the
  cosine and sine idea from lesson 00-04.

Then:

```
rcpp verify 01-01
```

## Use It

Every robotics library has this type. In ROS 2 it is `geometry_msgs::msg::Pose`,
carrying a position and an orientation as a quaternion rather than a single
angle, because it describes three dimensions rather than a flat floor. Eigen,
which arrives in phase 06, calls the same idea a transform.

They are all the same move you just made: give the group of numbers a name so the
compiler can help you keep them straight.

## What Breaks First

- **Your struct definition has no semicolon after the closing brace.** A struct
  ends with `};`, and the error appears on the line after it. See `E-CPP-0005`.
- **You changed a copy and expected the original to change.** Passing by value
  copies. Lesson 01-03 is about exactly this. See `E-CPP-0006`.
- **A distance test fails by a hair.** Compare fractional numbers with a
  tolerance. See `E-NUM-0003`.

## Ship It

`Pose` and these three helpers become the beginning of `rc::math`, the module
every later phase uses to talk about where things are.
