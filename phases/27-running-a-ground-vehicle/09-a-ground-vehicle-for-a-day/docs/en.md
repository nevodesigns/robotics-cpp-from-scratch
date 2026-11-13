# A Ground Vehicle for a Day: The Checklist, the Mission, the Field, the Modes, the Unsticking and the Log Reader, on One Scripted Day, With and Without Them

> Each part was tested alone; the day is where they have to agree.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 27-01, 27-02, 27-03, 27-04, 27-05, 27-06, 27-07, 27-08

## The Problem

Each of the last eight lessons made one part of a vehicle that works for a living, and tested it alone. A day is where they meet: the checklist lets it out, the mission sends it, a person steps in front of it, a wheel catches, the GNSS goes, the scanner flickers, there is sand on the road, and somebody offers an update. The parts have to **agree about the speed**, about **who is in charge**, and about **what was true when they were not looking**.

## The Concept

### One script, one road

The day is **scripted** so that it is the same every time, and so that the lesson can say what each part did. A road of 240 m with the charger at one end and three sites (at 80, 160 and 240 m, 20 s at each) and home. Sand from 100 to 130 m. The events:

| when | what | the part that meets it |
|---|---|---|
| before leaving | a battery of 2.0 Wh | the checklist (27-06) with the energy of the route over its sand (27-04) |
| 20 to 40 s | a person in the road at 40 m | the protective field (27-03) |
| on the way to 160 m | a snag at 150 m, which lets go when the vehicle has backed up 0.4 m | the unsticking ladder (26-08) |
| 110 to 210 s | no GNSS | the modes (27-07): reduced, and the sigma growing |
| 250 s | a release offered, in the middle of a job | the release check (27-08) |
| 300 to 303 s | no scanner | the modes: a safe stop, and a climb back |
| the end | the vehicle parked at the charger, a release offered again | the release check |

### The speeds are joined by taking the least

`combine_speed` is the one rule: the speed the mission wants, no more than the cap of the mode, and then the protective field has the last word (a creep or a stop). The unsticking ladder sits on top of that: when the vehicle has been commanded and has not moved, it overrides the command. The order matters and is the lesson's one decision: the field before the ladder, so that a vehicle backing up does not back into the person who made it stop.

### With them, and without

With the layers, the vehicle did **4 of 4 legs** in **470 s** and touched **0** people. The field stopped it for **11.4 s** at the person. It took **2** unsticking attempts to get past the snag. It spent **130 s** in a reduced mode or creep (the GNSS, and the climb back from the scanner) and **12.9 s** in a safe stop. It used **1.52 Wh** of **2.00** and got home with the reserve. It **refused the release** in the middle of the job and **took it at the dock**. See `E-VEH-0136`.

Without the layers (the same vehicle, the same events, no checklist, no field, no modes, no ladder) it completed **2 of 4**, drove into the person (**1** contact), pressed against the snag until the timeouts of its legs let the mission give up on them (**975 s** in all, twice as long), and ran out of charge **150 m** from the dock. The vehicle was not broken: it did what it was told. See `E-VEH-0137`.

### What the log said

The day's log, read by the doctor of 27-05, held **5** findings. Two of them were the snag: stuck, in two pieces either side of the wait of the ladder, each with its time. Two were the sand, where the charge fell faster than usual (a drain: true, and the energy of 27-04 had said so). One was **not** a fault: the back and forth of the recovery, which moved the wheels and not the ground, was called slip, because the doctor judges slip by the net movement of a window. That is a limit of the doctor, and it is in the log of what to improve. The stop for the person is not a finding (the vehicle was told to stop, and was stopped). No gap, no off path. A finding that the vehicle reported for itself is a finding that nobody needs to look for. See `E-VEH-0138`.

### A vehicle with too little battery stays at the dock

With 0.5 Wh the checklist blocked on **energy**, and only energy; the vehicle did not move.

## Build It

`combine_speed` in `exercise/solution.hpp`.

```
rcpp verify 27-09
```

## Use It

`rc/drive/ugv_day.hpp`.

```cpp
rc::drive::DayConfig cfg;
const rc::drive::DayResult r = rc::drive::run_day(cfg, true);
std::puts(rc::drive::day_report(r).c_str());
```

**What this does not do.** This is a **simulation of a day** on a road one dimension long, with a point vehicle and scripted events, and it says nothing about what a real day is like: nothing here has been on a real vehicle (the whole of this course has been run against fakes and a simulator, and the fakes agree with themselves). The road has no turns, so the phase 26 planners are not in it; the position is a distance. The events do not overlap, and a real day's do. The energy per metre is computed, not measured. Nothing about the weather, the people, or what a bad day is. The layers are the ones of this phase; the supervisor of 19-10 and the safety case of 22-17 are what a real vehicle must also have, and a certified safety function is a device and not a header.

## What Breaks First

- **Parts that were tested alone and disagree.** See `E-VEH-0136`.
- **A vehicle with no layers.** See `E-VEH-0137`.
- **A log nobody reads.** See `E-VEH-0138`.

## Ship It

`rc/drive/ugv_day.hpp` holds `combine_speed`, `DayConfig`, `DayResult`, `run_day` and `day_report`: the last header of `rc::drive`. With it the ground course (phases 23 to 27) is complete: a vehicle that is modelled, perceives, knows where it is, plans and follows, and is run.
