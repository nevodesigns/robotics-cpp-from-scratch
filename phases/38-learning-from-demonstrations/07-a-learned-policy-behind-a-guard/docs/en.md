# A Learned Policy Behind a Guard: Inputs Nobody Trained On, Outputs Past the Limit, and a Controller to Hand Over To

> A learned policy is trusted inside the states it was taught and nowhere else, and the code round it has to know which is which.

**Type:** Build
**Time:** about 90 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 38-06, 38-05, 19-10

## The Problem

38-05 showed where a cloned policy fails: in states nobody demonstrated. 38-06 shrank that region by asking the expert there. Neither makes it empty. A policy will meet a state it was not taught, and a sensor will hand it a NaN, and a network can give an output past what the actuator can do. Phase 19 put one place in charge of stopping a robot (19-10); a learned policy gets the same treatment: code round it, written by hand and readable, that decides when the network may act and hands over to a plain controller when it may not.

## The Concept

### What the guard checks, in order

1. **An input that is not a number.** A NaN from a sensor reaches the network and comes out as a NaN turn, or as anything. The guard never passes it in; it hands over to the fall-back with that input taken as zero, and says so (`kBadInput`).
2. **A state the policy was not taught.** The guard keeps the scale of the training data. A state more than z_max spreads from the mean on either input is unseen, and the fall-back acts (`kUnseen`). This is a box round the data, the crude measure of 38-05 put to work.
3. **An output that is not a number.** A damaged weight, a division somewhere, the network gives a NaN. The fall-back acts (`kBadOutput`).
4. **An output past the limit.** It is clipped to the car's largest turn (`kClipped`); the network's direction is kept.

Every decision carries its reason, so a log of a run says how often the network actually drove. See `E-LRN-0019`.

### The fall-back

A proportional law on the offset and the heading, clipped, written by hand with gains a controls engineer would choose. It is not the expert (the learner never sees the expert in use); it is a controller that is simple enough to be checked by reading it. On its own it succeeded from the wide starts on 98 percent: good enough to be trusted where the network is not, not necessarily better than the network where the network was taught.

### The guarded clone

The clone of 38-05, trained only near the centre line, succeeded from the wide starts on 87 percent. Behind a guard with a threshold of 3 spreads it succeeded on 98 percent, the guard handing over on 12 percent of the steps, near the edges, where the clone had never been taught. From the narrow starts the guard handed over on 2.4 percent of the steps, the first steps of starts at the very edge of the demonstrations: where the network was taught, it drives. See `E-LRN-0020`.

### The threshold

The threshold is a design number, and it decides how much of the driving the network does. With 1 spread the guard handed over on 20 percent of the steps and succeeded on 98 percent: here the fall-back is as good as the expert, so the success did not suffer, but a fifth of the driving was taken from the network, which is what it was trained for. With 1000 spreads it handed over on 0 percent and succeeded on 87 percent: a guard that never acts is no guard, and the clone fails as before. Notice what the usual threshold of 3 caught: the clone's failures came in the first steps of the sharply angled starts, far outside the demonstrations, which is exactly where the box says unseen. A box round the data is a coarse test; states inside the box can still be far from any sample (in a corner the data never reached), and that is a limit to say plainly. See `E-LRN-0021`.

## Build It

`guarded_act` in `exercise/solution.hpp`.

Open `exercise/solution.hpp`. The part you write is marked `TODO`; the rest of the file is already written, and it is worth reading before you start, because your part is called by it or calls it.

What to write, as the note beside the `TODO` puts it:

- In this order: an input that is not finite gives the fall-back (with that input taken as 0) and kBadInput; a state more than g.z_max spreads from the mean on either input (scaled by g.seen) gives the fall-back and kUnseen; then the learner's output: not finite gives the fall-back and kBadOutput, past t.max_turn is clipped with kClipped, otherwise it is used as it is with kLearned.

As shipped, all 3 of the 3 tests fail. They are, in the order the suite runs them:

- The guard says why it acted: a bad input, an unseen state, a bad output, an output past the limit, or the network's own.
- Behind the guard the clone of 38-05 succeeds from wide starts, and from the narrow ones the guard leaves it alone.
- A tighter threshold hands more of the driving to the fall-back, and one so loose it never acts leaves the clone as it was.

```
rcpp verify 38-07
```

A failing check names the test and prints the number it expected beside the one it got, so start from the first failure and work down. When the suite is green, read `reference/solution.hpp` beside your own version: where the two differ and both pass, the difference is worth understanding.

## Use It

`rc/learn/guard.hpp`.

```cpp
rc::learn::Guard guard;
guard.seen = policy.scale;                 // the training data's own scale
const rc::learn::GuardedAction a = rc::learn::guarded_act(task, guard, policy, offset, heading);
if (a.reason != rc::learn::kLearned) log_handover(a.reason);
```

What this does not do. The test of unseen states is a box in the scaled inputs; better tests measure the distance to the nearest training samples, or train a second model to say how sure the first is, and none is perfect. The fall-back is assumed to be safe wherever it is used; on a real robot the last fall-back is to stop (phase 19). Switching between two controllers can itself cause a jolt, and a real guard blends or holds the switch for a moment. The guard does not make a bad policy good; it limits where a policy is allowed to be bad.

## What Breaks First

- A NaN passed into a network. See `E-LRN-0019`.
- A learned policy with no fall-back. See `E-LRN-0020`.
- A threshold for unseen states chosen by eye. See `E-LRN-0021`.

## Ship It

`rc/learn/guard.hpp` holds `GuardReason`, `Guard`, `GuardedAction`, `fallback_turn`, `guarded_act` and `guarded_success`.
