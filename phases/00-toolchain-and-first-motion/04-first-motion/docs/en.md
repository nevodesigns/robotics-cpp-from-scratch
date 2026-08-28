# First Motion: Driving a Robot in a Straight Line

> Two wheels, two numbers, and a small piece of trigonometry is the whole of how most robots know where they are.

**Type:** Build
**Time:** about 90 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** none, the robot is simulated
**Prerequisites:** 00-03

## The Problem

A robot with two driven wheels is the most common mobile robot on earth. Warehouse
robots, cleaning robots, teaching robots, and most competition robots are all this
shape. You give it two numbers, the speed of the left wheel and the speed of the
right wheel, and it moves.

The question this lesson answers is: given those two numbers, where is the robot
one moment later?

Answering it is the foundation of odometry, which is how a robot estimates its
own position without any external help. Every navigation system in this
curriculum sits on top of the function you are about to write.

## The Concept

A **pose** is where the robot is and which way it faces. Three numbers:

- `x` and `y`, its position in metres
- `theta`, the direction it is pointing, in radians, measured anticlockwise from
  the positive x axis

Radians rather than degrees, because every trigonometry function in the standard
library takes radians. A full turn is `2 * pi` radians, so a right angle is about
1.5708.

### From two wheel speeds to motion

Start with the two wheel speeds in metres per second, `vl` and `vr`. Two things
fall straight out of them.

**Forward speed** is the average of the two. If both wheels do 1.0, the robot
moves forward at 1.0. If one does 1.0 and the other does minus 1.0, the average
is zero and the robot does not go anywhere, it spins.

```
v = (vl + vr) / 2
```

**Turn rate** comes from the difference between them, divided by how far apart
the wheels are. That distance is the wheel base, and it matters: wheels close
together turn sharply for the same speed difference, wheels far apart turn
gently.

```
omega = (vr - vl) / wheel_base
```

### From motion to a new pose

Now step forward by a small slice of time, `dt`. The robot moves `v * dt` metres
in the direction it is facing, and turns by `omega * dt` radians.

Moving `d` metres in direction `theta` changes x by `d * cos(theta)` and y by
`d * sin(theta)`. That is the only trigonometry in this lesson, and it is worth
holding on to: cosine gives you the x part of a direction, sine gives you the y
part.

```
x     = x + v * dt * cos(theta)
y     = y + v * dt * sin(theta)
theta = theta + omega * dt
```

This is an approximation. It pretends the robot travels in a straight line during
each slice, when it really travels along a slight arc. With small enough slices
the error is tiny, and every real robot in the world uses exactly this. Lesson
13 replaces it with the exact arc version and measures the difference.

### Angles need to be kept in range

Add up enough turns and theta grows without limit: 7.0 radians, 13.0, 400.0. It
still points the right way, but nothing downstream can compare two headings
sensibly any more. So after every step, wrap the angle back into the range from
minus pi to pi. Getting this wrong is one of the most common bugs in robot code,
and lesson 01-03 is entirely about it.

## Build It

Open `exercise/solution.hpp`. It gives you the `Pose` type and an empty `step`
function.

Implement `step(const Pose& start, double vl, double vr, double wheel_base, double dt)`,
returning the new pose. Then:

```
rcpp verify 00-04
```

When the tests pass, run the test binary directly. It prints the path your robot
drove, as a map:

```
ctest --test-dir build/default -R 00-04 --verbose
```

That is your robot moving, driven by arithmetic you wrote.

## Use It

Real robots do not know their wheel speeds exactly. They read encoders, which
count wheel rotations, and convert those counts into distance. They also drift:
wheels slip, the wheel base is never exactly what the drawing says, and errors
add up over minutes until the robot believes it is in a different room.

That is why odometry is always combined with something else, an inertial sensor
or a map or a beacon. Phase 16 builds that combination. The model you wrote here
does not become wrong, it becomes one input among several.

## What Breaks First

- **The robot moves in the wrong direction, or sideways.** You swapped cosine and
  sine, or wrote degrees where radians were expected. See `E-NUM-0004`.
- **Theta grows forever.** You did not wrap the angle back into range. See
  `E-CPP-0004`.
- **A test fails by a tiny amount.** You compared fractional numbers exactly
  instead of within a tolerance. See `E-NUM-0003`.

## Ship It

`step` becomes the first function in `rc::sim`, the simulated robot every later
phase drives. In phase 09 you will put a Qt window around this exact function and
watch the path draw itself. In phase 14 you will wrap a controller around it and
make it follow a target. Nothing gets thrown away.
