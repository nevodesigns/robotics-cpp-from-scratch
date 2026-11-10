id: E-MATH-0036
title: A gripper that is certain in one direction and lost in another
match: near a straight arm the doubt at the tool is nearly all along one direction, and with the elbow square it is not
match: expected this to be true: ratio_
platforms: linux, windows
teaches: 06-16-how-sure-is-the-gripper
---

## Symptom

An arm reaching out straight is very precise sideways and poor along its length, and a single "accuracy of the tool" number quoted for the arm hides it. A task that needs the tool to be within a millimetre along the arm fails there and never anywhere else.

## Cause

Near a straight arm the Jacobian loses a direction (the singularity of lesson 13-04): a small change in either joint moves the tool the same way, along one line. The doubt has the same shape.

Measured, joint errors of 0.02 rad each, shoulder at 0.4: with the elbow at 1.57 rad the spread of the tool was **13.97** mm along its long axis and **5.73** across, a ratio of 2.4. With the elbow at 0.5 rad it was 19.09 and 2.01 (9.5), and at **0.05** rad it was 19.69 and **0.20**, a ratio of **97**.

## Fix

Report the doubt as an ellipse, with its axes, and check the task against the long axis at the poses where it will be done:

```cpp
const rc::math::Axes a = rc::math::principal_axes(propagate(jacobian, joint));
```
