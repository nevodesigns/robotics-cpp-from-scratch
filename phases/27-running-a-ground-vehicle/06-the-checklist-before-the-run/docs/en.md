# The Checklist Before the Run: Ten Checks as Code, the Ones That Block and the Ones That Warn, and a Number That Is Not a Number

> A checklist that a person can talk their way past is a suggestion.

**Type:** Build
**Time:** about 90 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 27-04, 27-05, 22-16

## The Problem

A vehicle goes out on a job with a flat battery, a stale scan, a release from last week, and an emergency stop nobody has tried since the day it was fitted, because the person who sent it out had other things in their head. A checklist on paper is a list of things to remember to look at. A checklist in **code** looks at them, every time, and says **no**.

(Lesson 22-16 was the bring-up checklist of a board, from nothing. This is the shorter check before **each** run of a vehicle that is already working.)

## The Concept

### Ten checks, with numbers

Each check reads one fact about the vehicle and compares it to a limit. They come in two kinds, and which is which is a decision:

| check | kind | what it asks |
|---|---|---|
| energy | **block** | the charge left covers the mission (there and back, lesson 27-04) plus the reserve |
| emergency stop | **block** | it was tried in the last 24 hours |
| position | **block** | the position is known to within 0.5 m |
| scan | **block** | the newest scan is under 0.2 s old |
| geofence | **block** | it is loaded |
| release | **block** | the release on the vehicle is the one the mission was built for |
| link | warning | the round trip to the station is under 0.5 s |
| log space | warning | 500 MB free |
| clock | warning | the clock is within 0.1 s (a log with the wrong time is a log that cannot be read, 27-05) |
| motor temperature | warning | under 60 degrees |

A **block** means the vehicle does not go. A **warning** means a person may decide to go, and says so. On a good state all **10** passed. Making each fail alone blocked the run **6** times and warned **4** times, which is the 6 and 4 of the table.

### A block cannot be argued with

A person can **override a warning**, but must give a **reason**, and the reason is kept: an override of the link check with the reason "line of sight, no teleoperation today" went. An override with **no reason** was refused. An override of a **block** was refused: it is in the report as refused. The checklist that has an override for everything is the paper one again. See `E-VEH-0127`.

### A number that is not a number

A sensor that has failed gives a number that is not a number, and in C++ **a comparison with it is false**: `age <= limit` is false, which is good (it fails) for that way round, but `age > limit` is false too, which would **pass** a check written the other way. Every check here is written as "ok when the value is within the limit", so that a value that is not a number fails. A state in which all of the numbers were not numbers failed **8** checks (eight of them are numbers; the geofence and the release are not). See `E-VEH-0128`.

### Exactly at the edge

A vehicle with 5.00 Wh left and a mission that needs 4.50 Wh and a reserve of 0.50 **passed**, and one with 4.99 **failed**. The check is `>=`, not `>`, and the test is run at the edge. See `E-VEH-0129`.

## Build It

`run_checks` in `exercise/solution.hpp`.

```
rcpp verify 27-06
```

## Use It

`rc/drive/field_checklist.hpp`.

```cpp
const auto results = rc::drive::run_checks(state, limits);
const rc::drive::Decision d = rc::drive::decide(results, overrides_from_the_operator);
std::puts(rc::drive::report(results).c_str());
if (!d.go) refuse_to_start();
```

**What this does not do.** It checks what it is told. The values come from the rest of the vehicle, and a position that is wrong but confident passes. The limits are numbers to be set for the vehicle and the job, not these. Ten checks are the ones this course has: a real vehicle has more (tyre pressure, a camera lens that someone has wiped, the gate that must be shut). The checks run once at the start; a vehicle that goes bad in the middle of a run needs the supervisor of 19-10, and the degraded modes of 27-07. Nothing here proves the emergency stop works: it only says it was tried.

## What Breaks First

- **A checklist that can be overridden.** See `E-VEH-0127`.
- **A check that a bad sensor passes.** See `E-VEH-0128`.
- **A check off by one at the edge.** See `E-VEH-0129`.

## Ship It

`rc/drive/field_checklist.hpp` holds `Severity`, `VehicleState`, `CheckLimits`, `CheckResult`, `make_check`, `num`, `run_checks`, `can_go`, `Override`, `Decision`, `decide` and `report`.
