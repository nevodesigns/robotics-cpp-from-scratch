# Asking the Expert Where the Learner Goes: DAgger, Labels for the States the Clone Reaches, and a Policy That No Longer Drifts

> The fix for a learner that drifts into states nobody showed it is to show it those states, and only the learner can say which they are.

**Type:** Build
**Time:** about 90 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 38-05, 38-04

## The Problem

38-05 found the clone failing in states the expert never visited, because nobody showed it what to do there. More of the expert's own demonstrations do not fix that reliably: the expert, however it starts, goes where the expert goes, and the learner goes somewhere else. What is needed is the expert's answer in the learner's states, and only the learner can say which those are. DAgger (dataset aggregation, from imitation learning research) does exactly that: let the learner drive, ask the expert what it would have done at every state the learner reaches, add those answers to the data, train again, and repeat.

## The Concept

### Collecting on the learner's path

A run is driven by the learner. At each state it reaches, a sample is recorded whose label is the expert's action in that state, and then the car moves on with the learner's action, not the expert's. The expert is asked, not obeyed. That is what puts samples where the learner goes: near the edge, heading out, wherever its mistakes take it. The run ends when the car leaves the lane, since states past the edge are no use to a policy that should never reach them. See `E-LRN-0016`.

### Rounds

Each round collects from 20 runs, adds the samples to everything gathered so far (the original demonstrations included), and trains a new learner on the lot. The clone of 38-05, trained on 3000 samples, succeeded from the wide starts on 87 percent. After round 1 (4570 samples) the new learner succeeded on 98 percent, and after round 2 (6570 samples) on 98 percent; the expert's own rate from the same starts was 98 percent. On the states the learner itself visits, its actions differed from the expert's by 0.01082 before and 0.00009 after. See `E-LRN-0017`.

### What it costs

The expert has to be available to label states it did not choose, in the middle of the learner's mistakes. Here the expert is a formula and labelling is free. With a person it is not: they must say what they would do in a state they did not get the car into, often from a recording, and they label worse than they drive. A real robot driven by a learner that may leave the lane needs a safety driver or a simulator. That cost, not the algorithm, is what limits DAgger in practice. See `E-LRN-0018`.

## Build It

`collect_on_learner` in `exercise/solution.hpp`.

Open `exercise/solution.hpp`. The part you write is marked `TODO`; the rest of the file is already written, and it is worth reading before you start, because your part is called by it or calls it.

What to write, as the note beside the `TODO` puts it:

- For each of n starts (offset then heading, each drawn as offset0 (2 uniform - 1) and heading0 (2 uniform - 1)): for up to `steps` steps, record a Sample of the state with the EXPERT's action as its label and episode episode_base + k, then move the car with the LEARNER's action; stop the run when |offset| passes t.edge.

As shipped, both of the 2 tests fail. They are, in the order the suite runs them:

- The learner drives and the expert labels every state it reaches, with the learner's own path and the expert's actions.
- Two rounds of asking the expert bring the learner from the clone's success to the expert's.

```
rcpp verify 38-06
```

A failing check names the test and prints the number it expected beside the one it got, so start from the first failure and work down. When the suite is green, read `reference/solution.hpp` beside your own version: where the two differ and both pass, the difference is worth understanding.

## Use It

`rc/learn/dagger.hpp`.

```cpp
rc::learn::Learner policy = rc::learn::clone(demos, 16, 200, 0.02, seed);
const auto rounds = rc::learn::dagger(task, demos, policy, 2, 20, 100, 0.6, 0.8, 16, 100, 0.02, seed);
```

What this does not do. The rounds retrain from scratch on all the data, which is the simplest and the safest; versions that mix the expert's and the learner's actions while collecting, or weight new samples more, exist. The expert here is perfect and always available; with a noisy human expert the labels in hard states are the worst ones, and a guard (38-07) is still needed. Two rounds were enough for a two-number state; a real task needs more, and the success after each round is what says when to stop.

## What Breaks First

- A learner given more of the expert's own driving. See `E-LRN-0016`.
- Collected states labelled with the learner's action. See `E-LRN-0017`.
- A learner driven on the robot with no one to take over. See `E-LRN-0018`.

## Ship It

`rc/learn/dagger.hpp` holds `collect_on_learner`, `DaggerRound`, `dagger` and `visited_error`.
