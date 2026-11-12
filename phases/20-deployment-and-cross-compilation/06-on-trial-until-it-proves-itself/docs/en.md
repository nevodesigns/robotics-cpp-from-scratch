# On Trial Until It Proves Itself: A Boot Counter, Rollback by Itself, and Why Confirming at Start-Up Is the Same as Not Checking

> A release that boots is not a release that works. The robot has to find out for itself, before anybody notices, and go back if it does not.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 20-05, 19-11, 19-02, 01-06

## The Problem

Lesson 20-05 means a power cut cannot brick the robot: it boots the old release or the new one, whole. It says nothing about whether the new one is **any good**. A release can be valid, signed, checked against its manifest, booted, and broken: a driver that does not find the sensor, a crash twelve seconds after start, a deadlock when the first message arrives. The robot has switched to it, and is 400 km from anyone who can reach it.

The answer is the one that phones and routers use: the new release is **on trial**. It gets a few tries to prove itself; if it does not, **the boot loader**, which does not depend on the release, puts the old one back. And the part that goes wrong is what "prove itself" means.

## The Concept

### A counter in front of the release

`BootState` holds the active slot, the previous slot, whether the active release is **on trial**, and the tries it has left. `start_trial` makes the new slot active with, say, 3 tries; the slot that was active becomes `previous`. At every reset the boot loader calls `on_boot`: a release not on trial just runs; one on trial with tries left has one **used up before it runs**, and one on trial with none left is **given up on**: the previous slot becomes active, the trial ends and `rolled_back` is set. The release calls `confirm` when it is healthy, and the trial is over. Three tries gave three boots of the new release and the **fourth boot was the old one**; after confirming, ten more boots stayed on the new one. Zero tries rolled back at the first boot, and a second trial forgot the old rollback.

The try is used **before** the release runs, and that is the point: a release so broken that it never gets as far as saying anything still costs one. A counter that is only decremented by a release that has started to work does not count crashes. The counter and the decision live in the boot loader, which is not the thing being tried. See `E-DEPLOY-0016`.

### The three kinds of bad release

Three tries, a health deadline of 60 s and a soak of 30 s:

| release | boots | ends on | |
|---|---|---|---|
| healthy at 20 s, never crashes | 1 | release 2 | confirmed |
| crashes at 2 s | **4** | release 1 | rolled back |
| never healthy, never crashes (hangs) | **4** | release 1 | rolled back |

The crash is easy. The hang is not: a release that sits there, up, not healthy, does not reset the robot. So something has to: a **supervisor** that resets it at the **deadline** if the health checks have not passed. Without it the hang is a robot that is up, doing nothing and not rolling back, for ever. With five tries, six boots. See `E-DEPLOY-0017`.

### Confirming at start-up is not checking

Where the release confirms itself **when the program starts**, which is the simplest thing to write, a release that is healthy at 20 s and **crashes at 40 s, every time**:

| confirmed | boots | result |
|---|---|---|
| when the program starts | **20** (all of them) | release 2, **NOT UP** |
| after healthy plus a soak of 30 s | 4 | release 1, up, rolled back |

The first confirms at second zero. The crash at 40 s is then a crash of a confirmed release, with **nothing to roll back to**, and the robot loops through all 20 boots without being up. The second does not confirm until the checks have passed **and gone on passing** for the soak, which the crash at 40 s interrupts, so it is never confirmed, and rolls back after 4. Confirming is the whole value of the trial; do it early and the trial is a decoration. See `E-DEPLOY-0017`.

### The soak, and how long a failure takes to show

A release healthy at 20 s that crashes at 120 s, every time:

| soak | result |
|---|---|
| 0 s | crash loop, release 2, 20 boots |
| 30 s | crash loop, release 2, 20 boots |
| 90 s | crash loop, release 2, 20 boots |
| 100 s | stays up, release 1, 4 boots |
| 150 s | stays up, release 1, 4 boots |

The release is confirmed at 20 plus the soak. At 90 that is 110 s, **before** the crash at 120, so the release is confirmed and then crashes for good. At 100, it would be 120: the crash comes first and the release is never confirmed. **The soak has to be longer than the time it takes the failure to show**, and nothing in the code can know how long that is: it is a number to be found from what the robot does (a memory leak that takes ten minutes to matter, a log file that fills a disk in an hour, a task that runs once a day). A short soak confirms a release that fails later and cannot be taken back. See `E-DEPLOY-0018`.

A release whose checks pass at 90 s, past a deadline of 60 s, is treated as one that never passed: 4 boots, rolled back. One healthy at 50 s was confirmed in 1 boot.

## Build It

`start_trial`, `on_boot`, `confirm` and `simulate` in `exercise/solution.hpp`. The types are given.

```
rcpp verify 20-06
```

## Use It

`rc/deploy/trial.hpp` joins `rc::deploy`.

```cpp
rc::deploy::start_trial(state, new_slot, 3);          // by the installer, after the commit of lesson 20-05
// in the boot loader, at every reset:
const int slot = rc::deploy::on_boot(state);
// in the release, when its self test (lesson 19-11) has passed and kept passing:
rc::deploy::confirm(state);
```

**What this does not do.** It is the logic, and a real boot loader (U-Boot, GRUB, a vendor's) has its own counters, kept in a place that survives a reset. A release that is **bad in a way the checks do not see** is confirmed: the health check is only as good as its list, and the self test of lesson 19-11 is where that list is made. A robot whose **safety** depends on the release is not made safe by this: a release that is healthy by its own checks and dangerous (lesson 19-10) is a failure of a different kind, and that is why a new release is driven first with the wheels off the ground. It assumes the old release still works, which a change to the **data**, the configuration or the hardware may have made false. And a rollback is a version going **backwards**, which a signed manifest and a minimum version rule must permit deliberately and refuse otherwise (a later lesson).

## What Breaks First

- **A counter kept by the release it is counting.** See `E-DEPLOY-0016`.
- **A hang, with nothing to reset it.** See `E-DEPLOY-0017`.
- **Confirming at start-up, or after too short a soak.** See `E-DEPLOY-0018`.

## Ship It

`rc/deploy/trial.hpp` holds `BootState`, `start_trial`, `on_boot`, `confirm`, `Behaviour`, `RunResult` and `simulate`.
