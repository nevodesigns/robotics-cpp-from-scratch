# Releasing a Learned Policy: Every Check of Phase 38 Run Against One Job, and the First That Fails Named

> A learned policy is released on evidence a person can read: what it beats, where it was tested, how often it hands over, and that it can be made again.

**Type:** Build
**Time:** about 110 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 38-07, 38-06, 38-02, 36-08, 33-10

## The Problem

Phase 38 has built a learned policy in steps: data split so the test means something (38-01), a line to beat (38-02), a network with a checked gradient (38-03) trained without fooling anyone (38-04), driven to find where it drifts (38-05), taught where it drifts (38-06), and put behind a guard (38-07). Each lesson answered one question. Releasing the policy means answering all of them for one job, in an order, and saying which one stopped it, the way 32-06 chose an arm, 33-10 released a mobile manipulator and 36-08 a quadruped.

## The Concept

### The job and the candidate

A job is the spread of starts the policy must handle, the share it must succeed on, and how much of the driving its guard may take over. A candidate is the policy as it would ship: its network and scale, its guard, and the data and settings it was trained with, because a policy that cannot be trained again cannot be fixed or audited.

### Six requirements, in order

1. **It beats the straight line** on episodes it has not seen (38-02). If it does not, the network has earned nothing and the line, which a person can read, should ship instead.
2. **It succeeds from the job's starts**, behind its guard (38-05, 38-07).
3. **Its guard hands over rarely.** A guard that drives much of the time means the network was not taught the job (38-06 is the fix).
4. **It survives an input that is not a number** (38-07).
5. **It never commands past the car's limit** over the job's runs.
6. **It can be made again**: trained twice from the same seed, it gives the same weights (38-04).

The order runs from what a learned policy must be good at to what any released software must be. See `E-LRN-0022`.

### The cases

A policy trained by two rounds of DAgger, for a job of starts within 0.6 m and 0.8 rad: test error 0.00022 against the line's 0.01240, success 98 percent (needed 95), handover 3.2 percent (allowed 5). Released.

The plain clone of 38-05 failed at requirement 1. On episodes from the job's wide starts its test error was 0.01618, worse than the straight line's: taught only near the centre, it extrapolates worse than a line does. Had it got past that, its guard handed over on 12 percent of the steps, over the job's 5, and it would have failed the third as well. See `E-LRN-0023`.

The good DAgger policy behind a guard with a threshold of 1 spread failed at requirement 3: the guard handed over on 10 percent of the steps. The network was fine; the guard was set so tight that it would not let it drive.

A network of one hidden unit, trained for only 2 epochs, passed every requirement, with a test error of 0.00056. That is not luck. The expert is a line clipped at the car's limit, and one tanh unit is almost exactly a clipped line. A network the size of the task's shape is enough, and the line comparison is what shows it.

The DAgger policy given a job of starts within 0.95 m and 1.2 rad, some of which no controller can recover from: failed at requirement 2, success 73 percent. It is a good policy; the job is too hard, and the verdict says which. See `E-LRN-0024`.

## Build It

`check_policy` in `exercise/solution.hpp`.

Open `exercise/solution.hpp`. The part you write is marked `TODO`; the rest of the file is already written, and it is worth reading before you start, because your part is called by it or calls it.

What to write, as the note beside the `TODO` puts it:

- Fill every figure: test_error (mlp_mse of the learner on test, with its scale) and line_error (linear_mse of fit_linear(train, 0) on test), beats_line if the network's is lower; success and handover from guarded_success over the job's starts (100 starts, seed 77, 300 steps), checked against need_success and max_handover; bad_input_ok if guarded_act with a NaN offset and heading 0.1 gives kBadInput and a finite turn; largest_turn over 50 runs (seed 91, 300 steps), within t.max_turn; repeatable if two clone()s of c.data with c's settings have the same weights; first_fail is the first of the six in that order that fails (1 to 6), released if none.

As shipped, all 3 of the 3 tests fail. They are, in the order the suite runs them:

- A policy trained with DAgger, behind its guard, passes all six requirements and is released.
- The plain clone fails the first requirement on the job's episodes, a guard set too tight fails the third, and one well-shaped unit is enough.
- A job of starts no policy can manage fails on success, and the verdict keeps every figure.

```
rcpp verify 38-08
```

A failing check names the test and prints the number it expected beside the one it got, so start from the first failure and work down. When the suite is green, read `reference/solution.hpp` beside your own version: where the two differ and both pass, the difference is worth understanding.

## Use It

`rc/learn/policy_release.hpp`.

```cpp
const rc::learn::PolicyVerdict v = rc::learn::check_policy(task, job, candidate, train, test);
if (!v.released) explain(v.first_fail);   // 1 the line ... 6 repeatable
```

What this does not do. It is a screen run in simulation, against one task; the release of a policy that drives a real vehicle is a trial on the vehicle, with a safety driver, after the screen passes. The six requirements are the ones this phase taught; a real release adds the data's provenance (who demonstrated, when, with which sensors), a check on new sensors and conditions, and a plan for what is logged so that failures in use come back as data. Repeatability is checked on one machine; 38-04 noted that bit-for-bit results across machines need the same arithmetic, which these lessons were written to have.

## What Breaks First

- A learned policy released on its test error. See `E-LRN-0022`.
- A policy whose guard is doing the driving. See `E-LRN-0023`.
- A job too hard, blamed on the policy. See `E-LRN-0024`.

## Ship It

`rc/learn/policy_release.hpp` holds `PolicyJob`, `Candidate`, `PolicyVerdict`, `same_weights`, `largest_turn` and `check_policy`.
