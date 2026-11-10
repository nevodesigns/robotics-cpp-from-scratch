id: E-MATH-0034
title: Joint errors treated as if the tool had the same error
match: the doubt at the tool is J C J transposed, worked out for a pose
match: expected tool.xx
platforms: linux, windows
teaches: 06-16-how-sure-is-the-gripper
---

## Symptom

An arm's joints are known to 0.02 rad and the tool is assumed to be known to "about the same". In use the tool is off by a centimetre in one direction and a millimetre in another, and the error depends on the pose.

## Cause

A joint angle is an angle and the tool position is a length. How an error in one becomes an error in the other depends on the arm's shape, through the Jacobian: the tool moves by `J` times the joint change, so its covariance is `J C J'`. The same joint errors give different tool errors at different poses, and not in equal amounts in x and y.

Worked out for links of 1 and a shoulder at 0 with the elbow at 90 degrees: `J = [[-1, -1], [1, 0]]`. With joint spreads of 0.2 and 0.3 rad the tool's variances were **0.13** in x and **0.04** in y, with a covariance of **-0.04**. Errors that move together in the joints changed the answer: a joint covariance of 0.02 made the variance in x 0.17.

## Fix

```cpp
out.xx = j.a * (j.a * joint.xx + j.b * joint.xy) + j.b * (j.a * joint.xy + j.b * joint.yy);
```
