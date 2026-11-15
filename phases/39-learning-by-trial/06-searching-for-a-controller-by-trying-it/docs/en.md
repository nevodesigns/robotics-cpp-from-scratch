# Searching for a Controller by Trying It: A Pole on a Cart, Four Gains, and Random Changes Kept When They Help

> When the state is continuous and the controller has a few numbers, the plainest learning is to change the numbers at random and keep what balances longer.

**Type:** Build
**Time:** about 90 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 39-03, 35-05, 14-04

## The Problem

A table of values (39-03) needs a cell for every state. A robot's state is continuous, the angle of a pole, the speed of a cart, and a table cannot hold it. The other way to learn by trial is to learn the controller directly: give it a few numbers, try it, and change the numbers toward what did better. The plainest version of that, random search, is also one of the most honest tests of a learning problem: if changing the numbers at random and keeping the better ones works, the problem is easy; if it does not, something cleverer is needed. This lesson balances a pole on a cart with it, the task 35-05 balanced a body on legs with a hand-designed controller.

## The Concept

### The pole on a cart

The classic benchmark of Barto, Sutton and Anderson (1983): a cart of 1 kg on a track, a pole of 0.1 kg hinged on top, a push of at most 10 N either way every 0.02 s. A run fails when the cart leaves the track or the pole leans past 12 degrees. Balanced exactly upright with no push, it stays; tipped a little, it falls in well under 100 steps. The equations are the original ones, stepped by Euler's method.

### A controller of four numbers

The push is a weighted sum of the cart's position and speed and the pole's angle and rate: four gains, the shape of the state feedback of phase 14. With every gain zero the pole balanced for 36.9 steps on average from small random starts: it falls. The question is which four numbers balance it, and the learner is not told the equations.

### Random search

From zero, each try adds a random amount within plus or minus 2.0 to every gain, runs the result from 20 starts, and keeps the change if the mean number of steps balanced (out of 500) is higher. The best score was 64.0 after 10 tries, 207.7 after 50 and 500.0 after 300; the gains were -0.83, 4.97, 9.06 and 19.94. From 100 starts it had never seen they balanced for 497.9 steps on average, so it learned the task, not the starts. See `E-LRN-0040`.

The price was 6000 trial runs, every one of which, on a real cart, is a pole that may fall. A controls engineer with the equations would have designed the gains in one sitting (phase 14). Learning by trial is for when the equations are not known, or not to be trusted; when they are, use them, and use learning to check or to tune. See `E-LRN-0041` and `E-LRN-0042`.

## Build It

`random_search` in `exercise/solution.hpp`.

Open `exercise/solution.hpp`. The part you write is marked `TODO`; the rest of the file is already written, and it is worth reading before you start, because your part is called by it or calls it.

What to write, as the note beside the `TODO` puts it:

- For each try: new gains T[j] = K[j] + step (2 uniform(r) - 1), in order j = 0..3; score them with balance_score (n starts, score_seed, limit); if higher than the best so far, keep them in K and their score as the best; push the best so far onto history (if not null).

As shipped, 2 of the 3 tests fail, and these are the ones your code has to turn green:

- Random search finds gains that balance the pole, from gains of zero that let it fall.
- The gains it found balance from starts the search never saw.

The other one passes already, because it tests the parts that are given.

```
rcpp verify 39-06
```

A failing check names the test and prints the number it expected beside the one it got, so start from the first failure and work down. When the suite is green, read `reference/solution.hpp` beside your own version: where the two differ and both pass, the difference is worth understanding.

## Use It

`rc/learn/cartpole_search.hpp`.

```cpp
double gains[4];
rc::learn::random_search(model, 300, 2.0, 20, score_seed, seed, 500, gains, nullptr);
const double held_out = rc::learn::balance_score(model, gains, 100, other_seed, 500);
```

What this does not do. Four gains is a small search; a controller with thousands of numbers (a network) needs smarter steps than random ones, which is what the gradient of 39-07 gives. The score is the mean over twenty starts, a noisy measure, so a lucky try can be kept; scoring on fixed starts (one seed) makes the comparison fair between tries and risks fitting those starts, which the held-out test checks. Balancing for 500 steps (10 s) is the benchmark's measure, not a robot's; a real controller is judged on its disturbance rejection and its margins as well.

## What Breaks First

- Gains scored on the starts they were searched on. See `E-LRN-0040`.
- Learning by trial where the equations were known. See `E-LRN-0041`.
- A search step that is too large or too small. See `E-LRN-0042`.

## Ship It

`rc/learn/cartpole_search.hpp` holds `CartModel`, `CartState`, `cart_step`, `control_force`, `balance_steps`, `random_start`, `balance_score` and `random_search`.
