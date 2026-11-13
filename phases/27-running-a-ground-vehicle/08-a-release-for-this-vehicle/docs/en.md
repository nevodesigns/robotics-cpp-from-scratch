# A Release for This Vehicle: Only When It Is Parked and Charged, Never With Another Vehicle's Calibration, and Kept Only If It Drives Properly

> An update that cannot be refused, tested on the vehicle and taken back is a gamble with someone else's robot.

**Type:** Build
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 27-07, 20-12, 23-08

## The Problem

Phase 20 got a release **onto a robot**: staged, checked, installed in a second slot, tried, confirmed or rolled back, and rolled out in waves. That pipeline does not know what **this** robot is doing. A ground vehicle is a particular machine in a particular place with a **calibration of its own** (its track width, its slip, the offsets of its sensors), a **map** of its site, and a job in the middle of which it may be. An update that is right for the fleet can be wrong for **this** vehicle, now.

## The Concept

### Refuse, and say all the reasons

`check_update` compares the vehicle with the bundle and lists **every** reason it is refused, so a person sees the whole of it and not the first of it:

| reason | why |
|---|---|
| the wrong **class** of vehicle | software for a car is not software for a skid steer |
| a **calibration for another vehicle** | the track width and the slip of one vehicle are not another's |
| a **schema** or a **map older** than the one on the vehicle | the vehicle would lose what it has learned |
| the vehicle **not parked** | an update is a reboot, and a reboot of a vehicle that is moving is a stop (and a loss of the mission's place, 27-01) |
| a **mission** under way | the same |
| **charge** under 50 percent | a trial that flattens the battery halfway leaves a vehicle neither old nor new |
| a **version** that the rules of 20-12 do not accept | older, the same, below the floor, or not a version |

A vehicle that was moving, in a mission and at 20 percent charge was refused for **3** reasons at once. A charge that is not a number counts as low. A bundle with no calibration in it is fine: the vehicle keeps its own. See `E-VEH-0133`.

### Its own calibration

A bundle that carries the calibration of **another vehicle** is refused. The effect of letting it in is not subtle: a track width of 0.40 m taken for 0.25 m (lesson 23-02) makes a skid steer turn **1.60 times** too little for what it is asked, and every path of phase 26 is followed wide. See `E-VEH-0134`.

### The new software has to drive

The trial of 20-06 keeps a release only if it says it is healthy, and a release that says so without being asked to prove it is a release that says so. After the update, a **self test on the vehicle** runs a short drive on the new software and measures four things: a command to go forward **moves** it forward; the wheels' distance and the ground's over 1 m **agree** to 5 percent; an **emergency stop** stops it within its distance; the scanner **sees** a target. A wheel wired backwards failed **1** check; a calibration that was lost (the wheels over the ground 0.8) failed **1**. Only a pass **confirms**; a number that is not a number fails. See `E-VEH-0135`.

### Rolled back

A release whose self test passes is confirmed at its first boot (**1** boot). One whose self test fails is **never confirmed**, and the boot loader of 20-06 rolls it back when its tries (3) are used up, after **4** boots, to the old slot.

## Build It

`check_update` in `exercise/solution.hpp`.

```
rcpp verify 27-08
```

## Use It

`rc/drive/vehicle_release.hpp`.

```cpp
const auto why = rc::drive::check_update(on_board, bundle, policy);
if (!why.empty()) refuse(why);
// install, boot the new slot, then:
if (rc::drive::post_update_selftest(measured, limits).empty()) rc::deploy::confirm(boot);
```

**What this does not do.** The self test is four numbers handed to it: **driving the vehicle** to get them is the work of whoever runs it, and a vehicle that cannot be driven (a dead motor) fails by not producing them. The calibration is checked by the vehicle's id, which is a string somebody has to have set right. No migration of a configuration from one schema to the next is here: a newer schema than the vehicle's is accepted without a word, and its migration is the release's own business. The waves of the fleet rollout (20-07) and the signing of the bundle (20-12) are not repeated: this is the part that is about the vehicle. The checks of the state are the ones of the vehicle at the time they are made, not at the time the update lands.

## What Breaks First

- **An update that lands mid-mission.** See `E-VEH-0133`.
- **A calibration that belongs to another vehicle.** See `E-VEH-0134`.
- **A release that says it is fine.** See `E-VEH-0135`.

## Ship It

`rc/drive/vehicle_release.hpp` holds `OnBoard`, `Bundle`, `Refusal`, `UpdatePolicy`, `check_update`, `SelfTestInput`, `SelfTestLimits`, `post_update_selftest`, `Settled` and `boots_until_settled`.
