# A Policy Gradient: Trying Noisy Actions, Weighing Each by What It Earned, and What a Baseline Does to the Noise

> A policy gradient learns which way to change a controller from the noise it adds to its own actions, and subtracting the average return makes that noise far less loud.

**Type:** Build
**Time:** about 90 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 39-06, 38-03, 14-04

## The Problem

Random search (39-06) changes a controller's numbers blindly and keeps what helps. It works for four numbers; for a network of thousands it would never finish. A policy gradient tells the learner which way to change every number at once, from nothing but runs and their returns: no model, no expert, no derivative of the task. It is the idea under most of the learning that trains robot controllers today, and its weakness is noise, which this lesson measures and halves.

## The Concept

### A task with a known answer

A state x starts within plus or minus 1 and is moved by x next = x + u for ten steps; each step costs x next squared plus 0.1 times u squared, a cost on the state and a smaller one on the effort. The controller is u = -k x. Scanning k gives the best gain, 0.916, with a mean return of -0.0302 (k = 0 gets -3.2990). A real task has no scan to check against; this one does, so the learner can be judged.

### A noisy policy and its score

The learner's action is -k x plus normal noise of spread 0.3: it tries actions a little different from its own. For each action, the log of its chance changes with k by ((u - mean) / sigma squared) times (-x); summed over a run, that is the run's score. A run whose noise happened to help (its return was high) has a score that points the way k should move to make such actions likelier.

### The estimate

The gradient of the expected return with respect to k is estimated as the mean over a batch of runs of score times return. The policy moves k by a learning rate times it. From k = 0, 300 steps of batches of 10 runs at a learning rate of 0.002 reached k = 0.898, a return of -0.0303, near the best. See `E-LRN-0043`.

### A baseline

Every return here is negative, so every run's score is multiplied by a negative number, and the estimate is the difference of large numbers that mostly cancel. The score's mean over runs is zero, so any constant can be subtracted from the return without changing what the estimate tends to; subtracting the batch's mean return (a baseline) leaves only whether a run did better or worse than usual. Over the first 50 steps the estimates had a spread of 29.2 without the baseline and 13.7 with it. With the baseline the learner reached k = 0.837 (return -0.0328). Less noise per estimate means fewer runs for the same progress, and on a robot every run is a trial. See `E-LRN-0044` and `E-LRN-0045`.

## Build It

`pg_estimate` in `exercise/solution.hpp`.

Open `exercise/solution.hpp`. The part you write is marked `TODO`; the rest of the file is already written, and it is worth reading before you start, because your part is called by it or calls it.

What to write, as the note beside the `TODO` puts it:

- B is 0, or with the baseline the mean of G over the batch; the estimate is the mean over the batch of S[i] (G[i] - b).

As shipped, 2 of the 3 tests fail, and these are the ones your code has to turn green:

- From a gain of zero the policy gradient climbs to near the best gain, with or without a baseline.
- Subtracting the batch's mean return makes the gradient estimates far less noisy.

The other one passes already, because it tests the parts that are given.

```
rcpp verify 39-07
```

A failing check names the test and prints the number it expected beside the one it got, so start from the first failure and work down. When the suite is green, read `reference/solution.hpp` beside your own version: where the two differ and both pass, the difference is worth understanding.

## Use It

`rc/learn/policy_gradient.hpp`.

```cpp
const double k = rc::learn::reinforce(task, 0.3, 0.002, 10, 300, true, seed, nullptr);   // with a baseline
```

What this does not do. The policy has one number; with a network, the score is the gradient of the log chance through the network (38-03's backpropagation), and the rest is the same. The baseline here is the batch's mean; a learned value of the state (a critic) is a better one, and the methods that use one (actor-critic) are what large systems train with. The noise sigma is fixed; making it smaller as the gain improves sharpens the end, and making it learnable is common. For this linear task the best gain can be computed outright from the equations (a Riccati recursion, phase 14's territory), which is what to do when the equations are known.

## What Breaks First

- A learning rate that makes the gain overshoot. See `E-LRN-0043`.
- A policy gradient without a baseline. See `E-LRN-0044`.
- Noise in the actions set to zero. See `E-LRN-0045`.

## Ship It

`rc/learn/policy_gradient.hpp` holds `PgTask`, `pg_run`, `pg_estimate`, `reinforce`, `mean_return` and `best_gain`.
