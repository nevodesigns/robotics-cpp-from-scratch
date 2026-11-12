# The Whole Robot for a Day: Built, Gated, Shipped, Driven Through Its Faults and Reported, With the Black Box to Say What Happened

> The capstone is the robot, once, for a day, with a report. A person who was not there should be able to read the report and know what happened and what was never tried.

**Type:** Capstone
**Time:** about 170 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 21-09, 21-08, 21-07, 21-02, 20-12, 19-14, 19-12

## The Problem

Nine capstones built nine things: a line follower, a rover, an arm, a balancer, a localiser, a teleoperated robot, a mission, a test report and a release gate. The tenth is not another one. It is **the day**: the release goes through the gate and the pipeline; the robot is put to work through the situations that the hazard log said would come; whatever it does is recorded; and at the end somebody who was not there is handed a page.

The page is the product. Everything in phases 13 to 20 was in service of being able to write the five lines of it **and have each one be true**.

## The Concept

### The day

The jobs are the five scenarios of lesson 21-08, in order: an empty building; somebody steps into the path; somebody in the doorway who then goes; the link lost for four seconds; the range sensor dead. `run_day` runs them under a **policy** (the `DeliveryConfig` of 21-02, as shipped by 21-09) and keeps a **black box** (19-12) on the day's own clock:

- an event of code 1 when a job starts, with the job's number;
- an event of code 3 with the number of bumps, if there was contact;
- and an end event: code 2 with the time if the job reached the goal, code 99 with the stop causes if the supervisor stopped it, and code 4 if it simply did not arrive.

### What the rover with its safety layer did

```
Rover 1.5.0 (gate passed)
Day: 5 jobs, 3 delivered, 2 stopped by the supervisor (link, sensors), 0 bumps, 2 operator resets
Black box: 10 events, chain verified
Before the first stop: 2:12.3 1:1.0 2:22.6 1:2.0 2:27.2 1:3.0
Open: hazards with no scenario: G3, G5, G6, G7
```

Read each line, since each is a claim that a test makes:

1. **Which release, and whether the gate passed.** 1.5.0, through the gate of 21-09.
2. **The totals.** Five jobs, **3 delivered** (the empty building, the person stepping out, the doorway), **2 stopped by the supervisor** with causes `link` and `sensors`, **0 bumps**, and **2 operator resets**: a latched stop needs a person (19-02, 19-10), and the day cost two of them.
3. **The black box.** 10 events (a start and an end for each of five jobs) and the chain verified (19-12). Nobody edited the log between the robot and the report.
4. **What led to the first stop.** The six events before the first code 99: job 0 ended at 12.3 s, job 1 started, ended at 22.6 s, job 2 started, ended at 27.2 s, job 3 started: and then the stop, in job 3, the link. The numbers on this line are the times of 21-02's three deliveries.
5. **What is open.** G3 (rolls over), G5 (battery), G6 (overheats) and G7 (leaves its area): the hazards that **no scenario in the suite is evidence for**. It does not say the robot is safe against them. It says nobody has shown it.

The fifth line is the one a reader trusts the report for. A report that ended on the third line would be a report of a good day. See `E-CAP-0028`.

### The same day without the layer

The policy with the limiter and supervisor taken out, **and the gate refusing it** (as it did in 21-09):

```
Rover 1.5.0-unsafe (gate REFUSED)
Day: 5 jobs, 5 delivered, 0 stopped by the supervisor (none), 288 bumps, 0 operator resets
Black box: 12 events, chain verified
Before the first stop: none
Open: hazards with no scenario: G3, G5, G6, G7
```

| | with the layer | without |
|---|---|---|
| delivered | 3 of 5 | **5 of 5** |
| stopped by the supervisor | 2 | 0 |
| bumps | 0 | **288** |
| operator resets | 2 | 0 |

The unsafe robot **had the better day by the measure that a customer sees first**, and every other number is worse: 288 ticks of contact (14 seconds against people), no stops where two were needed (the link was lost and the sensor died and it drove on). The two it did not deliver, with the layer, are the two where **stopping was the right answer**. A robot is judged by deliveries until the first time somebody is hurt. See `E-CAP-0029`.

### Shipping, and the refusal

The good candidate through `ship_rover`: gate passed, pipeline of 20-12 passed, **running 1.5.0, confirmed**. The unsafe candidate: gate **refused**, not deployed, the robot **still on 1.4.2**. And the day on 1.4.2 after the refusal is the same day as on 1.5.0, with the same policy: 3 delivered, 2 stopped, 0 bumps. A refused release is not a bad day: it is the day the robot had before. See `E-CAP-0030`.

## Build It

`count_delivered`, `count_stopped` and `render_day` in `exercise/solution.hpp`. The day, the log and the ship are given: they are the integration.

```
rcpp verify 21-10
```

## Use It

`rc/capstone/robot_day.hpp` is the last header of `rc::capstone`.

```cpp
const ShipResult s = ship_rover(robot, policy, rover_release("1.5.0", policy, triple), false);
const DayResult d = run_day(policy, "1.5.0", s.gate_passed);
std::cout << d.report;
```

## Show It

This is the one to **put at the top of the README**: the five lines of the day, once with and once without the layer, and under them the line `Open: hazards with no scenario: ...`. Below it, the commands that produce each number: `rcpp verify 21-10`. A visitor who reads only that page learns what the robot does, what it does when things go wrong, what has not been tried, and that you counted.

**What this does not do.** It is **one day, five jobs, one building, one policy**, all simulated: a sample and not a measure of reliability. The jobs are the scenarios of the suite, so the day can only show what the suite tries, and the report says as much in its last line. The stop causes are those of the supervisor, and the operator reset is a count, not a person. The black box has 10 events and a real day has millions, and it is in memory and not on a disk that survives a power cut. The deploy is the model of lesson 20-12, with no cross compiler, no signature and no radio. And the report is **a claim made by the code that produced it**: an assessor will want to run the scenarios themselves, which is why every number has a command next to it.

## What Breaks First

- **A report that ends on the good news.** See `E-CAP-0028`.
- **A day that is judged by deliveries.** See `E-CAP-0029`.
- **A refused release treated as a failure of the robot.** See `E-CAP-0030`.

## Ship It

`rc/capstone/robot_day.hpp` holds `count_delivered`, `count_stopped`, `DayResult`, `render_day` and `run_day`.

This ends phase 21 and the curriculum's plan of 320 lessons: a robot that was built from the pieces of the earlier phases, gated, shipped, run through its faults and reported, and a report whose every number comes from a test that passes and whose last line says what no test has tried.
