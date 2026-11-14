# One Joint Too Many: The Null Space of a Seven Joint Arm, Moving the Elbow While the Hand Stays, and Steering Away From the Limits

> A seventh joint is not a spare, it is a free choice, and the Jacobian says exactly how much of it there is.

**Type:** Build
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 28-06, 28-05

## The Problem

A six joint arm has exactly enough joints to put its tool anywhere in space with any orientation: six numbers for six degrees of freedom, and for a given pose there are a few arm configurations (28-04: eight) and no freedom between them. A seven joint arm, like the Panda, has one more than it needs. For a given pose of the tool it has not eight configurations but a whole family: it can swivel its elbow round the line from the shoulder to the wrist while the hand does not move. That freedom can be used: to keep the joints away from their limits, to keep away from a singularity, or to get an elbow out of the way of something.

## The Concept

### The part the tool does not feel

The Jacobian of a seven joint arm is six rows by seven columns. There are joint motions that it maps to zero tool velocity: the null space. Given any wanted joint motion `m`, the part that the tool does feel is the damped solution for the tool velocity it would cause (the step of lesson 28-05); what is left over is the null space motion:

```
null part = m - J^T (J J^T + lambda^2 I)^-1 J m
```

For random poses of the Panda the null part of a joint motion was felt by the tool at 0.0000000001 of its size: not at all. For the six joint UR5, 0.0000002311: there is nothing left over, because six joints with a full-rank Jacobian leave nothing free. A motion applied without the projection was felt as 1.244 m/s; through it, 0.00000000. Adding a "preferred" joint motion straight onto the step that tracks the tool is the standard mistake: it moves the tool. See `E-ARM-0019`.

### What to do with the freedom

Here: move the joints towards the middle of their limits. The squared distance of the joints from the middle of their limits, each as a share of its range, `H = sum ((q - middle) / range)^2`, has a gradient, and minus the gradient is the direction that improves it; it is projected through the null space so that the tool does not notice. The margin is the least distance of any joint to a limit, as a share of its range. Started with joint 3 at 2.4 rad (0.086 of its range from its limit), the self motion over 6 s took the margin to 0.156, moved the joints 0.66 rad and left the tool within 0.00037 m of where it was. With no null space term the margin stayed at 0.086: the arm is where it was put. (The first version of this lesson started with joint 6 near its limit and the margin did not move: the one free motion of this arm, the swing of the elbow, hardly turns joint 6. The null space can only help the joints that take part in it, and one dimension of freedom does not reach every joint.) See `E-ARM-0020`.

### A first order promise

The null space is the null space of the Jacobian at this pose, and it holds only to first order: take a finite step through it and the tool moves a little, because the arm is not linear. Over 6 s of steps the tool, with its error corrected every step, never left a ball of 0.00037 m; with no correction it drifted 0.00076 m. See `E-ARM-0021`.

### Limits: clamp, or go round

With six joints, a target that needs a joint outside its limit is unreachable: that is the end. With seven, another configuration may reach it. A target that needed joint 6 at -0.3 rad (its limit is -0.0175) was reached by the unclamped iteration with joint 6 0.283 rad outside its limit, which the arm cannot do; the clamped iteration kept every joint inside, ending with joint 6 at 0.013 rad. Whether it also reached the pose is for the result's `ok` to say; the point is that the unclamped answer is wrong however accurate it is.

## Build It

`null_space_motion` in `exercise/solution.hpp`.

Open `exercise/solution.hpp`. The part you write is marked `TODO`; the rest of the file is already written, and it is worth reading before you start, because your part is called by it or calls it.

What to write, as the note beside the `TODO` puts it:

- V is the tool velocity that the motion m would cause; take from m the damped joint solution for that velocity (dls_step with lambda): what is left is felt by nothing.

As shipped, 3 of the 5 tests fail, and these are the ones your code has to turn green:

- A joint motion put through the null space is not felt by the tool, for seven joints, and there is none for six.
- A motion applied without the projection is felt by the tool, and through it is not.
- The elbow moves towards the middle of the limits while the hand stays, and nothing moves with no null space term.

The other 2 pass already, because they test the parts that are given.

```
rcpp verify 28-07
```

A failing check names the test and prints the number it expected beside the one it got, so start from the first failure and work down. When the suite is green, read `reference/solution.hpp` beside your own version: where the two differ and both pass, the difference is worth understanding.

## Use It

`rc/arm/redundant.hpp`.

```cpp
const auto g = rc::arm::limit_gradient(table, q);
std::vector<double> want(g.size());
for (std::size_t i = 0; i < g.size(); ++i) want[i] = -k * g[i];
const auto self = rc::arm::null_space_motion(j, want, 1e-3);   // add this to the tool's step
```

**What this does not do.** The only thing it uses the freedom for is the middle of the limits, and the middle of a joint's range is not a good pose for the arm: it is a pose far from the limits, which is different. It does not avoid obstacles or singularities with the extra joint (the same projection would; the gradient would be a different one). The projection is a first order one and wants a correction on the tool; the correction here is a plain proportional one. The Panda's own limits are the maker's published ones; velocity and torque limits are not in the margin. A joint that is redundant is not an arm that is redundant at every pose: at some poses (a singularity) the seventh joint is no longer free.

## What Breaks First

- **A preferred motion added without the projection.** See `E-ARM-0019`.
- **An arm that sits against a limit with a spare joint.** See `E-ARM-0020`.
- **A null space motion with no correction.** See `E-ARM-0021`.

## Ship It

`rc/arm/redundant.hpp` holds `joint_margin`, `limit_gradient`, `null_space_motion`, `SelfMotion` and `self_motion_run`.
