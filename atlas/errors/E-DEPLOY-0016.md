id: E-DEPLOY-0016
title: A trial counter that does not count crashes
match: a release on trial costs a try at each boot, the previous one comes back when they are gone, and confirming ends the trial
platforms: linux, windows
teaches: 20-06-on-trial-until-it-proves-itself
---

## Symptom

A broken release crashes and the robot reboots into it again and again, and never goes back.

## Cause

Three tries gave three boots of the new release and the fourth was the old one. The try is used before the release runs. After confirming, ten more boots stayed on the new release. Zero tries rolled back at the first boot.

## Fix

Keep the counter in the boot loader and use the try up first.

```cpp
--s.tries_left;
return s.active;
```
