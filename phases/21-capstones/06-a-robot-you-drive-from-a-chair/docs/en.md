# A Robot You Drive From a Chair: A Lossy, Delayed Link, a Stop Path on the Robot, and a Timeout Chosen From a Distance

> A robot that is driven by wire can always be stopped. A robot driven by radio can be stopped only if it is the robot that decides.

**Type:** Capstone
**Time:** about 140 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated link)
**Prerequisites:** 21-05, 19-14, 19-02, 19-04, 17-05, 12-08

## The Problem

The sixth capstone is the robot that a person drives from a chair, with a joystick, a screen and a radio between them. It is what a UGV in the field mostly is. The hazard is G2 from lesson 19-01, a robot that keeps driving when the link is lost, scored 16 and unacceptable, and lessons 19-02 and 19-04 gave the answer: a stop path **on the robot** whose timeout is a **distance**. This lesson puts the numbers on it, with a link that loses and delays packets: what the timeout costs in distance, and what it costs in **stops for nothing**.

## The Concept

### A timeout is a distance

The robot goes on at its speed for the timeout and then brakes: `speed * timeout + speed^2 / (2 * decel)`. `max_timeout` turns that round: for the robot to stop within **1 m**, at 1 m/s and 1.5 m/s^2 of braking, the timeout is at most **0.667 s**; at 2 m/s the braking alone is **1.333 m** and **no timeout** is short enough. The answer to "the robot cannot stop in a metre at 2 m/s" is a lower speed, and not a faster timeout. See `E-CAP-0016`.

### What it does with a link that goes out

The link carries nothing from 10 s. The stop path of lesson 19-02, on the robot's own clock:

| timeout | travel after the link went, 1 m/s | 2 m/s |
|---|---|---|
| 0.1 s | 0.398 m | 1.463 m |
| 0.3 s | **0.598 m** | 1.863 m |
| 1.0 s | 1.298 m | 3.263 m |

One stop each, in the outage, and none outside it. 0.2 s more timeout is 0.2 m more at 1 m/s, as the sum says. The robot took 0.93 s from the loss to standing still at a timeout of 0.3 s. And the link comes back at 20 s: with **no reset** the robot is still at 40 s; with a **person pressing reset after a second of good link** it goes on (2 resets, the first at the start, since the stop path starts stopped). The latch is the point: the link coming back is not permission. See `E-CAP-0016`.

### A link that is slow

A link that delivers every packet but late. The robot obeys old commands for as long as they keep arriving, so the stop comes later by the delay times the speed. With a timeout of 0.3 s at 1 m/s:

| delay | travel after the link went |
|---|---|
| 0 ms | 0.598 m |
| 100 ms | 0.698 m |
| 300 ms | 0.888 m |
| 1000 ms | **1.598 m** |

The delay is **added**: 1 m more at 1 m/s for a second of it. A timeout of 0.3 s is no longer 0.3 s of danger when the link has a second of delay; the distance to plan for is the timeout **plus the delay** times the speed. The latency of a video link, which is what the operator is steering by, adds in the same way, and it does so **in the operator's reactions** as well. See `E-CAP-0017`.

### A link that is lossy

So a short timeout is safer in distance. What does it cost? The operator sends a heartbeat every 50 ms and a packet is lost with probability `loss`. A timeout of `k` heartbeats stops the robot when `k` in a row are lost, which has a probability of `loss^k`. One hour at 1 m/s:

| loss | timeout | false stops | stopped for | chance a run is all lost |
|---|---|---|---|---|
| 0.05 | 0.1 s | **185** | 5.5 % | 0.00250 |
| 0.05 | 0.2 s | 0 | 0.0 % | 0.00001 |
| 0.20 | 0.1 s | **1198** | **52.1 %** | 0.04000 |
| 0.20 | 0.2 s | 101 | 3.0 % | 0.00160 |
| 0.20 | 0.3 s | 3 | 0.1 % | 0.00006 |
| 0.20 | 0.5 s | 0 | 0.0 % | 0.00000 |
| 0.40 | 0.2 s | 840 | 29.3 % | 0.02560 |
| 0.40 | 0.5 s | 6 | 0.2 % | 0.00010 |

With a timeout of two heartbeats on a link that loses a fifth of its packets, the robot was **stopped for more than half the hour** and moved 1672 m of the 3600 it was asked for. Each stop needs a person to press reset, so the real cost is the operator giving up: and a robot whose safety function is switched off by the people who use it is the end of the argument. The timeout is set from **two** things: the distance it can afford (the sum above) and the loss it has to live with (the table), and when they do not meet, the answer is neither: it is a **better link** (an acknowledged, redundant or slower-and-surer one), or a lower speed. See `E-CAP-0017`.

### A timeout from a distance

For a distance of 1 m at 1 m/s the timeout is 0.667 s; the robot went **0.958 m** after the link went, inside the metre (the margin is the 10 ms tick and the speed ramp). At 2 m/s with a timeout of only 0.1 s it went **1.463 m**, outside it, and `max_timeout` for that case is **0**. See `E-CAP-0018`.

## Build It

`max_timeout`, `chance_all_lost` and `should_reset` in `exercise/solution.hpp`. The link, the stop path wiring and the run are given.

```
rcpp verify 21-06
```

## Use It

`rc/capstone/teleop.hpp` joins `rc::capstone`.

```cpp
RemoteConfig config;
config.speed = 1.0;
config.timeout = max_timeout(1.0, config.speed, config.decel);
const RemoteResult r = run_remote(config, 3600.0);
if (r.stops_otherwise > 5) report_link_too_poor(r.packets_lost, r.packets_sent);
```

## Show It

Show **two columns side by side: the distance and the false stops**, for the same timeout. A timeout of 0.2 s looks like a great deal on one and a disaster on the other, depending on the loss. It is a trade-off whose terms you measured, and a safety case that says "the stop path has a timeout of 0.3 s" with neither is not one. And show the outage experiment, since it makes the latch visible: the robot standing still while the link is perfect.

**What this does not do.** The link is independent loss and a fixed delay. Real radio loses packets in **bursts** (a robot behind a wall loses a second at a time), which makes the false stops rarer and the real outages longer than `loss^k` says. The heartbeat is one command stream, where a real station sends video and telemetry too and shares the same radio. The operator is not modelled: a person who sees a stale picture steers into things, a matter of reaction time and display delay that lesson 19-04's latency also contains. The robot is a point with a speed and a deceleration. The reset is a timer, not a person looking at the robot, which is what it must be. And the stop path is **software on the robot**: lesson 19-03 is the argument that the same idea is in the hardware as well.

## What Breaks First

- **A timeout chosen for comfort and not for a distance.** See `E-CAP-0016`.
- **A delay left out of the stopping distance.** See `E-CAP-0017`.
- **A timeout so short that the operator turns the stop off.** See `E-CAP-0017`.
- **A speed too high for any timeout.** See `E-CAP-0018`.

## Ship It

`rc/capstone/teleop.hpp` holds `max_timeout`, `chance_all_lost`, `should_reset`, `LinkNoise`, `RemoteConfig`, `RemoteResult` and `run_remote`.
