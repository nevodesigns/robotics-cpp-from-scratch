# Which Door Am I At: A Belief Over Places, and Why It Must Never Say Never

> A Kalman filter says where the robot is, to within a spread. This one says where it might be, and it is allowed to say two places.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 16-02, 16-13

## The Problem

Lesson 16-02 combined two estimates that disagreed, with one number for the answer and one for its spread. That fits a robot that knows roughly where it is and wants to know more exactly. It does not fit a robot that wakes up in a corridor of identical doors and has no idea which.

The honest answer there is a list: here with this probability, there with that. A one number estimate cannot be at two places. A **belief** over places can, and it needs nothing more than a number for each place.

## The Concept

### A number for each place

The corridor is a ring of 24 places, each a door or a wall, and the belief is 24 numbers that sum to 1. At the start the robot knows nothing and every place gets 1/24, an entropy of 4.58 bits. The **entropy** is how unsure it is: zero when one place has everything, log2 of the number of places when it is spread evenly.

The numbers must sum to 1, and a belief that has nothing left in it, every place ruled out at once, goes back to knowing nothing and not to a division by zero. See `E-NAV-0038`.

### Moving spreads it

The robot moves a place on, but wheels slip. Say it manages exactly what it meant with probability 0.8, one place short with 0.1 and one place too far with 0.1. All of the belief at place 5, moved one place:

```
place 5 0.10, place 6 0.80, place 7 0.10; total 1.00
```

Each place's belief is moved on by the three amounts, scaled by their probabilities, and the ring wraps, so that a belief at place 23 moved one place is at place 0. Moving always spreads the belief out. See `E-NAV-0040`.

### Sensing sharpens it

The robot reads a door or a wall. Each place whose map entry matches the reading becomes more believable and each place that does not becomes less:

```cpp
p[i] *= (world[i] == reading) ? p_match : p_mismatch;   // then normalise
```

One reading of a door, from a sensor right nine times in ten, in a corridor with 9 doors:

| | belief | predicted |
|---|---|---|
| each door | 0.0938 | 0.0938 |
| each wall | 0.0104 | 0.0104 |

Nine places share the top, and the entropy fell from 4.58 to 3.91 bits. This is a belief that is at nine places at once, which no single number and spread can say. The prediction is the sensor's reliability worked out by hand, 0.9 over 0.9 times 9 plus 0.1 times 15.

### Walking and sensing

Each step moves, which widens the belief, and reads, which narrows it. Starting nowhere, at place 3, steps of one place:

| step | reads | most likely | its share | entropy (bits) |
|---|---|---|---|---|
| 0 | door | 0 | 0.094 | 3.91 |
| 4 | door | 22 | 0.286 | 2.99 |
| 7 | door | 10 | 0.611 | 2.09 |
| 12 | door | 15 | 0.794 | 1.41 |
| 14 | door | 17 | 0.799 | 1.17 |

At step 14 it was at place 17, which was the true one. The entropy did not fall every step: at step 2 it went from 3.75 up to 3.83 and at step 8 from 2.09 to 2.13, because a move spreads the belief and a wall reading can say little. Over many steps the readings win. See `E-NAV-0040`.

### How a sensor is described

The belief is only as good as the account it is given of the sensor. Two hundred trips of 60 steps, with a sensor that was in fact wrong one reading in twenty and wheels that slipped one place in five, and the belief told different probabilities for a wrong reading:

| belief says a wrong reading has probability | found the right place at the end |
|---|---|
| 0.00 | 0.625 |
| 0.02 | 0.685 |
| 0.05 | 0.710 |
| 0.10 | 0.715 |
| 0.30 | 0.620 |
| 0.50 | 0.050 |

The best is about the truth, 0.05 to 0.10. At 0.5 the sensor says nothing, and the result is 0.050, which is a guess among 24. Too confident and too sceptical are both worse than right. See `E-NAV-0039`.

### Never say never

Look at what a probability of zero does. Localised after 25 steps with exact moves, then one wrong reading:

| belief says a wrong reading has probability | share of the true place just after | steps to get back above a half |
|---|---|---|
| 0.00 | 0.042 | 5 |
| 0.01 | 1.000 | 1 |
| 0.10 | 1.000 | 1 |

A belief told that the sensor never errs multiplies the true place by zero on its first wrong reading. Nothing survives, the belief resets to knowing nothing (0.042 is 1/24), and the robot has to find itself again from scratch, 5 steps here and many more in a bigger world. One that allows a wrong reading
a 1 percent chance, or 10, barely noticed. A probability of exactly zero is a statement that something is impossible, and the world keeps proving otherwise. See `E-NAV-0039`.

## Build It

`normalise`, `predict`, `correct`, `most_likely` and `entropy` in `exercise/solution.hpp`. The constructor, `at`, `cells`, `peak` and `size` are given.

```
rcpp verify 16-14
```

Every random choice in the tests comes from a generator seeded in the test, so the figures above are the same on every run.

## Use It

`rc/nav/belief.hpp` joins `rc::nav`.

```cpp
Belief where(corridor.size());
// every step:
where.predict(1, 0.8, 0.1, 0.1);
where.correct(corridor, door_sensor_reading, 0.95, 0.05);
if (where.peak() > 0.9) trust(where.most_likely());
```

**What this does not do.** It is a one dimensional ring, and the number of places grows with the world: a real map in two dimensions with heading is millions of numbers, which is why a particle filter keeps a few thousand samples instead. It assumes the robot moves the same number of places each
step, and a different slip model for a different speed is the caller's work. It treats each reading as independent of the last, which a sensor stuck on one answer is not (lesson 15-04). And it knows only the map it is given: the room changing is a later lesson's problem in this phase.

## What Breaks First

- **Numbers that no longer sum to one.** Every share is wrong by the same factor, and an empty belief divides by zero. See `E-NAV-0038`.
- **A sensor described as perfect.** One wrong reading and the belief is wiped out. See `E-NAV-0039`.
- **Moving left out.** The belief gets sharper than the robot's wheels deserve. See `E-NAV-0040`.

## Ship It

`rc/nav/belief.hpp` holds `Belief`.

The robot can now say where it might be, in more than one place, and narrow it by walking.
