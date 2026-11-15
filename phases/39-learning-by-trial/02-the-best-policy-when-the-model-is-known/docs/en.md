# The Best Policy When the Model Is Known: Value Iteration, the Greedy Move, and the Route the Discount Chooses

> With the chances and the rewards known, the best policy is arithmetic; the discount in that arithmetic is a choice, and it chooses the route.

**Type:** Build
**Time:** about 80 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 39-01

## The Problem

39-01 could say what any one policy is worth. The question that matters is which policy is worth most, and there are four to the power of the number of cells of them. When the chances and the rewards are known, there is no need to try them one by one: the best value satisfies its own equation, and sweeping it finds the best policy for every cell at once. That is value iteration, the base of everything in this phase: the learning methods that follow do the same arithmetic without knowing the model.

## The Concept

### The best value

The best value of a cell is the value of its best move: the largest, over the four moves, of the chance-weighted reward plus the discounted best value of where the move leads. Sweep that over every cell, from all zeros, until nothing changes. On the grid of 39-01, now with a longer way round the pits, and gamma 0.95, it settled in 36 sweeps, with a value at the start of -5.245. Going round was worth -5.745 and the random policy -13.357; no policy does better than the optimum, at the start or anywhere else.

### The greedy policy

From the settled values, the best move in each cell is the one with the highest value: the greedy move. The policy made of greedy moves is optimal, and its own value, computed as in 39-01, is exactly the optimal value. Once the values are known, acting well is a lookup. See `E-LRN-0028`.

### The discount is part of the task

With gamma 0.80, the greedy move at the start was north, the long way round: 95 percent of runs reached the goal, with a mean undiscounted return of -5.13. With gamma 0.95 it was east, along the pits: 54 percent reached the goal, with a mean return of -5.16. The two are not one right answer and one wrong one. A low discount makes the far future count for little: the long way's many steps cost little, but a pit next to the next step costs a lot, so it takes no risks. A high discount weighs the whole path and accepts a pit on half the runs because the corridor is short. The discount turned out to be a statement about risk, which is the kind of thing an engineer has to decide on purpose, not leave at 0.99 because that is usual. See `E-LRN-0029` and `E-LRN-0030`.

## Build It

`value_iteration` in `exercise/solution.hpp`.

Open `exercise/solution.hpp`. The part you write is marked `TODO`; the rest of the file is already written, and it is worth reading before you start, because your part is called by it or calls it.

What to write, as the note beside the `TODO` puts it:

- As evaluate_policy of 39-01, but each cell (not a wall, not terminal) takes the largest move_value over the four moves; count sweeps in n from 1 and stop after the sweep whose largest change is below tol.

As shipped, both of the 2 tests fail. They are, in the order the suite runs them:

- Value iteration settles on a value no policy beats, and the greedy policy it gives is worth exactly that.
- The discount chooses the route: a low one goes the long way round, a high one takes the corridor beside the pits.

```
rcpp verify 39-02
```

A failing check names the test and prints the number it expected beside the one it got, so start from the first failure and work down. When the suite is green, read `reference/solution.hpp` beside your own version: where the two differ and both pass, the difference is worth understanding.

## Use It

`rc/learn/value_iteration.hpp`.

```cpp
const std::vector<double> V = rc::learn::value_iteration(g, 0.95, 1e-10, nullptr);
const std::vector<int> policy = rc::learn::greedy_policy(g, V, 0.95);
```

What this does not do. It needs the model: every chance and every reward. A robot seldom has one, and 39-03 learns the same values from trying. It sweeps every state every time, which is fine for a few dozen cells and hopeless for a continuous state. And it optimises one number, the expected discounted return; a pit on half the runs is acceptable to it if the average says so, which is why a constraint is a separate thing (39-09).

## What Breaks First

- A greedy policy from values that had not settled. See `E-LRN-0028`.
- A discount left at its usual value. See `E-LRN-0029`.
- An optimal policy that is not the one wanted. See `E-LRN-0030`.

## Ship It

`rc/learn/value_iteration.hpp` holds `value_iteration`, `greedy_move`, `greedy_policy`, `OutcomeStats` and `outcome_stats`.
