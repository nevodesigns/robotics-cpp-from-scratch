# Exploring Enough: A Small Reward Near By, a Large One Down a Corridor, and Values That Start Optimistic

> A learner finds only what its trying reaches; a little random trying stops at the first reward, and optimism sends it everywhere once.

**Type:** Build
**Time:** about 80 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 39-03

## The Problem

Q-learning (39-03) learns the value of the moves it makes. It never learns the value of a move it does not make, and it makes the moves its table already rates highest. A learner that finds a modest reward early will keep going back to it and never learn that something better lies further on. On a robot that is a gait that never tries the longer stride, or a grasp that never tries the other approach, because the first one worked well enough. Exploring is not optional, and how much of it is enough is a design question with numbers.

## The Concept

### A lure

The grid of this lesson has no slip. A goal worth 3 is two steps up from the start, and one worth 10 is at the end of a six-step corridor to the east. Going up returns 1 (two steps at -1, then +3); going east returns 4 (six steps, then +10). The best policy goes east.

### Too little trying

After 2000 episodes with no random moves at all, the learner reached the large goal 1 times; its greedy policy went for the small one and returned 1, and it was in the cell next to the large goal 2 times. With a tenth of the moves random, the usual setting, it reached the large goal 2 times and returned 1, and was next to it 3 times. To find the large goal by chance, a random walk has to go six steps east in a row without being pulled back to the lure, and the greedy moves pull it back. See `E-LRN-0034`.

### More trying

With half the moves random it was next to the large goal 44 times and reached it 27 times, and its greedy policy still returned 1: the large goal was found, but too seldom, against the lure's pull on every episode, for its value to overtake the lure's. More randomness reaches further, and it costs: a learner that moves at random half the time also behaves at random half the time, while it learns.

### Optimism

The other way is to start the table high: 20 for every move, more than anything in this task can return. Every move the learner has not tried still looks worth 20, every move it has tried has been pulled down to what it really gave, so the greedy choice is always an untried move until there are none. With no random moves at all, it reached the large goal 1970 times and its greedy policy returned 4, the best there is; it was next to the large goal 2155 times. Optimism makes exploration systematic instead of left to chance. See `E-LRN-0035` and `E-LRN-0036`.

## Build It

`q_learning_visits` in `exercise/solution.hpp`.

Open `exercise/solution.hpp`. The part you write is marked `TODO`; the rest of the file is already written, and it is worth reading before you start, because your part is called by it or calls it.

What to write, as the note beside the `TODO` puts it:

- The loop of q_learning in 39-03 (each episode from the start, at most 200 steps, choose_move, take_step, q_update), counting the start and every cell arrived in into visits, and counting in `big` the episodes that ended on 'G'.

As shipped, all 3 of the 3 tests fail. They are, in the order the suite runs them:

- With no random moves, or a tenth of them, the learner settles for the small goal next to the start.
- Half the moves random gets down the corridor more often, and still not often enough to learn that the large goal is better.
- Values started higher than anything the task can give send the learner to every untried move, and it finds the large goal with no random moves.

```
rcpp verify 39-04
```

A failing check names the test and prints the number it expected beside the one it got, so start from the first failure and work down. When the suite is green, read `reference/solution.hpp` beside your own version: where the two differ and both pass, the difference is worth understanding.

## Use It

`rc/learn/exploration.hpp`.

```cpp
std::vector<int> visits;
const auto Q = rc::learn::q_learning_visits(task, 2000, 0.1, 0.95, 0.0, 20.0, seed, visits, nullptr);
const int reached = rc::learn::cells_reached(visits);   // how much of the task was tried
```

What this does not do. Optimism works when every state can be tried; on a robot, "try every move once" includes the dangerous ones, so it has to be combined with a shield (39-09). The grid is small enough that visiting every cell is possible; in a large task, counts of visits stand in for it, as bonuses for the rarely visited. And how much exploring is enough is a property of the task: a lure close by and a prize far away is the hard case, and it is common.

## What Breaks First

- A learner that settles for the first reward it finds. See `E-LRN-0034`.
- Random exploration in a task with a long way to the prize. See `E-LRN-0035`.
- Optimism on a robot that can be hurt. See `E-LRN-0036`.

## Ship It

`rc/learn/exploration.hpp` holds `q_learning_visits`, `cells_reached` and `greedy_return`.
