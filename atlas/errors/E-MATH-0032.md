id: E-MATH-0032
title: A radius asked of an arc too short to say
match: the shorter the arc the less the radius is known, whatever the method
match: expected this to be true: short_arc
platforms: linux, windows
teaches: 06-15-the-circle-the-points-lie-on
---

## Symptom

A calibration from a gentle curve gives a radius that changes by metres from one run to the next. Refining the fit, or adding more points, does not help much.

## Cause

On a short arc almost any large circle passes through the points about as well as the true one: the curvature is nearly unseen. It is a property of the data and not of the method. The noise in the radius grows as the arc shortens, quickly.

Measured with the refined fit, the rms error of the radius over 300 runs: **0.006** for a 180 degree arc, **0.172** for 40 degrees and **5.986** for 15. More points narrow the scatter a little and cannot recover the information the arc does not contain.

## Fix

Drive a longer arc, or fix the radius from another measurement and fit only the centre. A calibration should report how long an arc it needs, and refuse a shorter one.
