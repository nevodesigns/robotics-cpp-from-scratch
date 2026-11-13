# A Log That Tells You What Went Wrong: Reading a Field Run for Stuck, Slipping, Off the Path, Draining and Missing Time

> Nobody reads a million rows; a log is only as good as the questions it can answer for you.

**Type:** Build
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 27-04, 26-08, 22-09

## The Problem

A vehicle comes back from a day in the field with a log of a million rows. Nobody reads it. If something went wrong, someone searches it for the time it happened, which they have to guess, and plots the columns they think matter. A log is only as good as the questions it can answer **without** a person working through it. Lesson 22-09 made the log, 19-12 kept the seconds before a stop. This lesson **reads** it, for the faults that a ground vehicle has.

## The Concept

### Five questions of a run

Each is a test on a window of rows, and each gives a **span** with a start, an end and a size:

1. **Stuck.** For a 3 s window the vehicle was commanded to move (more than 0.1 m/s) and moved less than 0.1 m. (The test of 26-08, here read after the fact.)
2. **Slipping.** The wheels reported a speed (at least 0.2 m/s) and the ground under the vehicle went by at less than **70 percent** of it. The ground speed comes from the position, not the wheels.
3. **Off the path.** The distance from the path beyond 0.6 m.
4. **Draining.** The charge fell more than **2.5 times** as fast as its median rate over a short window.
5. **A gap.** The time between rows jumped, which means the logger, the disk or the vehicle itself stopped for a while.

The rows of a flagged window are all marked, so a span starts **when the fault did**, not three seconds after (the length of the window needed to see it). Marks closer than a second are one span, and a span shorter than 2 s is dropped: that is what keeps a bump in the noise from being a finding.

### Checked by putting faults in

A run of 300 s with **five faults** put in at known times: stuck from 100 to 108 s, slipping from 150 to 160, off the path from 200 to 206, draining four times as fast from 250 to 262, and 20 s of rows missing from 270. All **5** were found, each within **1.8 s** of where it began and ended. See `E-VEH-0124`.

### And not crying wolf

**20 clean runs** with noise on every column gave **0 false findings**. A slip that left the ground at 80 percent of the wheels was **not** reported, and one at 50 percent was: the threshold is a choice, and the lesson shows where it sits. A vehicle told to stand still, with zero commanded speed, is not stuck. See `E-VEH-0125`.

### A log that is missing time

A gap is a finding in itself. Everything else in the log is only as good as the time that is in it: if 20 s are missing, the vehicle may have done anything in them, and the other tests, run across the gap, say nothing about it. See `E-VEH-0126`.

## Build It

`spans_from_flags` in `exercise/solution.hpp`.

```
rcpp verify 27-05
```

## Use It

`rc/drive/log_doctor.hpp`.

```cpp
const auto findings = rc::drive::analyse_log(rows, rc::drive::DoctorConfig());
for (const auto& f : findings) std::printf("%s %.1f to %.1f s\n", rc::drive::kind_name(f.kind), f.t0, f.t1);
```

**What this does not do.** The five questions are the ones a ground vehicle asked in this course. It does not know why: a stuck span is one thing and a snag, a wall or a dead motor are three, and the log has not the columns for it. The thresholds are numbers for a vehicle at 1 m/s; a slower vehicle needs a longer window. It reads rows in order and in memory, so a day at 10 Hz is 360 000 rows and it is not a streaming reader. The test faults are put in at a speed and a noise level of the author's choosing, and a real log is messier (it would need the false-finding rate measured on real clean runs, and the course has none). A person still has to decide what to do about a finding.

## What Breaks First

- **A log nobody can read.** See `E-VEH-0124`.
- **A detector that finds faults in a clean run.** See `E-VEH-0125`.
- **A log with time missing.** See `E-VEH-0126`.

## Ship It

`rc/drive/log_doctor.hpp` holds `FindingKind`, `LogRow`, `Finding`, `DoctorConfig`, `kind_name`, `spans_from_flags` and `analyse_log`.
