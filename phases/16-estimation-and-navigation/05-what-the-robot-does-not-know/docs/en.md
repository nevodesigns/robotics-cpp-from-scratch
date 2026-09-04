# What the Robot Does Not Know: A Map With Three Answers

> Forty four observations, which is under two seconds of a 30 Hz scanner, and
> the cell can never be corrected again.

**Type:** Build
**Time:** about 180 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 16-04, 01-04

## The Problem

The robot knows where it is and can follow a path. It has no idea what is
around it.

A map is the obvious next thing, and the obvious map is a grid of booleans:
blocked or not. That representation is wrong in two directions at once, and both
of them put a robot somewhere it should not be.

**It has no way to say it does not know.** Most of the world, most of the time,
has not been looked at.

**It has no way to be corrected in proportion.** Evidence accumulates, and if
withdrawing it costs as much as building it, then a van that parks for a minute
leaves a wall for a minute after it goes. Represented carelessly, it leaves one
for ever.

## The Concept

### A cell nobody has looked at is not free

A grid with two answers has to call unlooked-at ground something. Initialised to
free, the planner routes through a stairwell it has never seen. Initialised to
occupied, the robot will not leave its starting square, so somebody changes it
to free, and the first problem is back.

The absence of evidence is a third state, and it is the state most of the map is
in:

```cpp
enum class Cell { unknown, free, occupied };
```

Not a flag with a third value bolted on, though. Hold **evidence**, and let the
three answers fall out of it. A cell at zero has been told nothing. A cell needs
enough observations to pass a threshold before the map will commit, which makes
"one reading from a sensor right seven times in ten" the non-answer it is. And
evidence that cancels returns to not knowing, rather than to whichever reading
happened to arrive last.

What unknown then means to a caller is a decision to make once, out loud:

- a **planner** treats it as blocked or as expensive, and says which;
- a **safety check** treats it as occupied, always;
- a **display** gives it its own colour, because an operator who cannot tell
  unseen from empty cannot tell confident from blind.

### A map that can be told it was wrong

Here is the argument for log odds, and it is not about speed.

Update a probability by multiplying observations in, with a sensor right seven
times in ten:

```cpp
p = (p * l) / (p * l + (1 - p) * (1 - l));
```

| observations | 1 - p | misses to recover |
|---|---|---|
| 1 | 3.000e-01 | 1 |
| 10 | 2.090e-04 | 10 |
| 40 | 1.887e-15 | 41 |
| 43 | 1.110e-16 | 44 |
| 44 | **0.000e+00** | **never** |
| 2000 | 0.000e+00 | never |

At forty four consistent observations, `1 - p` falls below what a double can
hold beside 1.0. `p` becomes exactly one, every later update multiplies zero by
something, and the cell is not merely confident but unreachable.

Forty four observations is under two seconds of a 30 Hz scanner.

In log odds an observation is an addition, so nothing saturates. But addition is
symmetric, so two hundred scans of a parked van still take two hundred scans to
undo, and the robot spends that minute routing around a memory.

The fix for both is one line:

```cpp
cell += occupied ? kHit : kMiss;
if (cell > kClamp) cell = kClamp;
if (cell < -kClamp) cell = -kClamp;
```

**The clamp bounds how sure a cell may become, so it bounds how long it takes to
change its mind.** Measured with the clamp at plus and minus four, a cell
recovers in five contrary observations whether it was confirmed ten times or two
thousand, and the van's wall came down six scans after it drove away.

Choose the clamp from how fast your world changes, not from how good your sensor
is. The sensor's reliability is already in the per-observation step. The clamp
is a statement about the world, and it should have a sentence beside it saying
which world.

### What a beam actually tells you

One range reading is not one fact, it is a line of them: everything the beam
passed through is empty, and where it stopped is not.

Two mistakes live at the two ends of that line.

**Marking the whole ray free, endpoint included**, erases the obstacle the
reading just found. The scan detects the wall and the same pass deletes it.

**Marking the endpoint occupied on a max-range return** invents an obstacle that
was never detected. A beam that ran out without hitting anything says the space
was empty and says nothing about its far end. Mark it and you paint a ring of
phantom wall at exactly max range around every place the robot has ever stood.

So whether the beam stopped on something is part of the reading, and has to be
passed in with it:

```cpp
grid.integrate_ray(from_x, from_y, to_x, to_y, /*hit=*/true);
```

### Casting is not flooring

The conversion from a world coordinate to a cell index has a trap that a corner
origin cannot show.

```cpp
const int cx = static_cast<int>((x - origin_x) / resolution);              // wrong
const int cx = static_cast<int>(std::floor((x - origin_x) / resolution));  // right
```

A cast truncates toward zero, which differs from flooring for every negative
value. With 10 cm cells and the origin at the centre of the map:

| x | floored | truncated |
|---|---|---|
| -0.25 | -3 | -2 |
| -0.15 | -2 | -1 |
| -0.05 | -1 | 0 |
| 0.05 | 0 | 0 |
| 0.15 | 1 | 1 |
| 0.25 | 2 | 2 |

Every negative cell is shifted by one, and `-0.05` and `+0.05` land in the same
cell, so **the cell at the origin is 20 cm wide and every other one is 10**.
There is a seam down the middle of the world, and a wall mapped driving east
lands in a different place from the same wall mapped driving west.

It hides because the first test map puts the origin at a corner, where every
coordinate is positive and truncation and flooring agree exactly.

Report whether a point is inside the grid rather than clamping the index. A
clamped index piles everything past the edge into the border cells, which builds
a wall around the map that nobody put there.

## Build It

Implement `cell_for`, `classify` and `observe` in `exercise/solution.hpp`. The
ray walk is written for you, because Bresenham is not the lesson; what it does
with the endpoint is.

```
rcpp verify 16-05
```

The suite converts points on both sides of the origin, asks the map about ground
it has never seen, contradicts a cell two ways, and parks a van in front of the
robot for two hundred scans before driving it away.

## Use It

**Write the clamp down with a reason.** It is the map's memory in seconds, and
five contrary observations at your scan rate is a number you can quote.

**Decide what unknown means at each caller**, and never let the default be free.

**Test with the origin in the middle.** Every coordinate bug in this lesson is
invisible when all the coordinates are positive.

**Keep the resolution honest.** A 5 cm grid of a 50 m building is a million
cells, which is fine, and a 1 cm grid of the same building is twenty five
million, which is not fine at 30 Hz. Choose it from the smallest thing you must
not hit, and inflate obstacles for the robot's own width rather than shrinking
the cells.

## What Breaks First

- **A map that cannot be told it was wrong.** See `E-NAV-0011`.
- **Unknown ground treated as free.** See `E-NAV-0012`.
- **A cast instead of a floor.** See `E-NAV-0013`.

## Ship It

`OccupancyGrid` joins `rc::nav`. The rover now has a pose it can defend, a path
it can follow, and somewhere to put what it has seen, which is the last of the
three things navigating a map needs.
