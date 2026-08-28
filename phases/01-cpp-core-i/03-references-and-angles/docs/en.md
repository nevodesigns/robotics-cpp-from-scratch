# References: Copies, Aliases, and the Angle Bug

> A robot facing 179 degrees, asked to face minus 179 degrees, should turn two degrees, not 358.

**Type:** Build
**Time:** about 90 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 01-02

## The Problem

Two problems, and they are the two that show up in almost every robot code review.

The first is that a robot asked to turn from 179 degrees to minus 179 degrees
turns the long way round, almost a full circle, when the real answer is two
degrees. Every heading controller has this bug once.

The second is that a function that was supposed to change something did not. It
changed a copy, and the copy was thrown away when the function returned. This
one silently produces a robot that ignores commands.

Both come from a single question you have to be able to answer instantly: when
you pass something to a function, does the function get the thing, or a photocopy
of the thing?

## The Concept

### Copies and aliases

By default, C++ passes a copy:

```cpp
void reset(Pose p) { p.x = 0.0; }   // changes the copy, the caller sees nothing
```

Adding `&` makes the parameter an **alias** for the caller's object. There is no
copy, and changes are visible outside:

```cpp
void reset(Pose& p) { p.x = 0.0; }  // changes the caller's pose
```

Adding `const` as well makes an alias that cannot be modified:

```cpp
double distance(const Pose& a, const Pose& b);   // no copy, and cannot change them
```

The rule used throughout this curriculum:

- **Small values you only read**, like a `double`, pass by value. Copying eight
  bytes costs nothing.
- **Larger values you only read**, like a struct, pass by `const&`. No copy, and
  the compiler enforces that you do not change it.
- **Anything you intend to modify**, pass by plain `&`, and name the function so
  the reader expects it.

That last part matters more than it looks. A function that modifies through a
reference should be obvious from its name. `normalize_in_place(pose)` is honest.
`check(pose)` that quietly rewrites the pose is a trap.

### Angles wrap, and subtraction does not know that

An angle of 3.2 radians and an angle of minus 3.08 radians point almost the same
way, because a full turn is about 6.28 radians. Subtract them and you get 6.28,
suggesting they are a full turn apart, which is both true and useless.

What a controller actually needs is the **shortest** turn: how far to rotate, in
which direction, to get from one heading to the other, never more than half a
turn in either direction.

The reliable way to compute it is to subtract, then wrap the result back into the
range minus pi to pi:

```cpp
double shortest_turn(double from, double to) {
  return wrap_angle(to - from);
}
```

And the wrap itself, which you met in lesson 00-04:

```cpp
double wrap_angle(double radians) {
  return std::atan2(std::sin(radians), std::cos(radians));
}
```

Rebuilding the angle from its own sine and cosine works because those two values
repeat exactly once per turn, so the answer can only come back in one range. It
costs two trigonometry calls. There is a cheaper version using `std::fmod`, and
you will write it in the exercise, but this one is impossible to get subtly
wrong, which on a first pass is worth more than the speed.

## Build It

In `exercise/solution.hpp`:

- `wrap_angle(double radians)` brings any angle into the range minus pi to pi.
  Write it with `std::fmod` rather than trigonometry, and mind the sign: `fmod`
  keeps the sign of its left operand.
- `shortest_turn(double from, double to)` returns the signed shortest rotation.
  Positive means anticlockwise.
- `steer_towards(Pose& pose, double target_heading, double max_turn)` rotates the
  pose towards the target by at most max_turn, modifying it in place. Reuse
  `rate_limit` thinking from the last lesson, but on the wrapped difference.

```
rcpp verify 01-03
```

## Use It

`tf2` in ROS 2 and every serious robotics library handle this with quaternions
rather than raw angles, precisely because wrapping and its cousin, gimbal lock,
cause so much trouble in three dimensions. Phase 06 builds quaternions from
nothing and shows what they fix.

On a flat floor with one angle, the wrapped difference you just wrote is still
the correct and complete answer, and it is what runs inside most two dimensional
navigation stacks.

## What Breaks First

- **Your function changed nothing.** The parameter was a copy. Add `&`. See
  `E-CPP-0006`.
- **The robot turns the long way round.** You subtracted headings without
  wrapping the result. See `E-NUM-0006`.
- **You returned a reference to a local variable.** The local dies when the
  function returns, and the reference points at nothing. See `E-CPP-0008`.

## Ship It

`wrap_angle` and `shortest_turn` join `rc::math` and are used by every heading
controller, every navigation goal, and every pose comparison for the rest of the
curriculum. They are ten lines that never stop earning.
