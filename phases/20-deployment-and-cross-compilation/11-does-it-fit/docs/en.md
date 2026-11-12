# Does It Fit: CPU, Memory and Flash Budgets for a Small Board, the Peak a Short Test Never Sees, and What Heat Takes Away

> It ran fine on the development board, which had four times the memory and a fan.

**Type:** Build
**Time:** about 110 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (numbers from a profile)
**Prerequisites:** 20-10, 20-09, 20-05, 07-03

## The Problem

Everything built so far runs on a laptop. The robot's computer has four slow cores, a few gigabytes of memory, a small flash and, sealed in a box in the sun, a tendency to **throttle**: to slow itself down when it gets hot. Whether the software **fits** is a question with an answer in numbers, and the numbers are cheap to get and are usually not got until the first field day, from a robot that runs out of memory at 2 pm.

Three mistakes account for most of the answers that are wrong. The profile is taken from a **short test** that did not see the peak. The sums are made with **no headroom**. And they are made at the speed the board runs at on a **cool bench**.

## The Concept

### A profile that keeps the peak

A program's profile is the CPU it uses on average, the CPU at its **peaks**, and the memory it reaches at most (the resident memory, which is what runs out). `percentile` is the nearest rank: sort the samples and take the one at position `ceil(q * n)`. `profile_from_samples` keeps the mean, the percentile you ask for (1 for the largest) and the largest memory.

A planner that uses 0.4 cores while it waits, and 2.0 for a moment when it replans, which is once in 70 seconds. Measured for **60 s on the bench**, with none in the window: average 0.40, **peak 0.40**. Measured for **an hour in the field**: average 0.42, **peak 2.00**, and the 99th percentile also 2.00. The bench profile has the planner at **a fifth** of its real peak. The average barely moves (0.40 against 0.42), which is why a sum of **averages** looks fine: it is the peaks that make the robot miss a deadline, and they come in the middle of an obstacle. See `E-DEPLOY-0031`.

### Six programs on a small board

A board of 4 cores, 4096 MB and 16 GB of flash. Six programs: perception (1.2 average, 2.0 peak, 900 MB), planner (0.6, 1.5, 500), drivers (0.3, 0.5, 150), bridge (0.2, 0.4, 100), logger (0.1, 0.2, 80) and the system itself (0.2, 0.3, 300). Flash is two releases of 3000 MB, 2000 MB of data and the 200 MB log of lesson 20-09. `check_fit` against the **allowances** (the averages may use 70 percent of the cores, all the peaks together 125 percent of them, memory 80 percent, flash 90 percent):

| | used | allowed |
|---|---|---|
| CPU average | 2.60 | 2.80 |
| CPU peaks together | 4.90 | 5.00 |
| memory | 2030 MB | 3277 MB |
| flash | 8200 MB | 14746 MB |

It fits, with **0.20 cores** of the average and **0.10** of the peaks to spare: not much, and the allowances are why the answer is not simply "2.6 is less than 4". A system run to 100 percent of its memory does not slow down; a program is **killed**. A CPU run at 100 percent on average misses deadlines whenever anything coincides. Flash at 100 percent cannot write the next update. See `E-DEPLOY-0032`.

### What heat takes away

A small board in a sealed box in the sun throttles, and keeps 60 percent of its speed. At 60 percent it is a board of **2.4 cores**. The same six programs, throttled:

| | used | allowed |
|---|---|---|
| CPU average | 2.60 | **1.68** |
| CPU peaks together | 4.90 | **3.00** |

Over on **both**. Cool, it fits; hot, it does not, and the robot that did all of its tests on a cool day misses its deadlines on the warmest, which is the day it is working outdoors. The number to design to is the **hot** one. And a program that is late because of heat is a program whose lateness has no cause in the code: lesson 19-08's thermal monitor and this check are two ends of the same fact. See `E-DEPLOY-0032`.

### One more program

Add a map server of 1500 MB (it is idle most of the time, so its CPU is small): memory goes to **3530 MB against 3277** allowed, and only memory is over; the finding says `ram`, with the number used and the number allowed. Everything else still fits. Memory is the resource that does not degrade: it is fine, and then a program is killed, and which one is the kernel's choice. See `E-DEPLOY-0033`.

### How large a release

`max_release_mb` is the room that is left for releases: 90 percent of the flash, less the data and the logs, divided by the number of slots. With 16 GB of flash, 2 GB of data and 200 MB of logs: **6273 MB** with two slots and **12546 MB** with one. Two slots are what let lesson 20-05 work, and they **halve** what a release may be. A release 100 MB past 6273 MB gives a `flash` finding; one 1 MB under fits. With no slots, or no room left, the answer is 0. It is the number to put at the top of the build: **fail the build** when the release is bigger. See `E-DEPLOY-0033`.

## Build It

`percentile`, `profile_from_samples`, `check_fit` and `max_release_mb` in `exercise/solution.hpp`. The types and the budget are given.

```
rcpp verify 20-11
```

## Use It

`rc/deploy/fit.hpp` joins `rc::deploy`.

```cpp
const rc::deploy::Proc planner = rc::deploy::profile_from_samples("planner", cpu_samples, ram_samples, 1.0);
const rc::deploy::Fit fit = rc::deploy::check_fit(board, procs, release_mb, 2, data_mb, log_mb, rc::deploy::Budget(), /*hot=*/true);
for (const auto& f : fit.findings) std::cerr << f.resource << ": " << f.used << " against " << f.allowed << "\n";
```

**What this does not do.** It adds up numbers that somebody had to measure, and a **peak** is only as good as the longest run that was measured: an hour in the field is better than a minute and worse than a week, and the largest sample is a **lower** bound on the real peak. It treats CPU as a pool of cores, and a program that is one thread has a limit of 1.0 however many cores are idle, which is a constraint of its own (lesson 07-15 on priorities). It does not know about **memory bandwidth**, cache, the GPU, I/O, or a USB bus shared by three cameras, any of which may be the limit. It does not know the board. The throttling factor is a measured number for **that** board in **that** box at **that** temperature, found by running it hot (a heat gun, a sealed box, a stress test) and watching its clock, and the 60 percent here is an example. And a fit on paper is not a fit: run the whole robot on the real board, hot, for a shift.

## What Breaks First

- **A profile from a test too short to see the peak.** See `E-DEPLOY-0031`.
- **Sums with no headroom, on a cool bench.** See `E-DEPLOY-0032`.
- **A release that was never checked against the flash it has to share.** See `E-DEPLOY-0033`.

## Ship It

`rc/deploy/fit.hpp` holds `Board`, `Proc`, `percentile`, `profile_from_samples`, `Budget`, `Finding`, `Fit`, `check_fit` and `max_release_mb`.
