# Fix Quality and Outliers: Weighing a Fix by What the Receiver Says It Is, Refusing One That Is Too Far From Where the Robot Is, and Taking One Back After Too Many Refusals

> A GNSS fix is not a position, it is a claim about one, and the receiver says how much to believe it.

**Type:** Build
**Time:** about 85 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 25-02, 24-09, 22-06, 21-10

## The Problem

A GNSS receiver gives a position, and it says how good it thinks it is. A good deal of the information is not in the position at all: it is in the **fix type**, whether the receiver has a corrected solution good to centimetres (fixed), a float solution good to a quarter of a metre, or only a plain one good to a metre or two (single), or none. And a receiver is sometimes wrong in a way it does not know, when a signal has bounced off a wall and the position has jumped by metres (multipath). A filter that takes every fix as the truth is dragged about by the second, and one that weighs a plain fix as a corrected one is overconfident in the worst way.

## The Concept

### Weigh by level

`GnssFilter` keeps the position and velocity filter of lesson 24-09 and feeds it fixes with the noise of their **level**: 2 cm for fixed, 0.25 m for float, 1.5 m for single (the values of lesson 22-06's model, typical of each class of receiver), and nothing for none, when it only predicts. A fixed fix, an offset of a metre from the prediction, pulled the filter further than a single one did, because it said it was certain. On the run's single fixes the filter that weighed them by level had an rms error of **0.67 m**; one that took every fix as fixed had **2.81 m**, as it followed noise that it thought was signal. See `E-VEH-0061`.

### Test each fix against the filter

A fix of 8 m from where a robot going at half a metre a second has been is not a fix, it is a mistake. `GnssFilter` tests each fix against its own prediction: the **squared distance** of the fix from the predicted position, in units of their combined uncertainty (the covariance of the filter plus that of the fix), must be at most the **gate**, 9.21, which is the 99 percent point of a chi-square with two degrees of freedom: a correct fix is refused 1 time in 100. On a run of two minutes with 1 percent of the fixes thrown 8 m, five times, the worst position error was **6.04 m** for the filter that took every fix, and **2.29 m** with the gate, which is about the noise of the single fixes themselves (1.5 m a coordinate); the gate refused **0.2 percent** of the fixes that were right, close to the 1 percent that its threshold says, and in return took out every jump. See `E-VEH-0062`.

### A gate that never gives up

A gate is a statement that the filter is right. When the filter is wrong, it refuses every fix. A robot picked up and carried 20 m was found again by a gate that refused without limit only after **68 fixes (14 s)**: each fix was 20 m from the prediction and refused, until the uncertainty of the filter, which grows all the time it has no fix to correct it, had grown so large that 20 m was inside the gate. During all of that time the robot believed it was 20 m from where it was. `max_refused` is the cure: after that many refusals in a row, the next fix is taken as a **new start**. With 5, the robot was found again **5 fixes** after it was put down, a second of lost time. The price is that a run of bad fixes of the same kind can capture the filter, and that is what the number is for. See `E-VEH-0063`.

## Build It

`GnssFilter::update`, the gate in particular, in `exercise/solution.hpp`.

```
rcpp verify 25-03
```

## Use It

`rc/locate/gnss_filter.hpp` opens nothing new in `rc::locate`.

```cpp
rc::locate::GnssFilter gnss_filter((rc::locate::GnssFilterConfig()));
const bool used = gnss_filter.update(dt, fix);   // fix from the receiver, with its level
```

**What this does not do.** It filters position, and fuses no wheels and no heading; the next lessons add the scan and the map. The sigma of each level is a fixed number, where a real receiver reports an estimate of its own error that varies with the sky (the HDOP) and should be used in its place. The noise of the fixes is taken as independent from one to the next, which it is not: errors of a real receiver wander slowly, and a filter that thinks they are independent is too sure of itself in the long run. And it is the position of the reference point only if the lever arm of lesson 25-02 has been taken off.

## What Breaks First

- **A fix taken for a position whatever its level.** See `E-VEH-0061`.
- **A jump believed.** See `E-VEH-0062`.
- **A gate with no way back.** See `E-VEH-0063`.

## Ship It

`rc/locate/gnss_filter.hpp` holds `fix_sigma`, `GnssFilterConfig` and `GnssFilter`.
