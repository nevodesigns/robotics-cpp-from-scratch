# A Dataset You Can Trust: Demonstrations as Samples, a Scale Taken from the Training Part, and a Split by Episode

> A test set that shares episodes with the training set tests the memory of the learner, not what it learned.

**Type:** Build
**Time:** about 90 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 31-09, 26-04, 18-07, 00-12

## The Problem

Phase 31 recorded what a person's hand showed an arm, and 31-09 kept those recordings as episodes: a sample at every moment, the state seen and the action taken. This phase learns a policy from such recordings, a function from the state to the action, and the first thing to get right is not the learner. It is the data, and in particular the test that says whether the learner has learned anything. A test that is wrong in the learner's favour is the most expensive mistake of this phase, because it looks like success.

The phase uses one task throughout so the numbers can be compared: a car on a straight lane (the cross-track error and the heading error of 26-04), steered by a turn rate, with an expert that knows how to steer it.

## The Concept

### The task and the expert

The state is the car's offset from the centre line and its heading error; the action is a turn rate, limited to 1.0 rad/s, at a speed of 2 m/s. The expert is a proportional law on both, clipped to the largest turn: the kind of controller a person's steering roughly follows. It is the teacher; the learner never sees the law, only what the expert did. 40 episodes of 100 steps, from starts up to 0.6 m off centre, gave 4000 samples, and the expert never let the car further than 0.71 m from the centre. Two things make the demonstrations like a person's. Each episode's demonstrator steers with the expert's gains times a factor of its own, within 30 percent, as a person steers a little harder one time than another; and the executed action has a little noise added, so the episodes wander and cover some of the space round the expert's path. See `E-LRN-0001`.

### A scale taken from the training part

Inputs in different units (metres and radians here, and in a real robot millimetres and newtons) are put on one scale before any learner sees them: subtract the mean, divide by the spread. Here the spreads were 0.174 m and 0.108 rad. The mean and the spread are part of the learned policy and are fitted on the training part only. Fitted on everything, they carry a little of the test set into the training, which is a small leak of the same kind as the next one. A spread of zero (an input that never changed in the data) must not be divided by.

### Splitting by episode

Neighbouring samples of one episode are nearly the same: 0.05 s apart, the car has hardly moved, and the same demonstrator, with the same style, chose both actions. If every fifth sample is put in the test part, every test sample has a training sample of its own episode a moment before and after it, and a learner that only remembers will look excellent. A nearest neighbour, which is pure memory, shows it plainly: split by sample its error was 0.00118, split by episode 0.00317, 2.7 times as much. The split by sample shared all 40 episodes between the two parts; the split by episode shared 0.

The rule is to split by the unit that is independent: the episode, the day, the robot, the operator. A learner will be used on episodes it has never seen, so it must be tested on such episodes. See `E-LRN-0002` and `E-LRN-0003`.

## Build It

`fit_normaliser` and `split_by_episode` in `exercise/solution.hpp`.

Open `exercise/solution.hpp`. The part you write is marked `TODO`; the rest of the file is already written, and it is worth reading before you start, because your part is called by it or calls it.

What to write, as the notes beside each `TODO` put it:

- For each of the two inputs, the mean over d and the spread (the square root of the mean of the squares less the square of the mean, at least 1e-6); an empty d leaves the defaults.
- A sample whose episode number is a multiple of `every` goes to test, every other sample to train, in order.

As shipped, 3 of the 4 tests fail, and these are the ones your code has to turn green:

- A normaliser fitted on a set gives that set zero mean and a spread of one, and a set with no spread does not divide by zero.
- A split by episode keeps every episode on one side, and a split by sample puts every episode on both.
- Splitting by sample makes a nearest neighbour look better than it is, because its test samples sit beside training samples of their own episode.

The other one passes already, because it tests the parts that are given.

```
rcpp verify 38-01
```

A failing check names the test and prints the number it expected beside the one it got, so start from the first failure and work down. When the suite is green, read `reference/solution.hpp` beside your own version: where the two differ and both pass, the difference is worth understanding.

## Use It

`rc/learn/dataset.hpp`.

```cpp
std::vector<rc::learn::Sample> train, test;
rc::learn::split_by_episode(data, 5, train, test);
const rc::learn::Normaliser scale = rc::learn::fit_normaliser(train);   // from the training part only
```

What this does not do. The task is a straight lane with a constant speed and two state variables; a real policy sees many more, and images. The expert is a formula, which a real demonstrator is not: people are inconsistent, and two people steer differently, which is a second reason to split by person. The nearest neighbour is here as a measuring stick for leakage, not as a learner to use.

## What Breaks First

- A test set made from the same episodes as the training set. See `E-LRN-0002`.
- A scale fitted on all the data. See `E-LRN-0003`.
- Demonstrations that cover only where the expert went. See `E-LRN-0001`.

## Ship It

`rc/learn/dataset.hpp` holds `Rng`, `LaneTask`, `lane_expert`, `lane_step`, `Sample`, `demonstrations`, `Normaliser`, `fit_normaliser`, `normalise`, the two splits, `shared_episodes`, `nearest_action` and `nearest_mse`.
