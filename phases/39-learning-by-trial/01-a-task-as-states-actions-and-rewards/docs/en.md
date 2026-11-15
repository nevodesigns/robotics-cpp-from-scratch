# A Task as States, Actions and Rewards: A Grid with a Pit, Moves That Slip, and the Value of Following a Policy

> Learning by trial starts by writing the task down as states, actions, what each action may lead to and what it is worth; everything after is arithmetic on that.

**Type:** Build
**Time:** about 90 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 38-01, 16-06, 06-11

## The Problem

Phase 38 learned from an expert's answers. Often there is no expert: nobody can say what a quadruped's knee should do on ice, or how a grasp should change when the part is wet. What can be said is how good an outcome was. Learning by trial turns that into a controller: try things, see what they are worth, do more of what was worth more. Before any learning, the task has to be written down in a form that learning can work on, and this lesson does that for the smallest task that shows every difficulty of the phase: a grid with a goal, a pit and moves that slip.

## The Concept

### States, actions, outcomes, rewards

A Markov decision process is four things. The states: here the cells of a grid. The actions: four moves. The outcomes: a move goes where it is meant with chance 1 - 2 slip and to each side with chance slip (a wheel that slips, a foot that skids), so the same action in the same state does not always end in the same place. And the rewards: every step costs -1, arriving at the goal is worth +10 and ends the run, falling into a pit costs 10 and ends it. "Markov" means the outcome depends only on the cell and the move, not on how the robot got there; if that is not true of a task, the state is missing something (16-05).

The grid of this phase has a corridor straight to the goal along a row of pits, and a longer way round above. With slip 0.1, every step along the pits risks one. See `E-LRN-0025`.

### A policy and its value

A policy says which move to make in every cell. Its value at a cell, V(s), is the expected sum of the rewards from there on, each step's discounted by gamma (0.95 here) so that the far future counts less and the sum stays finite. It satisfies an equation: V(s) is, over the three outcomes of the policy's move, the chance times (the reward on arrival plus gamma times V of where it arrives). Sweeping that equation over every cell again and again, from all zeros, settles on the value. See `E-LRN-0026`.

The random policy, each move a quarter of the time, was worth -13.276 at the start (settled in 221 sweeps): it wanders, pays for every step, and finds the pit as often as the goal. Always moving east, along the pits, was worth -5.852. Going round above was worth -4.042. Which is better is a matter of arithmetic once the task is written down, and 39-02 does that arithmetic for every policy at once.

### What the number means

The value is an expected discounted return, and running the policy shows it. 20000 runs of the east policy gave a mean discounted return of -5.836, close to its value of -5.852, and 47.7 percent of them ended in a pit. A value is an average over outcomes; it does not say that every run is fine. See `E-LRN-0027`.

## Build It

`evaluate_policy` in `exercise/solution.hpp`.

Open `exercise/solution.hpp`. The part you write is marked `TODO`; the rest of the file is already written, and it is worth reading before you start, because your part is called by it or calls it.

What to write, as the note beside the `TODO` puts it:

- Sweep (counting sweeps in n, from 1, at most 100000): for every cell that is not a wall and not terminal, set V[s] to move_value of the policy's move there, keeping the largest change of any cell; stop after the sweep in which that change is below tol.

As shipped, 2 of the 3 tests fail, and these are the ones your code has to turn green:

- The value of a policy is found by sweeping until it settles, and the random policy is worth little.
- The value is what running the policy gets on average: many runs agree with it, and some of them end in a pit.

The other one passes already, because it tests the parts that are given.

```
rcpp verify 39-01
```

A failing check names the test and prints the number it expected beside the one it got, so start from the first failure and work down. When the suite is green, read `reference/solution.hpp` beside your own version: where the two differ and both pass, the difference is worth understanding.

## Use It

`rc/learn/grid_task.hpp`.

```cpp
const rc::learn::GridTask g = rc::learn::make_grid(rows, -1.0, 0.1);
const std::vector<double> V = rc::learn::evaluate_policy(g, policy, 0.95, 1e-9, nullptr);
```

What this does not do. The grid has a few dozen states and four actions, so values are kept in a table; a robot's state is continuous, and 39-06 onward deals with that. The model (the chances and the rewards) is known here, which is what lets the value be computed exactly; in the rest of the phase the learner does not know it and has to find out by trying. The discount is a choice, and 39-02 shows that it changes the answer.

## What Breaks First

- A task whose state leaves out what decides the outcome. See `E-LRN-0025`.
- A value taken from a sweep that had not settled. See `E-LRN-0026`.
- A value read as a promise about every run. See `E-LRN-0027`.

## Ship It

`rc/learn/grid_task.hpp` holds `Move`, `GridTask`, `make_grid`, `start_of`, `is_wall`, `is_terminal`, `reward_of`, `moved`, `outcomes`, `move_value`, `evaluate_policy`, `evaluate_random` and `run_policy`.
