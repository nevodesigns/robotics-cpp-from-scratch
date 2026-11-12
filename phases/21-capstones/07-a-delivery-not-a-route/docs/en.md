# A Delivery, Not a Route: A Mission State Machine With a Battery Check, a Timeout for a Blocked Road, and a Pause That Does Not Start Over

> A route is where to go. A mission is what to do when the route is not available, the battery is not enough, or somebody tells the robot to stop.

**Type:** Capstone
**Time:** about 140 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated road)
**Prerequisites:** 21-06, 19-14, 19-08, 19-10, 01-06, 17-20

## The Problem

The seventh capstone is about the layer **above** driving. The robots so far got from one place to another. A delivery robot is asked for something larger: pick up a load, take it 100 m, put it down, come home, and do it with a battery that may not be enough, a road that may be blocked, and a link that may go. Each of those is a decision, and a robot with no place in its code for them makes them anyway, by accident.

The structure is lesson 01-06's: a state, and events that change it. The question that each state has to answer is **what does the robot do on every event**, and the test is a property over many worlds. Its pieces: the battery of 19-08, the supervisor of 19-10, and the restart-from-where-it-was idea of 17-20.

## The Concept

### The states

`idle`, `loading`, `to drop`, `unloading`, `to home`, and the ends: `done`, `aborted`, `needs help`. Pressing go does one thing: from `idle` it starts only if the battery covers the whole round trip and a reserve. At 0.1 Wh/m and 100 m to the drop the round trip is 200 m, **20 Wh**, so **25 Wh with a 5 Wh reserve**: the robot refuses to start on 24.9. A refusal is an end state of its own, `aborted` at 0 s, with the load not taken, which is the cheapest place to find out.

| battery | result |
|---|---|
| 40 Wh | done, delivered, 19.99 Wh left after 404 s |
| 26 Wh | done, 5.99 Wh left |
| 24 Wh | aborted at once |

### Checking on the way

A battery that was enough at the start may not be on the road: a headwind, a slope, cold. `must_turn_back` asks, at each tick, whether what the battery would have left on getting home from here is below the reserve, using the rate the robot **expects**. With 26 Wh and the robot using more than it expects:

| really used (Wh/m) | checking on the way | checking at the start only |
|---|---|---|
| 0.10 | done, 5.99 Wh left | done, 5.99 Wh left |
| 0.12 | **aborted, home, 3.07 left** (turned back, not delivered) | done, home, 1.98 left |
| 0.15 | aborted, home, 0.79 left | **STRANDED 26.7 m out** |
| 0.18 | STRANDED 5.55 m out | STRANDED 55.6 m out |
| 0.25 | STRANDED 16.0 m out | STRANDED 96.0 m out |

Read it row by row. At 0.12 the check **turned back a delivery that would have made it**, since it judges by the expected rate, and the unchecking robot got through with 1.98 Wh. At 0.15 the check **saved the robot**, which got home with 0.79 Wh, while the other was stranded 26.7 m from home having delivered the load and unable to come back. At 0.18 the reserve is not enough for either, but the one that checked was **5.55 m** from home and the other **55.6 m**. A check on the way costs deliveries and saves robots, and the reserve is the number that says how wrong the estimate may be. See `E-CAP-0019`.

### A blocked road

A blocked road is waited for, for a timeout of 10 s, and then the mission turns back.

| the road is blocked | result |
|---|---|
| for 8 s | done, delivered, waited 8.0 s |
| for 12 s | **aborted**, not delivered, waited 10.1 s, back at home with 35.2 Wh |
| ahead and behind | **needs help**, at 19.1 m from home |

The robot that turns back has the load still on it: `aborted` is not "delivered nothing", it is "came home with the parcel", which is a different message to the customer. And a road blocked **both ways** has no safe place to go and ends in a state of its own, `needs help`, stopped where it is, instead of waiting for ever or doing something clever. A state machine's value is that every combination has an answer somebody chose. See `E-CAP-0020`.

### A pause that does not start over

The supervisor stops the robot (here, a lost link from 30 s to 60 s, a person pressing reset 1 s after it returns). The mission **holds**: the state, the position and the timers are kept, time held is counted, and nothing times out. It was done at **435 s** against 404 s with no loss: **31.0 s** of hold, the whole difference, and **no energy** spent holding. A mission that started again after a pause would carry the load back to the start of the road, and one that counted the pause as waiting for a blocked road would turn back when the link returned. With the link lost for ever the mission was held at 14.0 m for 570 of 600 s: it does not decide anything for itself, which is right. See `E-CAP-0021`.

### Over many worlds

72 worlds: four batteries (24, 26, 30, 40 Wh), three real rates (0.10, 0.12, 0.15 Wh/m), a road clear or blocked 8 or 20 s, a link good or lost 30 s. Always on the road (72 of 72), and always in a state somebody can name (72 of 72, counting stranded). The three ways of planning:

| planning with | stranded | refused at the start | delivered |
|---|---|---|---|
| the expected rate, checking on the way | **5** | 18 | 30 |
| 1.5 times the expected rate, checking | **0** | **54** | 15 |
| the expected rate, no check | **10** | 18 | 45 |

The five that remain even with the check are cases where the robot used 1.5 times what it expected and the reserve of 5 Wh was spent exactly on getting home. A margin of 1.5 on the rate **removes every stranding** and does it by **refusing 54 of the 72 missions**, delivering half as many: safety bought with work. Neither is wrong, and a number of that kind is for the person who owns the fleet to choose, with the table in front of them. See `E-CAP-0019`.

## Build It

`mission_state_name`, `energy_for`, `can_start` and `must_turn_back` in `exercise/solution.hpp`. The mission machine, the world and the run are given.

```
rcpp verify 21-07
```

## Use It

`rc/capstone/mission.hpp` joins `rc::capstone`.

```cpp
MissionConfig config;
config.rate_margin = 1.5;
const MissionResult r = run_mission(config, world, 3000.0);
if (r.stranded) escalate(r.position_end);
```

## Show It

The table of three planning styles is the one to show: **stranded, refused, delivered** side by side. It makes the point that the safe fleet and the productive fleet are the same code with one number different. Put the 0.12 row next to the 0.15 row, where the check is wrong and then right, and say what you did about the first.

**What this does not do.** One road and one robot. The mission has no route choice, no second robot to hand the load to, no charging station, and no human at the drop to say the parcel arrived: the delivery is a flag. Energy is a flat cost per metre, where a real robot's depends on speed, slope, load and temperature, and a battery's voltage falls as it empties (lesson 19-08's monitor). A blocked road is a flag that comes and goes, not something sensed by a rover (21-02). The timeout of 10 s is a constant, where for a road with people it would depend on what is blocking it. And the **hold** assumes the world waits: a robot held for 570 s on a road is a hazard to the road, which is another reason for a person to be told.

## What Breaks First

- **A battery check made only at the start.** See `E-CAP-0019`.
- **A state with no answer for an event.** See `E-CAP-0020`.
- **A pause that restarts, or counts as waiting.** See `E-CAP-0021`.

## Ship It

`rc/capstone/mission.hpp` holds `MissionState`, `mission_state_name`, `energy_for`, `can_start`, `must_turn_back`, `MissionConfig`, `MissionInputs`, `MissionStep`, `Mission`, `MissionWorld`, `MissionResult` and `run_mission`.
