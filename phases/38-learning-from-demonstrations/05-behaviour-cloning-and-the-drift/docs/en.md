# Behaviour Cloning and the Drift: A Copy of the Expert That Is Good Where the Expert Went and Lost Where It Did Not

> A cloned policy is tested by driving with it, not by its error on the demonstrations, because its own small mistakes take it where nobody demonstrated anything.

**Type:** Build
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 38-04, 38-01, 26-04

## The Problem

Behaviour cloning is the plainest way to learn from demonstrations: treat every sample as a question (the state) with an answer (the expert's action), and train a network to give the answers. 38-04 showed a network can match the expert's actions closely on new demonstrations. That is not the test that matters. A policy is used by letting it drive, and when it drives, its own small mistakes decide what it sees next. The demonstrations showed what the expert did in the states the expert visits; the clone, once a little off, is in states the expert never visited, where it was never taught anything.

## The Concept

### A clone

A network of 16 units, trained on 3000 samples from a consistent expert whose episodes started within 0.2 m and 0.1 rad of the centre line. The network and the scale of its inputs are kept together as one Learner: the network means nothing without the scale it was trained with (38-01). On new demonstrations of the same kind its actions matched the expert's to an error of 0.000014. By the numbers of 38-04, a success.

### Driving with it

A rollout drives the car with a policy from a start and says whether it stayed in the lane, whether it came to rest on the centre line, how far out it went, and how many of its steps were in unseen states: more than three spreads from the demonstrations' mean on either input. From starts like the demonstrations', the clone succeeded on 100 percent, as the expert did (100 percent). See `E-LRN-0013`.

### The drift

From wider starts (within 0.6 m and 0.8 rad), the expert succeeded on 98 percent and the clone on 87 percent. The difference is not a poorly trained network. Near the edge of the lane, heading outward, the right action is a full turn, and the narrow demonstrations never showed one: the expert started near the centre and never needed it. The clone's turn there is whatever its tanh units happen to give, too gentle, and the car leaves the lane. Its failed runs spent 100 percent of their steps in unseen states; its successful ones 12 percent. See `E-LRN-0014`.

This is the general shape of the problem. Each small error of a cloned policy moves it a little away from the states the expert visited; there its errors are larger; those take it further. The error compounds with the length of the run, where a learner that was only judged on single answers would see none of it. See `E-LRN-0015`.

## Build It

`rollout` in `exercise/solution.hpp`.

Open `exercise/solution.hpp`. The part you write is marked `TODO`; the rest of the file is already written, and it is worth reading before you start, because your part is called by it or calls it.

What to write, as the note beside the `TODO` puts it:

- For each step: count the state as unseen if either input, scaled by `seen`, is more than 3 from zero; the turn is learner_act for a learner or lane_expert when learner is null; lane_step; count the step; keep the largest |offset| in worst; if |offset| is past t.edge (or not a number), set stayed false and return at once; after the last step, settled is |offset| < 0.05 and |heading| < 0.05.

As shipped, 2 of the 3 tests fail, and these are the ones your code has to turn green:

- Driving from starts like the demonstrations', the clone does as well as the expert.
- From wider starts the clone fails where the expert succeeds, and its failures are spent in states nobody demonstrated.

The other one passes already, because it tests the parts that are given.

```
rcpp verify 38-05
```

A failing check names the test and prints the number it expected beside the one it got, so start from the first failure and work down. When the suite is green, read `reference/solution.hpp` beside your own version: where the two differ and both pass, the difference is worth understanding.

## Use It

`rc/learn/cloning.hpp`.

```cpp
const rc::learn::Learner policy = rc::learn::clone(demos, 16, 200, 0.02, seed);
const double share = rc::learn::success_rate(task, &policy, rc::learn::fit_normaliser(demos), 0.6, 0.8, 100, seed, 300);
```

What this does not do. The lane is straight and the state has two numbers; on a real car the unseen states are uncountable, and the same drift happens with images. Success here is a simple rule (stay in, end on the line). The unseen count uses a box of three spreads round the mean, a crude test of what the network has seen; 38-07 makes it the basis of a guard. Training on wider demonstrations would help, but the expert's own episodes, however wide they start, still go where the expert goes; 38-06 asks the expert about the states the learner reaches instead.

## What Breaks First

- A clone tested only on the expert's states. See `E-LRN-0013`.
- A clone used where the expert never was. See `E-LRN-0014`.
- A cloned policy judged by its error on single answers. See `E-LRN-0015`.

## Ship It

`rc/learn/cloning.hpp` holds `Learner`, `learner_act`, `clone`, `Rollout`, `rollout` and `success_rate`.
