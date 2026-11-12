id: E-SAFE-0038
title: A design with a second channel and a single point of failure
match: a fault tree gives the chance of the top event and names the single points that would cause it alone
match: the average chance that a tested function is dead is about half of rate times interval, and the approximation is only good while that is small
platforms: linux, windows
teaches: 19-13-how-likely-after-the-fix
---

## Symptom

The design review counts two channels, and the top event is dominated by one part that has no partner.

## Cause

Contactor welded 1e-4, or (software stop 1e-3 and brake 1e-2) gave 1.100e-04 per demand with one single point, the contactor. A second contactor in series gave 1.001e-05 and none. At 1e-5 per hour the average pfd was 3.641e-03 for a monthly test and 4.258e-02 for a yearly one; rate T over 2 gave 4.383e-02, and 4.383e-01 against 3.340e-01 at ten years.

## Fix

Name the single points in the tree and put the effort there.

```cpp
single_points(tree, names);
```
