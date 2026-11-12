id: E-DEPLOY-0017
title: A release that hangs and is never rolled back
match: a good release confirms itself, and a release that crashes at once is tried three times and then given up on
match: confirming when the program starts is the same as not checking, and a crash loop is the result
platforms: linux, windows
teaches: 20-06-on-trial-until-it-proves-itself
---

## Symptom

The robot is up and does nothing, forever, on a release that never became healthy.

## Cause

A release healthy at 20 s was confirmed in 1 boot; one that crashed at 2 s and one that never became healthy were each rolled back in 4 boots; with five tries, 6 boots. Confirming at the start of the program left a release that crashed at 40 s in a crash loop of all 20 boots, and a confirmation after healthy plus a soak rolled it back in 4.

## Fix

Reset at a deadline if not healthy, and confirm only after the checks have held.

```cpp
const double at = b.healthy_at + soak;
```
