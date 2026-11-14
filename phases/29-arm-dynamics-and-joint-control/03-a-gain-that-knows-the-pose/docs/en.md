# A Gain That Knows the Pose: The Joint Control Cascade of an Arm, an Inertia That Changes Threefold With the Pose, and Gains That Follow It

> A loop tuned for the arm folded up rings when the arm is stretched out, or the other way, and the arm moves between them all day.

**Type:** Build
**Time:** about 110 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 29-02, 14-04, 22-04

## The Problem

Each joint of an arm has a motor and a controller. The controller is a cascade: the position loop asks for a velocity, the velocity loop for a torque (a current), and the current loop, the fastest, makes the motor give it. Its gains are chosen for the inertia the joint has to move. On a mobile base that is one number. On an arm it is not: the inertia the shoulder sees depends on the pose, and over the poses of a typical arm it changes by a factor of several. A controller tuned for one pose is wrong for the others.

## The Concept

### The inertia the joint sees

The diagonal term of the mass matrix of 29-01, `M(q)[i][i]`, is the inertia joint `i` sees if nothing else moves. A motor adds its own rotor, through the gear ratio squared: a rotor of 1e-4 kg m2 behind a gear of 100:1 is 1.0 kg m2 at the joint. Over a spread of poses, the typical arm's shoulder saw 0.691 to 2.091 kg m2, a ratio of 3.0. Through a 100:1 gear (a rotor of 1e-4 kg m2, which is 1.0 kg m2 at the joint) the ratio fell to 1.8: a geared joint sees less of the arm's changes, because most of what it sees is its own rotor. (That is also why a direct drive arm needs the pose in its gains and a heavily geared one can often do without.) See `E-ARM-0034`.

### What the velocity loop does with it

The velocity loop makes a torque in proportion to the speed error: `tau = Kv times error`. The joint is `J times acceleration = tau`, so the loop's bandwidth (the speed at which it corrects) is `Kv / J`. A gain `Kv` chosen for a bandwidth of 80 rad/s at the heaviest pose is three times that at the lightest, and with the delays of a real drive (here a delay of three samples in the loop and a current loop that is a 1.5 ms lag) the loop at the light pose is far past what the delay allows: it rings.

With the gains for the heaviest pose, a step of 0.5 rad at the heaviest pose overshot 0.0 percent; the same gains at the lightest pose gave 13909338 percent: the loop is unstable, and that number only says how far it had swung when the run ended. With gains that follow the inertia, `Kv = J times wv` (and the integral gain putting its zero at a quarter of the bandwidth), the overshoot was 0.0 percent at the lightest and 0.0 percent at the heaviest: the same response in both. See `E-ARM-0035`.

### The torque limit, and the integral that does not know

A real motor has a limit. A step of 1.5 rad into a limit of 20 N m sits at the limit for a long time, and an integral term keeps adding up the error all that time: it is wound up, and has to unwind by overshooting. With the integral running on, the step overshot 186 percent; with it held while the torque is at its limit (a clamp), 19.0. See `E-ARM-0036`.

## Build It

`scheduled_gains` in `exercise/solution.hpp`.

Open `exercise/solution.hpp`. The part you write is marked `TODO`; the rest of the file is already written, and it is worth reading before you start, because your part is called by it or calls it.

What to write, as the note beside the `TODO` puts it:

- The velocity loop's proportional gain is the inertia times its bandwidth; its integral gain is that times a quarter of the bandwidth.

As shipped, 3 of the 4 tests fail, and these are the ones your code has to turn green:

- Gains tuned for the heaviest pose are good there and ring at the lightest; gains that follow the inertia are good at both.
- A step into the limit overshoots far more if the integral keeps running.
- The gains scale with the inertia.

The other one passes already, because it tests the parts that are given.

```
rcpp verify 29-03
```

A failing check names the test and prints the number it expected beside the one it got, so start from the first failure and work down. When the suite is green, read `reference/solution.hpp` beside your own version: where the two differ and both pass, the difference is worth understanding.

## Use It

`rc/arm/joint_cascade.hpp`.

```cpp
const double j = rc::arm::effective_inertia(model, q, joint, rotor_inertia, gear_ratio);
const rc::arm::CascadeGains g = rc::arm::scheduled_gains(j, 15.0, 80.0);
```

**What this does not do.** The joint is simulated alone: the other joints are held still, and the coupling between joints (the off-diagonal terms of the mass matrix, the velocity products) is a disturbance the loop does not know about; computed torque (29-05) is the answer to it. The gravity torque is not included (29-02 is its answer, and here it would be an offset). The delay and the current lag are numbers chosen for the example, not measured. Viscous friction is a single number; Coulomb friction and play are 29-04. The gain schedule is instant: the inertia is worked out from the measured pose each cycle, and for a fast move it changes while the loop is running. There is no stability margin measured, only a step response; a loop can have a good step and a poor margin.

## What Breaks First

- **Gains for one pose used for all.** See `E-ARM-0034`.
- **A velocity loop that rings at the light end.** See `E-ARM-0035`.
- **An integral that winds up at the torque limit.** See `E-ARM-0036`.

## Ship It

`rc/arm/joint_cascade.hpp` holds `CascadeGains`, `effective_inertia`, `scheduled_gains`, `StepResult`, `LoopConfig` and `step_response`.
