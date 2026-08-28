# PID From Scratch

> Three numbers, one line of arithmetic, and most of the automatic control running on earth today.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** none, the plant is simulated
**Prerequisites:** 01-04, 01-02

## The Problem

You want the robot to drive at 1.0 metres per second. You command 1.0 to the
motors. It drives at 0.7, because the floor has friction, the battery is not
full, and the motor is not the ideal object in the datasheet.

So you command 1.4 and it does 1.05. You command 1.35 and it does 1.0, until
someone puts a box on the robot and it does 0.8 again.

Guessing the command is not control. Control is measuring the error and reacting
to it, continuously, without needing to know why the error is there.

## The Concept

Take the error, the difference between what you want and what you measure:

```
error = setpoint - measurement
```

A PID controller responds to that error three ways, and adds the results.

**Proportional** reacts to the error right now. Big error, big push:

```
p = kp * error
```

On its own it never quite arrives. To hold the robot at speed you need some
command, but proportional only produces a command when there is error, so it
settles a little short. That permanent shortfall is called steady state error.

**Integral** reacts to the error accumulated over time. While error persists,
the integral term grows, and it keeps growing until the error is gone. That is
what removes the steady state error:

```
integral = integral + error * dt
i = ki * integral
```

**Derivative** reacts to how fast the measurement is changing, damping the
approach so the robot does not overshoot and oscillate:

```
d = kd * (rate of change)
```

The output is the sum, and there are two details in it that separate a working
controller from a demonstration.

### Detail one: integral windup

The motor saturates at 1.0. If the setpoint demands more than the machine can
deliver, error never reaches zero, and the integral keeps growing. It might reach
50, or 5000. When the setpoint finally drops, the controller keeps commanding
full power until that enormous integral unwinds, which can take many seconds.
The robot sails past its target with the operator watching.

The fix is anti windup: stop accumulating when the output is already saturated
and the error would push it further into saturation. Three lines, and it is the
difference between a controller you can ship and one you cannot.

### Detail two: derivative kick

If derivative is computed from the change in *error*, then changing the setpoint
changes the error instantly, and the derivative term spikes to an enormous value
for one step. The motor jumps.

The fix is to compute derivative from the change in *measurement* instead, and
negate it. The measurement moves smoothly because it is physical. This is called
derivative on measurement and it is what real controllers do.

## Build It

Implement the `Pid` class in `exercise/solution.hpp`:

- `update(double setpoint, double measurement, double dt)` returns the command.
- Output is clamped between `min_output` and `max_output`.
- Anti windup: do not accumulate the integral when the output is saturated and
  the error pushes further into saturation.
- Derivative on measurement, not on error.
- `reset()` clears the accumulated state.
- Guard `dt <= 0`, and return the last output rather than dividing by zero.

```
rcpp verify 14-01
```

The tests include a simulated first order plant, so you can see your controller
actually settling rather than only checking arithmetic.

## Use It

`ros2_control` ships a PID with exactly these features. So does every motor
driver, every drone flight controller, and every thermostat worth having.

What varies between them is the tuning, and tuning is a physical activity, not a
mathematical one: start with everything at zero, raise proportional until it
responds briskly and just begins to oscillate, back it off, add derivative to
damp the overshoot, then add just enough integral to remove the remaining offset.
Doing that on real hardware is lesson 14-04.

## What Breaks First

- **The output rushes past the target and takes seconds to come back.** Integral
  windup, with no anti windup guard. See `E-CTRL-0001`.
- **The motor jumps every time the setpoint changes.** Derivative computed on
  error rather than on measurement. See `E-CTRL-0002`.
- **A settling test fails by a tiny margin.** Compare with a tolerance and give
  the loop enough steps to settle. See `E-NUM-0003`.

## Ship It

`Pid` becomes the centre of `rc::control`. Lesson 14-02 puts a rate limiter and
a watchdog around it, phase 16 uses it to follow a path, and the capstone control
station plots its error live while it runs.
