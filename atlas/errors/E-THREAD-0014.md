id: E-THREAD-0014
title: A pool sized for the machine, and a speedup that stopped at 7
match: (hardware_concurrency|more workers than cores|speedup stopped|plateau)
platforms: linux, windows
teaches: 07-10-how-many-workers
---

## Symptom

A pool of `std::thread::hardware_concurrency()` workers was meant to make a job twelve
times faster and made it seven. Raising it to 48 made no difference. Or a pool of that
size for tasks that mostly wait on a device runs at a small fraction of what it could.

## Cause

The right number of workers depends on **what the tasks do**, and the machine's thread
count is one input to it.

Measured on 6 cores with 2 hardware threads each, so `hardware_concurrency()` said 12.
48 tasks of about 20 ms of real computation, by workers:

| workers | 1 | 2 | 4 | 8 | 12 | 24 | 48 |
|---|---|---|---|---|---|---|---|
| speedup, run 1 | 1.0 | 2.0 | 3.5 | 6.1 | 6.6 | 6.8 | 7.2 |
| speedup, run 2 | 1.0 | 2.0 | 3.8 | 6.4 | 7.7 | 7.4 | 7.4 |

It scales with the cores and stops. Twelve workers gave 7, not 12, because the two
threads of a core share execution units, and workers beyond the hardware threads gave
nothing and cost memory and switching.

The same 48 tasks if they each **wait** 20 ms, as a task blocked on a device does:

| workers | 1 | 4 | 12 | 48 |
|---|---|---|---|---|
| speedup | 1.0 | 4.0 | 11.9 | **44.6** |

A waiting task uses no core, so it scales far past the core count.

The experiment is easy to get wrong in a way that flatters. The first version spun until a
wall clock deadline, so an oversubscribed task finished on time without having been given
the CPU, and reported speedups of 15 and 20 on 12 hardware threads, which is impossible.
Computation has to be a fixed amount of work, not a duration.

## Fix

Size the pool for the work. For computation, about the number of physical cores, and treat
the second hardware thread as a bonus of 20 percent or so rather than a core. For tasks that
wait, as many as there are things to wait on, within reason. A pool used for both wants two
pools, so that waiting tasks cannot starve the computing ones.

Measure the speedup you got as well as the pool you configured, at the sizes you will run.
