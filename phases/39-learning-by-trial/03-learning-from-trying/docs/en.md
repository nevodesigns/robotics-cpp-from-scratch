# Learning from Trying: Q-Learning, One Update at a Time, and What the Trials Cost on the Way

> Without the model, the value of each move is learned from the moves themselves, and every lesson the learner takes is a run it really made.

**Type:** Build
**Time:** about 90 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 39-02, 39-01

## The Problem

Value iteration (39-02) needs every chance and every reward. A robot does not have them: nobody knows the chance that a foot slips on that floor or the reward of a grasp before trying it. What a robot can do is try: make a move, see where it ended and what it got. Q-learning turns those observations into the same optimal values, one at a time, without ever being told the chances. It is the plainest learner by trial, and it shows the price of every learner by trial: the lessons are runs that really happened, including the bad ones.

## The Concept

### A table of moves

Instead of a value per cell, Q-learning keeps a value per cell and move: Q(s, m), what making move m in cell s is worth if the best is done afterwards. With that table, acting well needs no model at all: take the move with the highest Q.

### One update

After a move from s that led to s' with reward r, the target is r plus gamma times the best Q in s' (or just r if the run ended there). Q(s, m) moves a share alpha of the way toward it. Each update uses one real outcome instead of the chance-weighted sum of 39-01, so it is noisy, and the small alpha averages the noise over many visits. Under mild conditions (every move tried in every cell often enough, alpha shrinking) the table settles on the optimal values. See `E-LRN-0031`.

### Trying, and trying something else

If the learner always took the move its table rates highest, it would never try the others and never find out they are better. So most of the time it takes the greedy move, and with a small chance epsilon (0.1 here) a random one. 39-04 is about how much of that is enough.

### How close, and at what cost

On the grid of 39-02, with gamma 0.95 and alpha 0.1, the learned policy agreed with the optimal one in 12 of the 17 cells after 100 episodes, 15 after 500 and 17 after 2000, and reached the goal on 48, 50 and 54 percent of test runs (the optimal policy: 54 percent). The cells it gets wrong last are the ones it seldom visits.

While it learned for 2000 episodes it fell into a pit 759 times. Every one of those falls was a real run, and on a robot each would be a fall, a crash or a dropped part. That is the cost value iteration never pays, and it is why learning by trial is done in simulation first and why 39-09 puts a shield round it. See `E-LRN-0032` and `E-LRN-0033`.

## Build It

`q_update` in `exercise/solution.hpp`.

Open `exercise/solution.hpp`. The part you write is marked `TODO`; the rest of the file is already written, and it is worth reading before you start, because your part is called by it or calls it.

What to write, as the note beside the `TODO` puts it:

- The target is reward, plus gamma times the best value in `next` unless the run ended there; move Q(s, m) (at index s * 4 + m) by alpha times (target - Q(s, m)).

As shipped, all 3 of the 3 tests fail. They are, in the order the suite runs them:

- One update moves a move's value a share alpha of the way to its target, and a run that ends adds no future.
- Learning only from trying, the policy comes to agree with the optimum of value iteration as the episodes add up.
- The trials are real: while it learned, the learner fell into a pit many times.

```
rcpp verify 39-03
```

A failing check names the test and prints the number it expected beside the one it got, so start from the first failure and work down. When the suite is green, read `reference/solution.hpp` beside your own version: where the two differ and both pass, the difference is worth understanding.

## Use It

`rc/learn/q_learning.hpp`.

```cpp
rc::learn::QStats st;
const auto Q = rc::learn::q_learning(task, 2000, 0.1, 0.95, 0.1, 0.0, seed, &st);
const auto policy = rc::learn::q_policy(task, Q);   // and st.pits is what learning cost
```

What this does not do. The table has an entry for every cell and move, which is fine for a grid and impossible for a robot's continuous state; 39-06 and 39-07 learn controllers with a few numbers instead. Alpha is fixed here, so the values keep moving a little with every noisy outcome. The learner is trained on the grid it is tested on; 39-08 is about what happens when the world it is used in differs.

## What Breaks First

- A learning rate that never lets the values settle. See `E-LRN-0031`.
- A policy taken from cells the learner seldom visited. See `E-LRN-0032`.
- Learning by trial on the real robot from the start. See `E-LRN-0033`.

## Ship It

`rc/learn/q_learning.hpp` holds `QStats`, `make_q`, `best_value`, `best_move`, `choose_move`, `take_step`, `q_update`, `q_learning` and `q_policy`.
