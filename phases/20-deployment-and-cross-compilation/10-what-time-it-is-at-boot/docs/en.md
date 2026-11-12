# What Time It Is at Boot: A Clock That Starts in 1970, the Jump When It Is Set, and Records That Sort Themselves Wrongly

> The robot does not know what time it is. It knows how long it has been awake.

**Type:** Build
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 20-09, 19-02, 19-12, 03-18

## The Problem

A laptop knows the time. A robot, very often, does not: the small boards that robots are built on have **no battery for the clock**, and wake at the time the image was built, or the last time the clock was saved, or 1 January 1970. Some seconds or minutes into the boot the network comes up, the clock is **set** to the real time, and the wall clock **jumps**, by decades, in one step.

Everything in the robot that uses the time is now in a different state from the moment before: the timeouts, the log stamps, the certificate checks, the schedules. Lesson 19-02 already met the other direction, a clock going back, and made a heartbeat on a clock that goes backwards stale. This lesson is the other half: the robot's clock **jumps**, and the answer is to know which of its clocks can.

## The Concept

### Two clocks, one that can jump

The **monotonic** clock counts the time since boot and only ever goes forward. It cannot tell you the date. The **wall** clock tells the date and can be set at any moment, forward or back, by anyone: the network, an administrator, a bad driver. A `BootClock` knows both: it stamps a moment with the time since boot (`mono`), the wall time, and whether the wall clock **has been set** (`synced`). It starts at an initial wall time (here 1970, a counter) and is set once with `sync(mono, true_wall)`.

A stamp at 12 s read **12** (seconds after 1970) with `synced` false. The network answered at 40 s; that moment is now known to have been 1794699972, 28 seconds before the answer. `wall_of` gives the real time of any moment since boot, early ones included, once the clock is set, and a NaN before it: it will not guess. See `E-DEPLOY-0028`.

### How big a jump

`jump` is how much wall time passed minus how much time since boot passed. Setting the clock at 40 s: **1794699960 s**, which is **56.9 years**, while 0.2 s of real time went by. A slew (the network nudging the clock by 20 percent for a second): **0.20 s**. An hour put back at a change of daylight saving by a careless setup: **-3600 s**. `classify` calls anything above a threshold a step, forward or backward; exactly the threshold is not a step.

### A timeout on the wall clock

A heartbeat 0.2 s ago, a timeout of 0.3 s, and the clock set in between:

| timeout measured in | answer |
|---|---|
| wall time | **expired**, a stop for nothing |
| time since boot | fine |

Set the clock the other way, an hour back, with a heartbeat 100 s old: wall time says **fine** (the elapsed time is negative), and time since boot says expired. A watchdog on the wall clock fires at the moment the time is set, **every boot, as the network comes up**, and cannot fire at all when the clock is put back. **Every interval, timeout and rate in the robot is measured in time since boot.** The stop path of lesson 19-02 uses the robot's own clock for that reason, and it must be the monotonic one. See `E-DEPLOY-0029`.

### Records that sort wrongly

Two boots. Boot 1 set its clock at 30 s and its records carry 2026 time. Boot 2's early records, before its clock was set, carry **1970** time. Sorted by the wall clock:

```
boot 2: starting
boot 2: drivers up
boot 1: network up
boot 1: motor fault
boot 1: power lost
boot 2: network up
```

**5 of the 6** are in the wrong place: the start of the second boot comes before the whole of the first. An incident review that reads this sees a robot that **started, and then had a motor fault**. `sort_records` orders by **boot** and then by **time since boot**, which never lies: boot 1's three records, then boot 2's, with equal times keeping the order they came in. The wall time is for a human to read, and after the sync `wall_of` can put a real one on the early records. Lesson 19-12's black box has a sequence number for the same reason. See `E-DEPLOY-0030`.

### A certificate before the clock

A certificate valid from 30 days ago for a year, at 10 s into the boot, on a 1970 clock: it is "not yet valid" by 56 years. A secure connection to the fleet server fails at every boot, until the clock is set, and the clock is set **by a network service that may be what needs the connection**. `check_validity` answers `unknown` before the clock is set and does not judge; after it, `valid`, `not_yet` and `expired` are told apart, with both ends inclusive. The robot has to be able to ask for the time without a certificate, and then ask for everything else.

## Build It

`BootClock::stamp`, `sync` and `wall_of`, `jump`, `classify`, `sort_records` and `check_validity` in `exercise/solution.hpp`.

```
rcpp verify 20-10
```

## Use It

`rc/deploy/bootclock.hpp` joins `rc::deploy`.

```cpp
rc::deploy::BootClock clock(0.0);                         // the board has no battery
const auto stamp = clock.stamp(time_since_boot());       // every log line, and the black box of lesson 19-12
clock.sync(time_since_boot(), time_from_network());      // once, when the network answers
if (rc::deploy::check_validity(not_before, not_after, clock, time_since_boot()) == rc::deploy::Validity::unknown) wait_for_time();
```

**What this does not do.** It models one set of the clock. A real one is set **more than once** (the network again, a GPS, an operator), is **slewed** gently by a good time daemon so as not to jump, and **drifts** by a few seconds a day between sets, which makes `wall_of` right only to the accuracy of the source. It does not read a time source, and getting the time without a certificate (NTP, or the time from a GPS) is its own piece of work. It assumes the monotonic clock really does not stop: on many systems it does not count the time the machine was **suspended**, and a robot that sleeps is a robot whose time since boot is shorter than its real time. And it does not decide **what to do** with the records stamped before the set: rewriting a log is a change to a record that lesson 19-12 would not allow, and the answer there is to keep both stamps.

## What Breaks First

- **Wall time used before it was set.** See `E-DEPLOY-0028`.
- **A timeout on the wall clock.** See `E-DEPLOY-0029`.
- **Records sorted by the wall clock.** See `E-DEPLOY-0030`.

## Ship It

`rc/deploy/bootclock.hpp` holds `Stamp`, `BootClock`, `jump`, `ClockStep`, `classify`, `Record`, `sort_records`, `Validity` and `check_validity`.
