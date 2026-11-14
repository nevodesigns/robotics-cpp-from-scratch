# The Base and the Arm as One: A Jacobian With Eight Columns, a Weight on Moving the Base, and a Line Drawn Farther Than the Arm Alone Can Reach

> Give the hand a velocity and let the base and the arm share it, at a price for each.

**Type:** Build
**Time:** about 110 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 33-03, 28-03, 28-05, 28-06

## The Problem

33-03 parked the base, and then the arm worked from a standstill. Many jobs are not like that: wiping a long bench, laying a bead along a wall, following a conveyor, going through a door with the arm out. The hand has to travel farther than the arm alone can reach, along a straight line, and the base has to move while the arm works. The answer is to treat the base and the arm as one mechanism with more joints than the hand has directions.

## The Concept

### Eight columns

A hand has six velocities (three linear, three angular). The arm has six joints. The base adds two more rates: driving along its heading and turning about its centre. A differential drive base cannot slide sideways (the rule of 23-01), which is why there are two and not three. The whole-body Jacobian has eight columns: the base driving moves the hand along the heading, the base turning sweeps the hand round the base's centre (and turns it), and the other six are the arm's Jacobian (28-03) with the arm's base in the world. The test checks each column against a small move of the base or a joint. See `E-MOB-0010`.

### Eight unknowns, six equations

With eight rates and six wanted velocities there is a family of answers, as in 28-07's redundant arm. The one chosen is the one of least cost, where each rate has a price: the arm's joints cost one each and the base's two rates cost w_base each. The weighted damped least squares is the same formula as 28-05's with the weights: the rates are W inverse J transposed (J W inverse J transposed + lambda squared I) inverse times the velocity. A dearer base moves less and the arm more, a cheaper base the other way. See `E-MOB-0011`.

### A line farther than the arm can reach

A line of 1.5 m was drawn at 0.15 m/s with the hand held in its starting orientation. The arm alone ended 1.18 m short: it stretched out to its full length (its manipulability went to 0.0000, a singularity, 28-06) and could go no farther. Arm and base together reached the end to 0.000 m, the base driving 1.43 m and the arm's manipulability never below 0.0316: the base took up the distance and the arm stayed in the middle of its range, away from its singularity. See `E-MOB-0012`.

### The price on the base

With the base 30 times as costly to move it drove 1.20 m, and with it made 33 times cheaper 1.50 m. The difference is not large, because the base has to cover the distance the arm cannot, however dear: the weight shifts the share and not the total. A very dear base would leave the arm to do as much as it can and approach its singularity first.

## Build It

`wb_rates` in `exercise/solution.hpp`.

Open `exercise/solution.hpp`. The part you write is marked `TODO`; the rest of the file is already written, and it is worth reading before you start, because your part is called by it or calls it.

What to write, as the note beside the `TODO` puts it:

- With winv the inverse costs (1 / w_base for the two base rates, 1 for the joints), form A = J winv J^T + lambda^2 I (6 by 6), solve A y = xi with rc::arm::solve6, and the rates are winv times J^T y.

As shipped, 2 of the 3 tests fail, and these are the ones your code has to turn green:

- The rates give the hand its velocity, and a dearer base moves less.
- A line farther than the arm can reach is drawn by the arm and base together and not by the arm alone.

The other one passes already, because it tests the parts that are given.

```
rcpp verify 33-04
```

A failing check names the test and prints the number it expected beside the one it got, so start from the first failure and work down. When the suite is green, read `reference/solution.hpp` beside your own version: where the two differ and both pass, the difference is worth understanding.

## Use It

`rc/mobile/whole_body.hpp`.

```cpp
const rc::mobile::WbJacobian j = rc::mobile::wb_jacobian(m, base, q);
const std::vector<double> r = rc::mobile::wb_rates(j, wanted_hand_velocity, 1.0, 0.05);
```

**What this does not do.** The base is a kinematic point with a heading that turns as fast as asked and drives as fast as asked: its acceleration, slip and the wheel speed limits (23) are not here, nor the joint speed limits of the arm (29-08). The hand's line is free of obstacles (30 is not in the loop). The cost weights are constants, and the base's cost does not rise as it nears a wall. The error of the base's position (25) is zero: the base is exactly where its own rates put it, which is the topic of 33-07. A turn of the base moves the hand by the lever (33-01) and the test only draws a line along the heading's direction.

## What Breaks First

- **A base's rate taken as free to use.** See `E-MOB-0011`.
- **An arm left to do the whole line.** See `E-MOB-0012`.
- **A turn of the base that was not counted in the hand's velocity.** See `E-MOB-0010`.

## Ship It

`rc/mobile/whole_body.hpp` holds `WbJacobian`, `wb_jacobian`, `wb_rates`, `WbRun` and `run_line`.
