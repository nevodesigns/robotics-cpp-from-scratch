# The Robot That Was Picked Up: A Cloud That Is Sure and Wrong, the Fall in How Well It Explains the Scan, and Scattering a Few New Guesses Until One Is Right

> The most dangerous estimate is the confident one that is wrong, and the only way out is to keep a little doubt on purpose.

**Type:** Capstone
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 25-05, 25-06, 25-03, 21-10

## The Problem

A localiser that has found the robot is confident, and that is what it is for. It is also what makes it dangerous when the robot is **lifted and put down somewhere else**: a person picks it up, a crane moves it, a wheel slips so much on a bank that the odometry is a lie. The cloud has settled on a place and keeps explaining the scans from there. When the new scans do not fit, the weights fall for every particle, are normalised again to sum to one, and the cloud goes on being as sure as ever of a place that is wrong. A plain cloud, picked up and put down 5.6 m away, was **8.3 m** out 40 steps later, and would have stayed so.

## The Concept

### How well does the cloud explain the scan

The normalising of the weights hides the one number that matters. Before it, the average likelihood of the scan under the cloud is a measure of how well the cloud explains what is seen (lesson 25-05's `correct` returns it). While the robot is where the cloud says, it is steady. When the robot is moved, it **falls**, at once, and stays down.

### A slow average and a fast one

`RecoveryMonitor` follows the average likelihood with two exponential averages of different speed. The **slow** one is the usual level; the **fast** one is what it is lately. When the robot is where it should be, the two agree. When the likelihood falls, the fast average follows it down in a few steps and the slow one stays up, and the ratio `fast / slow` falls. The share `1 - fast / slow`, never below zero and never above a limit (here 20 percent), is the **fraction of the cloud to put in at random**. An improvement gives no share: a rise in the likelihood is not a loss.

### Scattering

`scatter` replaces that fraction of the cloud, at random, by poses spread over the whole map with any heading, and gives them an equal weight. Most of the new particles are wrong and are weighed away in the next step. One, out of a few hundred, may land near the truth, and then it is the heaviest, is copied by the resampling, and the cloud is on the robot again. The robot lifted and put down 5.6 m away was **found again after 21 steps**, within 0.3 m, and the monitor called it **lost after 1**. See `E-VEH-0079`.

### The price of doubt

The monitor must not put in particles when the robot is where it should be: that throws away a good cloud. In the 40 steps before the robot was moved the monitor put in an average of **0.1 percent** of random particles, and the error at that point was **0.006 m** with it and **0.006 m** without it. A limit on the share (**20 percent**) keeps one bad scan from throwing the cloud off. A monitor tuned to be quick to notice is a monitor that cries wolf; the two averages are what keeps it honest. See `E-VEH-0080`.

### Telling the rest of the robot

`verdict` turns the two averages and the spread of the cloud into **good**, **doubtful** or **lost**. A robot whose localisation is lost should stop, or slow, and say so: it should not carry on with a task on a position it has no reason to believe. It is the same idea as the bound of lesson 25-06, from the other side: that said how wrong the estimate can be if the model is right, and this says that the model is wrong. See `E-VEH-0081`.

## Build It

`RecoveryMonitor::update` in `exercise/solution.hpp`.

Open `exercise/solution.hpp`. The part you write is marked `TODO`; the rest of the file is already written, and it is worth reading before you start, because your part is called by it or calls it.

What to write, as the note beside the `TODO` puts it:

- On the first call set both averages to the value and return 0; after that move each average towards the value by its alpha; the share is 1 - fast / slow, held between 0 and max_share.

As shipped, 2 of the 4 tests fail, and these are the ones your code has to turn green:

- The monitor asks for nothing while the scans are explained and for more as the fast average falls below the slow.
- A robot put down elsewhere is lost for good by a plain cloud and found again by one with a monitor.

The other 2 pass already, because they test the parts that are given.

```
rcpp verify 25-09
```

A failing check names the test and prints the number it expected beside the one it got, so start from the first failure and work down. When the suite is green, read `reference/solution.hpp` beside your own version: where the two differ and both pass, the difference is worth understanding.

## Use It

`rc/locate/recovery.hpp` opens nothing new in `rc::locate`.

```cpp
const double avg = mc.correct(angles, ranges, rc::locate::McMeasure());
const double share = monitor.update(avg);
mc.resample_if_needed();
if (share > 0.0) rc::locate::scatter(mc, share, x0, x1, y0, y1, rng);
if (rc::locate::verdict(monitor.fast(), monitor.slow(), mc.spread()) == rc::locate::Health::lost) stop_and_say_so();
```

**What this does not do.** The random particles are spread over the whole map, which for a big map is thin: the recovery is quick in a room and slow in a building, and a better source of new guesses is the last known place, a fix, or a scan matched against a map of places (lesson 25-04). The detection takes time: for a few steps the robot acts on a position that is wrong, and the verdict is the only protection. A robot moved to a place that looks the same (lesson 25-05's symmetric room) is not noticed at all, because the scans are explained just as well. And it needs the map to be right: a wall that has been moved in the building makes the likelihood fall without any kidnapping.

## What Breaks First

- **A localiser that is sure and wrong.** See `E-VEH-0079`.
- **A monitor that cries wolf.** See `E-VEH-0080`.
- **A robot that carries on with a lost position.** See `E-VEH-0081`.

## Ship It

`rc/locate/recovery.hpp` holds `RecoverySettings`, `RecoveryMonitor`, `scatter`, `Health` and `verdict`.

This ends phase 25 of the specialisation course, knowing where you are, outdoors: a heading from a gyro and the wheels, GNSS in the robot's frame and weighed by its quality, scans matched to scans and to a map, a bound on how lost the robot can be, the bias of a gyro read at the stops, measurements from the past put where they belong, and a way back for a robot that has been picked up.
