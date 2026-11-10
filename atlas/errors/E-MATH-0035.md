id: E-MATH-0035
title: A linear estimate of the doubt trusted when the errors are large
match: for small joint errors the propagated doubt matches twenty thousand random trials, and for large ones it does not
match: expected this to be true: large_error
platforms: linux, windows
teaches: 06-16-how-sure-is-the-gripper
---

## Symptom

The doubt at the tool, worked out from the joint errors, is confidently quoted for a robot whose joints are poorly known, and the tool lands outside the quoted ellipse far more often than it should.

## Cause

`J C J'` assumes the tool moves in proportion to the joint error, which is true only for small errors. The arm's position is a sine and a cosine of the angles, and over a large error the curve shows.

Measured, at a shoulder of 0.4 and an elbow of 1.0 rad, against 20000 random trials: for joint spreads of **0.01** rad the variance of x was 5.02e-05 by formula and 5.03e-05 sampled. For **0.3** rad it was 4.52e-02 against 4.00e-02, and the covariance -3.04e-02 against -2.56e-02: out by more than a tenth.

## Fix

Check the approximation against random trials for the largest errors that matter, which costs one run in a test, and use the trials themselves (`sampled_tool_covariance`) where the errors are large.
