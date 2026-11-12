# An Arm That Picks and Places: Inverse Kinematics, Joint Limits, a Timed Cycle, and a Speed Cut by the Distance to a Person

> A cobot is an arm that is allowed to work next to a person because it slows down when the person comes near. This lesson is that sentence as code.

**Type:** Capstone
**Time:** about 160 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated arm)
**Prerequisites:** 21-02, 19-14, 19-05, 19-10, 13-14, 13-05, 13-02

## The Problem

The third capstone is the other kind of robot: an arm. Everything it needs is in phases 13 and 19: the inverse kinematics of lesson 13-02, the joint limits of 13-05, the timing of 13-14, and the limiter and supervisor. What the capstone adds is the **cycle**, a loop of picks and places, and the question that decides whether it can work beside a person: what does the arm do when somebody walks up to the pick point?

## The Concept

### Poses that fit

`arm_tip` is the forward kinematics of a two link arm of 0.4 m and 0.3 m. `arm_pose_for` goes the other way, with the care of lesson 13-05: of the two elbow branches it takes the **elbow down** one if it is inside the joint limits, **otherwise the elbow up one**, otherwise there is none. All the points of the loop came back from the angles to the same point to within 1e-9 and inside the limits. A point at (1.0, 0.5) is 1.12 m away and out of reach, one at (0.02, 0.02) is closer than the 0.1 m the two links allow, and with the elbow held to [-2.4, 0.5] the point (0.5, 0.15) took the **other branch** (q2 of -1.48), and with it held to [-0.5, 0.5] there was none. See `E-CAP-0007`.

### A cycle

`plan_cycle` gives a loop through the points: home, above the pick, the pick, above the pick, above the place, the place, above the place, and back. Seven points, seven legs, each a move of both joints **in step** (taking as long as the slower, 13-14) with a trapezoid of speed: **3.91 s** for the loop at the joints' own limits of 2 rad/s and 6 rad/s^2. A loop with a point out of reach is `not ok` and names the point: `bad point 3`, found before anything moves, which is when to find it.

### A speed limit near people, and what it costs

The supervisor's top speed is the speed limit of the **tip**. The loop with nobody there:

| limit | first cycle | fastest tip | cycles in 60 s |
|---|---|---|---|
| none (5 m/s) | 4.10 s | 1.14 m/s | 15 |
| 0.8 m/s | 4.28 s | 0.80 m/s | 14 |
| 0.25 m/s | **7.63 s** | 0.30 m/s | 8 |

A limit of 0.8 m/s costs **4 percent**: the arm's own tip speed is 1.14 at most, and only the quickest parts of the loop are slowed. A limit of 0.25 m/s, the sort of figure a standard may ask for a person's contact with a hand, costs **86 percent**, almost twice the cycle. And look at the last column of that row: the tip reached **0.30 m/s** against a limit of 0.25. The speed is scaled by a fraction that can only move at a limited rate (6 a second down), and when a leg begins quickly the arm is a little over before the ramp catches up. A limit held by a ramp is a **target and not a wall**, and any claim about "the arm never exceeds 0.25" has to say how it was measured, or the margin has to go into the number. See `E-CAP-0008`.

### Somebody stands at the pick point

A person walks up to a point 8 cm from the pick point at 2.5 s and stays until 9 s. The arm's own limiter turns the distance into a speed (the tip at 2 m/s^2 of braking, 0.1 s of latency, 0.15 m of margin: lesson 19-04's sum, with an arm's numbers), and gives it to the supervisor.

| arm | closest | ticks in contact | cycles in 30 s |
|---|---|---|---|
| monitored | **0.150 m** | **0** | 6 (7 with nobody there) |
| not looking | 0.077 m | **66** | 7 |

The monitored arm came no nearer than **0.150 m**, exactly the margin, held there until the person left and then finished its cycle, and it lost **one cycle in thirty seconds** for it. The one that does not look carried on to **0.077 m** and was in contact for **66 ticks of 10 ms**, 0.66 s. It had one more cycle to show for it. The honest comparison is the one in the table: the cost of the safety is a cycle, and the cost of its absence is a person's hand. See `E-CAP-0008`.

### The sensor that fails, and the loop that opens

The person sensor reads NaN from 3 s: the supervisor sees three ticks of nothing and **stops the arm**, cause `sensors`, with the tip **0.011 m** further on at 3.17 s. The limiter alone would have set a speed of zero (a NaN is a blocked path); the supervisor makes it a latched stop with a reason. An e-stop loop that opens at 2 s: cause `e-stop`, the tip went **0.044 m** further at the 0.8 m/s limit and **0.065 m** with the limit raised to the arm's full 1.14 m/s: a stop takes distance in proportion to the speed it started from, which is why the speed limit near people matters twice, once to hurt less and once to stop shorter (19-04). See `E-CAP-0009`.

## Build It

`arm_tip`, `arm_pose_for`, `human_at` and `plan_cycle` in `exercise/solution.hpp`. The speed scaling, the limiter and supervisor wiring and the cell loop are given.

```
rcpp verify 21-03
```

## Use It

`rc/capstone/pick_and_place.hpp` joins `rc::capstone`.

```cpp
const ArmCell cell = default_arm_cell();
const ArmPlan plan = plan_cycle(cell, {{0.35, 0.35}, {0.5, 0.25}, {0.5, 0.15}});
if (!plan.ok) return report_bad_point(plan.bad_point);
const ArmRunResult r = run_cell(cell, plan, human, ArmRunConfig(), 30.0);
```

## Show It

Show the **cycle time against the speed limit**: 4.10, 4.28 and 7.63 seconds is a table an engineer reads in ten seconds and a cell designer uses. Show the row where the limit is exceeded (0.30 against 0.25) and what you did about it, and the pair of rows with and without the monitor. A portfolio that says the arm "slows near people" proves nothing; one that shows by how much, at what price, and with what margin, does.

**What this does not do.** It is two links in a plane with no mass: the real arm has inertia, so its stopping distance depends on its pose and its load, a gripper that holds a part, a collision with the table, and a third dimension in which a person's head is. The person is a point with a straight-line walk and a sensor that reads the distance to the **tip**, while a person is hurt by the **whole arm**, and the distance that matters is to the nearest part of it. The speed limit applies to the tip only, and the elbow can move faster. The pick and the place are points with no object. There is no gripper force limit or contact detection, which is what a collaborative arm actually relies on. **An arm that works beside people is certified against a standard, by measuring contact forces on a test device, and this is a model of the logic and not of that.**

## What Breaks First

- **A point that was never checked against the joint limits.** See `E-CAP-0007`.
- **A speed limit that is a target and not a wall.** See `E-CAP-0008`.
- **A stop distance that nobody measured at the speed it has.** See `E-CAP-0009`.

## Ship It

`rc/capstone/pick_and_place.hpp` holds `ArmCell`, `default_arm_cell`, `ArmXY`, `arm_tip`, `arm_pose_for`, `ArmPlan`, `plan_cycle`, `pose_on_leg`, `ArmHuman`, `human_at`, `ArmRunConfig`, `ArmRunResult` and `run_cell`.
