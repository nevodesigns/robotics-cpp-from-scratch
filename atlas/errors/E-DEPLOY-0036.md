id: E-DEPLOY-0036
title: A bad release given to everyone
match: power lost at each of the ten steps of the install, the robot always boots, and run again each one ends on the new release
match: a release with a fault on three robots in ten is stopped by the first wave, none of the robots it touched is bricked, and the one that failed is back on the old release
platforms: linux, windows
teaches: 20-12-a-release-that-reaches-the-robot
---

## Symptom

A release that works in the lab takes a third of the fleet out of service at once, each robot rolling back by itself.

## Cause

A fault on 300 of 1000 robots took 300 through a trial when given to all; staged, the rollout halted at wave 0 after 10 robots, 9 on the new release, 1 back on 1.4.2 and none bricked. A cut after each of the ten steps of the install left 10 of 10 booting 1.4.2, none bricked, and 10 of 10 ended on 1.5.0 when run again.

## Fix

Roll out in waves and let each robot be able to say no.

```cpp
rollout(fleet, policy);
```
