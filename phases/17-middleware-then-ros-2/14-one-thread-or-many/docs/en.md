# One Thread or Many: Executors, Callback Groups and the Control Loop Stuck Behind the Planner

> The controller runs at 100 Hz and takes a millisecond. The planner takes fifty. They are in the same process, on the same thread, and the controller has stopped.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 17-02, 17-13, 03-03

## The Problem

A node does not run its subscriber functions itself. It hands them to an **executor**, which calls them: when a message arrives, when a timer is due, when a service request comes in. The default executor in ROS 2 is **single threaded**, and a callback that takes a long time holds up every other callback in every node it is serving, however unrelated.

Lesson 17-02 put a queue between a publisher and a subscriber so that the slow one did not slow the fast. A slow callback on a single thread does the same thing from the other side: the queue is not what is slow, the thing that empties it is, and the thing that empties it is also emptying the control loop's.

The fix has two parts, threads and **callback groups**, and the second is the one that is easy to get wrong.

## The Concept

### One thread, in order

An executor with one thread starts callbacks in the order they became ready, each when the last has finished. Three callbacks ready at 0, 1 and 2 ticks and taking 5, 3 and 2:

```
started at 0, 5, 8; waited 0, 4, 6
```

Each waits for everything before it. Nothing is wrong yet.

### The long callback

Now a control callback of 1 tick every 10 ticks, 100 times, and a planner callback ready at tick 5 that takes `n` ticks. How long does a control callback wait, worst case, on one thread?

| planner takes (ticks) | worst wait of a control callback |
|---|---|
| 5 | 0 |
| 20 | 15 |
| 50 | 45 |
| 200 | 195 |

A planner of 5 ticks finishes before the next control tick is due, and there is no wait. A planner of 50 ticks makes the control callback 45 ticks late, and one of 200 ticks makes it 195 late, which is the planner's length minus the gap until the next control tick. At 1 ms per tick the 200 tick planner is a control loop that stops for 195 ms: a robot arm
moves a long way in that time with nobody steering it. See `E-BUS-0040`.

### Another thread, in another group

Two threads let two callbacks run at once, and a **callback group** says which may. A **mutually exclusive** group never runs two of its callbacks at the same time. A **reentrant** group may.

With the control in one group and the planner in another, each exclusive, and two threads:

```
planner of 50 ticks: one thread, control waited up to 45 ticks; two threads in separate groups, 0
```

The control callback runs on time, every time, while the planner runs beside it. See `E-BUS-0040`.

### Threads that do nothing

Put everything in **one** mutually exclusive group, and give the executor two threads. The control callback and the planner are in the same group, so they cannot overlap, and the second thread has nothing to do:

| group | threads | worst wait |
|---|---|---|
| one exclusive group | 2 | 45 |
| one reentrant group | 2 | 0 |

45 ticks is exactly what one thread gave. The extra thread was there and idle, and a person who adds threads to cure a latency problem and sees nothing change has met this. Making the group reentrant helps, and it moves the burden to the callbacks: two callbacks of that group can now run at once, so whatever they share needs a lock (phase 7). See `E-BUS-0041`.

### More callbacks than threads

A reentrant group does not mean unlimited. Four callbacks of 10 ticks, ready together, in a reentrant group, on two threads:

```
started at 0 0 10 10
```

Two started, and two waited for a thread. With three threads the third started at once and the fourth waited. See `E-BUS-0042`.

### Blocked, and behind

A callback held back by its group does not hold up callbacks in other groups that are behind it in the line. Task 1 holds group 0 for 100 ticks. Task 2, in group 0, must wait, and starts at 100. Task 3, in group 1 and ready after task 2, starts at 2:

```
the next in group 0 started at 100, one in group 1 ready after it started at 2
```

And a callback that never gets to run, because the run was stopped first, is reported as having waited the whole time: stopped at tick 50, the blocked callback counts as having waited 49. A measure that left it out would say the worst wait was 0. See `E-BUS-0042`.

## Build It

`simulate` and `worst_wait` in `exercise/solution.hpp`. The types are given.

```
rcpp verify 17-14
```

The simulation is in whole ticks, and nothing is timed or random.

## Use It

`rc/bus/executor_sim.hpp` joins `rc::bus`.

```cpp
std::vector<Task> tasks = ...;      // when each callback became ready, how long it takes, and its group
const auto runs = simulate(tasks, {true, true}, 2, 5000);
const int control_wait = worst_wait(tasks, runs, 0, 5000);
```

**What this does not do.** It simulates a model: callbacks have fixed durations and ready times, a thread starts a callback the instant it is free, and there is no cost to switching. A real executor has several kinds of callback, with a priority that its scheduler applies differently and a wait set that is polled. Real callbacks do not take a fixed time. It also leaves the answer open when the fix is neither threads nor groups but
making the planner shorter or putting it in another process, and that is usually the better fix.

## What Breaks First

- **A long callback in the executor that runs the control loop.** The loop stops for as long as the callback runs. See `E-BUS-0040`.
- **Threads added and everything in one exclusive group.** No change at all. See `E-BUS-0041`.
- **A reentrant group with shared data and no lock.** The race that the group was protecting against. See `E-BUS-0042`.

## Ship It

`rc/bus/executor_sim.hpp` holds `Task`, `Run`, `simulate` and `worst_wait`.

The cost of putting two things in one executor is now a number to read, before it is a robot that stops.
