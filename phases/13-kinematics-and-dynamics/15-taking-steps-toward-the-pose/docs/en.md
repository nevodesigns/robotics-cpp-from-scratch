# Taking Steps Toward the Pose: Numerical Inverse Kinematics, the Damping That Keeps It Going Near a Singularity, and Knowing When It Has Failed

> Where there is no formula, take steps. Damp them near a singularity, limit their size, apply the joint limits at every one, and say when you have not arrived.

**Type:** Build
**Time:** about 150 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 13-11, 13-13, 13-04

## The Problem

Most arms have no formula for their angles. A program asks where the tool is, looks at how far that is from where it should be, asks how the tool moves when each angle moves, and changes the angles to close the gap. It works until the arm is near a singularity, or the pose cannot be reached, or the answer is outside a joint limit, and each of those fails in a different way.

## The Concept

### The Jacobian

`jacobian3` is how x, y and the heading change with each of the three angles. The heading column is 1, 1, 1: every angle turns the tool one for one. Against central differences of the forward kinematics the largest difference was **1.5e-10**.

### Damped steps arrive

Four poses and thirty starts each, spread round the arm: **120** of 120 converged, **0** of those were off the pose by more than 1e-5, **17** iterations on average and **35** the worst. The step is `Jt (J Jt + lambda squared I) inverse e`, scaled down if it is longer than the limit.

### Near a singularity

An arm 1e-5 rad from straight, asked to move its tool 0.1 along itself, the direction a straight arm cannot move in: the plain step (no damping) was **24496.8** rad, for a tenth of a metre. The damped step was **0.000163301** rad, small and well inside the limit. The damping gives up some accuracy near the singularity to keep the step sensible: the price is that progress in that one direction is very slow, which is the arm telling you what it can do. Away from the singularity, a damping of 0.01 changed the step by less than five percent. See `E-KIN-0038`.

### A pose that cannot be reached

Links 1, 1, 1 and the tool at (2.9, 0) pointing back along x: the wrist would have to be at 3.9. The solver did not converge, stopped at its limit of **300** iterations (and did not run on), the angles were finite, and it left the tool **1.7661** m and **0.604296** rad from the target: a compromise between the two errors. A target at (5, 0), beyond the whole arm, left the position error between 1.9 and 2.1 with the arm stretched out along x towards it. A solver that cannot say that it did not arrive is one whose answer cannot be used. See `E-KIN-0039`.

### Joint limits

Limiting the elbow to bend one way, then the other: the same pose was reached both ways from **5** of 5 starts, with elbow angles of **2.55128** and **-2.55128**: the mirror solutions. Limits that leave no solution, an elbow that may move 0.05 rad, held at every step and the solver did not converge. The limit is applied after each step and not once at the end. See `E-KIN-0040`.

## Build It

`jacobian3`, `dls_step` and `solve_numeric` in `exercise/solution.hpp`.

```
rcpp verify 13-15
```

## Use It

`rc/kin/numeric_ik.hpp` joins its module.

```cpp
rc::kin::NumericOptions options;
options.use_limits = true;
options.lo[1] = 0.1;  options.hi[1] = 2.9;          // the elbow bends one way
const double start[3] = {q1, q2, q3};
const rc::kin::NumericResult r = rc::kin::solve_numeric({1.5, 0.5, 0.3}, start, 1.0, 1.0, 1.0, options);
if (!r.converged) { /* the pose was not reached: say how near it got, r.position_error and r.heading_error */ }
```

**What this does not do.** It is for the three link arm in the plane, and the idea carries to six joints in space with a six by six matrix and the pose error of lesson 13-13, which is how it is used in practice. It finds one solution: the one it falls into from its start, so the start is a choice, and it finds a different solution from a different start. A fixed damping is a compromise, and a better method adapts it to how near the singularity is. A step limit that is too small makes it slow and one that is too large makes it wander. Clamping to limits can leave the solver stuck at a limit in a place that is not a solution, and it will stop at the iteration limit and say so. And it is slower than a formula where there is a formula, so it is the choice when there is none.

## What Breaks First

- **The plain step near a singularity.** See `E-KIN-0038`.
- **A solver that does not say it failed.** See `E-KIN-0039`.
- **Joint limits applied at the end, or not at all.** See `E-KIN-0040`.

## Ship It

`rc/kin/numeric_ik.hpp` holds `Jac3`, `jacobian3`, `JointStep`, `dls_step`, `NumericOptions`, `NumericResult` and `solve_numeric`.

The capstone asks the arm for a pose and finds out which of the failures it is.
