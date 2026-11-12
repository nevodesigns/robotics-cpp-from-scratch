# A Few Robots First: Staged Rollout, the Wave That Cannot See a Rare Fault, and Halting on Silence

> Update one robot, and you learn about one robot. Update a thousand at once and you learn about a thousand, from a telephone.

**Type:** Build
**Time:** about 110 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated fleet)
**Prerequisites:** 20-06, 19-13, 18-07, 03-03

## The Problem

Lessons 20-05 and 20-06 make one robot safe to update: it cannot be bricked, and it takes a bad release back. That protects the robot. It does not protect **the customer**, whose robot spends the minutes of a trial boot not working, nor the fleet, if the bad release fails on a third of it at the same moment. A release is tested on the robots the engineers have, and **the fleet has robots they do not**: a hardware revision from last year, a sensor from another supplier, a site with a bad link.

So a release goes out **in waves**: a few robots first (the canary), then more, with a check between each. The check, and its size, are what this lesson is about, and both have a limit that is worth knowing in numbers.

## The Concept

### Waves, and what halts them

A `RolloutPolicy` is a list of cumulative shares (here 1, 10, 50 and 100 percent), a **maximum failure ratio**, and a **minimum reporting ratio**. Wave by wave, `rollout` gives the release to the next robots in order, then looks at the news: of the robots that reported, how many failed? It **halts** if more than the maximum did, and a halted rollout gives no more robots the release. A clean fleet of 1000 went through four waves of 10, 90, 400 and 500 robots. The waves are taken in the order given (the caller shuffles the fleet, so the canary is not always the same site), a wave that adds nobody is skipped, and a share that rounds up to the same robot as the last is not a new wave.

### A common fault is caught at once

A release that fails on **300 of 1000** robots (a hardware variant, say), with the robots spread through the fleet:

| policy | robots given it | failed | |
|---|---|---|---|
| all at once | 1000 | **300** | |
| four waves, halting above 5 percent | 10 | **4** | halted at wave 0 |

Four robots failed, of ten, and the rollout stopped there. Giving it to everyone failed 300. This is the case staged rollout is for, and it works well: any fault that is common is seen by a small wave. See `E-DEPLOY-0019`.

### A rare fault is not

A release that fails on **5 of 1000** robots, one in two hundred:

| policy | robots given it | failed | |
|---|---|---|---|
| halting above 5 percent | **1000** | 5 | never halted |
| halting on any failure | 500 | 2 | halted at wave 2 |

A threshold of 5 percent never fires for 5 robots in 1000: 0.5 percent is below it at every wave, and the release reached the whole fleet. A threshold of **zero** does halt, but only once a wave includes a bad robot, and that is a matter of chance. The chance that `n` robots include at least one of a fraction `p` is `1 - (1 - p)^n`. For `p` of 0.005:

| robots in the wave | chance of seeing the fault |
|---|---|
| 10 | 4.9 percent |
| 100 | 39.4 percent |
| 500 | 91.8 percent |
| 598 | 95.0 percent |
| 1000 | 99.3 percent |

A canary of ten robots sees a fault of one in two hundred **one time in twenty**. For 95 percent confidence, **598 robots** must have the release, which is 60 percent of this fleet, and for 99 percent **919**. A staged rollout is not a safety net for a rare fault: it tells you how big the net is. The only things that help are more robots in the first waves, more time (each robot used for longer shows more faults), and **tests on the variants that exist** before the release goes out at all. A rare fault that is also severe (a motor that overheats, a stop that does not stop) is the reason lesson 19-13's numbers are written down: it belongs in the hazard log and not in the rollout. See `E-DEPLOY-0020`.

### No news is not good news

The robots that do not report are in the fleet, and a rollout that counts only the ones that did looks healthy. With **45 percent** of the fleet not reporting (off, out of range, a dead link):

| policy | robots given it | silent | |
|---|---|---|---|
| halting below 80 percent reporting | **10** | 4 | halted at wave 0 |
| not checking who reports | **1000** | 450 | never halted |

The first sees that 4 of 10 gave no word and stops: the check cannot say it is safe, which is not the same as saying it is. The second sees no failures among the 55 percent who reported and goes on to **all of them**, 450 of which it has heard nothing from, which may be exactly the ones that failed to boot. A robot that crashes into a failed update **is** a silent robot. See `E-DEPLOY-0021`.

## Build It

`rollout`, `detection_probability` and `robots_needed` in `exercise/solution.hpp`. The types are given.

```
rcpp verify 20-07
```

## Use It

`rc/deploy/rollout.hpp` joins `rc::deploy`.

```cpp
rc::deploy::RolloutPolicy policy;
policy.cumulative = {0.01, 0.1, 0.5, 1.0};
const auto result = rc::deploy::rollout(shuffled_fleet, policy);
const int need = rc::deploy::robots_needed(0.005, 0.95);          // is the first wave big enough to be worth anything?
```

**What this does not do.** It simulates a rollout all at once, and a real one takes **time**: the robots that fail do so over hours, the report arrives later, a wave has to be waited for, and "silent" is a state that changes. It treats each robot as the same, with the fault a property of the robot, where faults often depend on **site and time** (a bad link, a cold morning). It does not decide who is in the first wave: an internal robot, then a friendly customer, then the rest, which is a policy about people. It assumes the fault is **visible**: a release that is slightly worse, or fails once a week, will be through every wave. And it gives no help with the release that must **not** go out of order, such as a change to the protocol between robots, which is a matter of compatibility between releases and not of rollout.

## What Breaks First

- **Everyone at once.** See `E-DEPLOY-0019`.
- **A canary too small to see a rare fault.** See `E-DEPLOY-0020`.
- **Going on because nobody complained.** See `E-DEPLOY-0021`.

## Ship It

`rc/deploy/rollout.hpp` holds `FleetRobot`, `RolloutPolicy`, `RolloutResult`, `rollout`, `detection_probability` and `robots_needed`.
