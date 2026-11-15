# The Reward Is the Specification: A Hint That Speeds Learning, and a Bonus That Is Gamed

> A learner by trial does what the reward pays for, exactly and only; a reward that pays for something near what is wanted gets that something.

**Type:** Build
**Time:** about 90 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 39-04, 39-03

## The Problem

The learner of 39-03 does whatever the reward pays for, as well as it can. That is the point, and it is also the danger: the reward is the only specification it has. A reward only at the goal is honest but gives the learner nothing to go on until it stumbles on the goal, so learning is slow. The natural fix is to pay for progress on the way, and the natural way to do that is gamed. This lesson shows both, and the one form of hint that cannot be gamed.

## The Concept

### Only at the goal

The grid of this lesson is open, with no slip, no cost for a step and +10 at the goal, which is 4 steps from the start. With the plain reward, a learner learns nothing until a random walk happens to reach the goal. After 20 episodes its greedy policy had not found the goal (200 steps, where 200 means never); after 200 it took 8.

### A bonus for getting closer

Paying 1 for every step that ends closer to the goal sounds like the obvious hint. It is a reward for being closer, not for arriving, and the learner finds the difference: step away (no bonus, but no cost either), step back (+1), and repeat. Over 200 steps its policy collected 101 in bonus and never arrived (200 steps after 200 episodes). Nothing is wrong with the learner. It was paid to go back and forth, and it learned to. This is reward hacking, and every learned behaviour that surprises its makers starts here. See `E-LRN-0037`.

### A hint from a potential

The hint that cannot be gamed is the change in a potential: gamma times the potential where the move led, less the potential where it started, with the potential here minus the distance to the goal (and zero at the goal). A step closer is paid a little and a step away is charged a little. Along any run, the hints, discounted as the learner discounts rewards, add up to gamma to the power of the run's length times the potential at the end, less the potential at the start: they telescope, and the route in between does not appear. Every way of getting from the start to the goal is given the same total, so the best route under the shaped reward is the best route under the plain one, and going round a loop gains nothing that a straight route does not. It only gets found sooner: 4 steps after 20 episodes, the shortest, and 4 after 200. See `E-LRN-0038` and `E-LRN-0039`.

## Build It

`shaped_reward` in `exercise/solution.hpp`.

Open `exercise/solution.hpp`. The part you write is marked `TODO`; the rest of the file is already written, and it is worth reading before you start, because your part is called by it or calls it.

What to write, as the note beside the `TODO` puts it:

- KPlain pays r as it is; kPotential adds gamma times the potential of next (minus its goal_distance, or 0 if next ends the run) less the potential of s (minus its goal_distance); kCloser adds 1 when next is closer to the goal than s.

As shipped, all 3 of the 3 tests fail. They are, in the order the suite runs them:

- Each reward pays what it says: the plain one only at the goal, the potential a hint whose discounted sum depends only on where a run starts and ends, the bonus for every step closer.
- The potential's hint finds the shortest way after a few episodes, where the plain reward has not yet found the goal.
- The bonus for getting closer is gamed: the learner steps away and back for ever and never arrives.

```
rcpp verify 39-05
```

A failing check names the test and prints the number it expected beside the one it got, so start from the first failure and work down. When the suite is green, read `reference/solution.hpp` beside your own version: where the two differ and both pass, the difference is worth understanding.

## Use It

`rc/learn/reward_design.hpp`.

```cpp
const auto Q = rc::learn::q_learning_shaped(task, 200, 0.2, 0.99, 0.1, rc::learn::kPotential, seed);
```

What this does not do. The potential here is the distance on an open grid; where walls make that distance wrong, a poor potential slows learning but still cannot change the best route, which is its guarantee. The bonus here is gamed in a way that is easy to see; on a robot the gaming is subtler (a gait that shuffles to collect a reward for foot contact, a gripper that pushes the part to the edge of the camera's view because the reward was for the part being near the gripper in the image). Testing the learned behaviour against what was meant, not against the reward, is the only defence.

## What Breaks First

- A reward for progress that pays for going back and forth. See `E-LRN-0037`.
- A behaviour checked against its reward and not against what was wanted. See `E-LRN-0038`.
- A reward only at the end of a long task. See `E-LRN-0039`.

## Ship It

`rc/learn/reward_design.hpp` holds `Shaping`, `goal_distance`, `shaped_reward`, `q_learning_shaped` and `steps_to_goal`.
