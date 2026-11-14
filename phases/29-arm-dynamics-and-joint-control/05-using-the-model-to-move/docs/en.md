# Using the Model to Move: Computed Torque, an Error That Behaves Like a Second Order System, and How Wrong the Model Can Be Before It Is Worse Than None

> If the model is right the arm is a set of independent unit masses; if it is wrong the controller is a PD controller with extra steps.

**Type:** Build
**Time:** about 110 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 29-04, 29-02, 29-01

## The Problem

A joint controller (29-03) treats each joint alone and leaves the coupling between them as a disturbance. For a slow move that is fine. For a fast one it is not: the torque one joint needs depends on what all the others are doing (the mass matrix is full, and the velocity products are a rope of cross terms), and a loop that does not know is always behind. Gravity (29-02) was the first step: add the part of the torque that the model can tell you. Computed torque takes the idea all the way.

## The Concept

### Ask the model for the torque

The arm obeys `M(q) qdd + C(q, qd) qd + g(q) = tau`. Suppose you want the joints to have an acceleration `a`. Then choose `tau = M(q) a + C(q, qd) qd + g(q)`: the model's own inverse. If the model is exact, `qdd = a`: the arm becomes six independent unit masses, whatever its pose and speed. Now choose `a` to be what a unit mass needs to follow a reference and correct its own error:

```
a = qdd_ref + Kv (qd_ref - qd) + Kp (q_ref - q)
```

so that the error obeys `e'' + Kv e' + Kp e = 0` in every joint, separately: a second order system with the gains you choose. With `Kp = wn squared` and `Kv = 2 wn` it is critically damped at a bandwidth `wn`. In code it is one call: the inverse dynamics of 29-01, for the state and the wanted acceleration. See `E-ARM-0040`.

### A reference to follow

A move that starts and ends at rest, with no jump in acceleration, is a quintic in time: `s = 10 u cubed - 15 u to the fourth + 6 u to the fifth`, with `u = t / T`. Its derivatives are the velocity and acceleration that the controller needs as feedforward. For a move of about 0.7 rad in each joint in 1 s, the joint PD with gravity (29-02) had an rms error of 0.04293 rad; computed torque with an exact model, 0.000056948 rad: the model removes the error, and what is left is the integration. See `E-ARM-0041`.

### The error behaves

Started 0.1 rad out of place and at rest, computed torque at a bandwidth of 20 rad/s crossed zero 0 times (a critically damped system does not overshoot) and was within a hundredth of the start in 0.33 s. The behaviour is designed, not hoped for: that is what the model is for.

### The price: the model has to be right

An exact model is not on offer. Every mass of the typical arm made too big by 10, 20 and 40 percent gave an rms error of 0.00467, 0.00853 and 0.01455 rad: it grows with the model error, and gently for the first 10 percent. A model that is too light by 40 percent gave 0.03469, which is worse than the heavy one of the same size: a model that underestimates the arm gives torques that fall short and the feedback has to make it up. The largest torque asked by the two controllers was the same, 28.5 N m (joint PD) and 28.5 N m (computed torque): on this move it is the weight of the arm held out that sets it, not the acceleration. A faster move would ask for more, and the motors have to have it (29-08). See `E-ARM-0042`.

## Build It

`computed_torque` in `exercise/solution.hpp`.

Open `exercise/solution.hpp`. The part you write is marked `TODO`; the rest of the file is already written, and it is worth reading before you start, because your part is called by it or calls it.

What to write, as the note beside the `TODO` puts it:

- The torque that gives `want` as the acceleration in this state is the inverse dynamics of the model for (q, qd, want), gravity included.

As shipped, 3 of the 4 tests fail, and these are the ones your code has to turn green:

- With an exact model computed torque follows a fast move far more closely than a PD controller with gravity.
- From a start out of place the error of computed torque dies away as a critically damped second order system.
- A model that is wrong gives an error in proportion, and one that is too light is worse than one that is too heavy by as much.

The other one passes already, because it tests the parts that are given.

```
rcpp verify 29-05
```

A failing check names the test and prints the number it expected beside the one it got, so start from the first failure and work down. When the suite is green, read `reference/solution.hpp` beside your own version: where the two differ and both pass, the difference is worth understanding.

## Use It

`rc/arm/computed_torque.hpp`.

```cpp
const auto tau = rc::arm::computed_torque(model, q, qd, q_ref, qd_ref, qdd_ref, wn * wn, 2.0 * wn);
```

**What this does not do.** The model is the typical rods of 29-01 and the comparison is for that arm. Friction and play are not in the plant (29-04 made them); computed torque does nothing about either, and a loop wants an integral or a friction feedforward on top. The mass matrix is worked out every cycle, which is seven inverse dynamics calls for six joints (one for the bias and six for the columns): a real controller has a faster way, or a slower rate for the model and a fast loop under it. There is no torque limit: the largest torque of the fast move may be more than a motor gives (29-08). The reference must be smooth to the second derivative: a trajectory with a jump in acceleration asks for a jump in torque. The measurement of the state is perfect, with no delay and no noise.

## What Breaks First

- **Treating the joints as independent when they are not.** See `E-ARM-0040`.
- **A feedforward that is left out.** See `E-ARM-0041`.
- **A model that is wrong, trusted.** See `E-ARM-0042`.

## Ship It

`rc/arm/computed_torque.hpp` holds `Quintic`, `quintic_at`, `computed_torque`, `scaled_model`, `TrackResult` and `track`.
