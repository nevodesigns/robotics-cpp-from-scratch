id: E-PLOT-0006
title: A straight drive that plots nothing
match: a straight drive is a line, not a blank picture
match: expected rows_with_marks to equal
platforms: linux, windows
teaches: 00-14-the-first-tool
---

## Symptom

The robot is told to drive straight and the picture is empty, or filled with one mark at the corner. The end position printed under it is right.

## Cause

A straight path along x has no height. Scaling the picture divides by that height, which is zero, and every position becomes not a number.

`scale_to_fit` guards for it: a path with no extent on one axis is scaled by the other. Measured, a drive at speed 1 and 1 for 2 seconds ended at x=2.000 y=0.000 heading=0.0, and exactly **1** row of the picture had anything on it, with `S` and `E` in it.

## Fix

Use the scale from `scale_to_fit`, which already treats the empty axis, and do not divide by the extent yourself.
