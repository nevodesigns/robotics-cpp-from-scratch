# The Same Path, in a Window

> You have drawn the robot in a terminal. Here it is in a window, and almost
> nothing had to change. Finding out how little is the lesson.

**Type:** Build
**Time:** about 90 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 00-07
**Needs Qt:** yes, and this lesson is optional

## Before you start

This is the only lesson in phase 00 that needs something installed.

```
rcpp doctor
```

If it says Qt is missing and you would rather keep going, **skip this lesson**.
Lesson 00-07 already drew your robot with the same arithmetic, and nothing later
in the curriculum depends on having done this one. Qt becomes genuinely required
at phase 09, by which point installing it is a reasonable thing to ask of
somebody who has built eight phases of work.

If you do have Qt, this is worth an hour, because it makes a point that is hard
to make any other way.

## The Problem

A terminal grid and a window are not obviously similar. One is sixty characters
by twenty, addressed by row and column, and prints. The other is four hundred
pixels by three hundred, addressed by floating point coordinates, and paints.

The natural assumption is that drawing the path in a window means writing the
drawing again.

It does not, and the reason is worth more than the window: **the arithmetic that
turns a trajectory into a picture does not know what a picture is made of.**

## The Concept

### What actually differs between two surfaces

Put a terminal and a window side by side and list what the mapping has to know:

| | terminal | window |
|---|---|---|
| how wide | 60 | 400 |
| how tall | 20 | 300 |
| shape of one cell | twice as tall as wide | square |
| where the path goes | fit, centre, flip | fit, centre, flip |

Three of those four are numbers you already pass in. The fourth is the same in
both.

So there is one difference, and it is the shape of a cell. A terminal character
is about twice as tall as it is wide, so a metre across costs twice as many
columns as a metre up costs rows. A pixel is square, so it costs the same.

Make that a parameter and one function serves both:

```cpp
Point place(pose, bounds, across, down, aspect, margin);
//                                     ^^^^^^ 2.0 for a terminal, 1.0 for a window
```

That is the whole lesson. When you meet a second surface and the answer is one
new argument, the first version was written at the right altitude.

### Across and down, not x and y

The mapping returns `across` and `down` rather than `x` and `y`, and the naming
is doing work.

`down` is the direction the number grows, which is the opposite of the robot's
`y`. Calling it `y` invites exactly the mistake the flip exists to prevent, and
this family of functions has enough coordinate systems in it already.

### One off by one, and why it is interesting

A character grid is indexed by cell. Sixty columns give you fifty nine steps
between the centre of the first and the centre of the last, so the terminal
version fits into `columns - 1`.

A window is continuous. Four hundred pixels wide gives four hundred units of
room, and no subtraction is needed.

The tests check that the general mapping reproduces the terminal one exactly
when it is handed the step count, and that handing it the cell count instead
gives an answer that is close but not equal. That is what this kind of off by
one looks like when it does not crash anything: a picture very slightly too big,
for years.

### A margin, which the terminal did not have

A window has edges you can see, and a path touching them looks like a mistake
even when it is not. So the mapping takes a margin, taken off both sides of both
directions, and the fit uses what is left.

The centring is the other half of it. Once the path is fitted, there is room
left over in one direction, and it belongs split between the two sides rather
than piled against one.

### The window itself

The window is supplied. You are not expected to write Qt yet; phase 09 is where
that begins.

It is still worth reading, for one reason: of the roughly forty lines in
`PathWindow`, the only line doing any thinking is the one that calls the
function you wrote. Everything else is choosing colours and asking Qt to repaint.
That ratio is what a good boundary between arithmetic and a user interface looks
like, and it is why the tests for the mapping need no window at all.

## Build It

Implement in `exercise/solution.hpp`:

- `surface_scale`, one scale for both directions, allowing for the margin, the
  cell shape, and an axis with no extent.
- `place`, the mapping, fitting, centring and flipping.

```
rcpp verify 00-08
```

Half the tests draw nothing at all: they check the arithmetic, including that it
agrees with lesson 00-07 when given the terminal's cell shape. The rest render
the widget with no window and no screen, which is how they run on a machine with
no display and in continuous integration.

## Use It

Run it and look at it. Then resize the window: the path refits, recentres and
stays the right shape, because the mapping is computed from the size every time
it paints rather than once at startup.

Phase 10 comes back to this and asks you to write the widget rather than being
given it. The arithmetic will already be done.

## What Breaks First

- **A circle drawn as an oval.** Each axis stretched to fill the window. In
  pixels the ratio to expect is one, not two. See `E-PLOT-0002`.
- **A picture that is upside down.** The flip is the same line here as in the
  terminal, and leaving it out looks just as plausible. See `E-PLOT-0001`.
- **Coordinates that are not numbers.** A path that has not moved yet is
  degenerate in every direction at once. See `E-PLOT-0003`.

## Ship It

The general mapping joins `rc::sim` beside the terminal one, which becomes a
call to it with the aspect of a character. Phase 00 now ends twice: once with a
picture that needed nothing installed, and once with a window, for whoever wants
it.
