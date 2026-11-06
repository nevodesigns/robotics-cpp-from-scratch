# Two Loops, One Inside the Other: Cascade Control and a Speed Limit You Do Not Have to Enforce

> One position loop is asked to move fast, hold against a load and not overshoot, with
> three gains. Two loops share the job, and a speed limit falls out of the arrangement.

**Type:** Build
**Time:** about 150 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 14-04, 14-07

## The Problem

A joint is asked to make a large move and then hold, and a load arrives. With one position
loop, the proportional and derivative gains alone settle quickly and then sag under the load,
by the load over the gain, for ever. An integral term removes the sag and brings a new problem:
during the large move the motor is saturated and the error is big, the integral winds up, and
it has to be unwound on the far side, which is overshoot.

A speed limit is a third problem. The joint has a rated speed, and a position loop does not know
it.

All three come from asking one loop with one set of gains to do three different jobs.

## The Concept

### Two loops

The **outer loop** looks at where the joint is and decides how fast it should be going. It is a gain
times the position error, clamped to a speed limit. Its output is a speed.

The **inner loop** looks at how fast it is going and decides what torque gets it there. It is
proportional and integral on the speed error. Its setpoint is the outer loop's output.

```text
position error  --[ gain, clamped ]-->  speed wanted  --[ PI ]-->  torque  -->  joint
                                              ^                                   |
                                              +----------- measured speed --------+
```

`CascadeController::update(target, position, velocity, dt)` takes the three measurements and
returns the torque. `speed_command()` shows what the outer loop asked for, which is how the limit
is seen to hold.

### What it does to a move and a load

A joint of 0.5 kg m^2 asked to move 2 radians with a motor of 20 N m, held, and at eight seconds a
steady load of 5 N m arrives. The two single loops are tuned for critical damping and the PID has an
integral gain of the same size as the cascade's:

| controller | overshoot | settled | peak speed | worst error after the load | final |
|---|---|---|---|---|---|
| position loop, P and D | none | 0.61 s | 8.04 rad/s | 0.0250 rad | 0.0250 rad |
| position loop, PID | **14.2 %** | **3.18 s** | 8.83 rad/s | 0.0208 rad | 0.0002 rad |
| cascade, speed limit 3 | none | 0.81 s | **3.04 rad/s** | **0.0051 rad** | **0.0000 rad** |

The P and D loop sags by 25 mm and stays there. The PID overshoots by 14 percent and takes 3.2 seconds
to settle, and recovers from the load slowly. The cascade does neither, and costs 0.2 seconds on the
move. See `E-CTRL-0021`.

### The speed limit is not enforced

A single position loop outputs a torque, so a speed limit has to be applied to some other quantity,
a rate limit on the setpoint or a cut on the torque when the measurement is too high. Each is a second
mechanism fighting the first, and the controller knows nothing of it. The two single loops reached
8.04 and 8.83 rad/s: they go as fast as the torque allows.

In a cascade the outer loop's output *is* a speed, so the limit is a clamp on that output before it is
used:

```cpp
double wanted = gains_.position_gain * (target - position);
if (wanted > gains_.speed_limit) wanted = gains_.speed_limit;
if (wanted < -gains_.speed_limit) wanted = -gains_.speed_limit;
```

The inner loop is never asked for more, so the joint peaks at 3.04 rad/s against a limit of 3. It
overshoots its setpoint by a hundredth, which is the price of a tracking loop. A limit that must never be
exceeded still needs the hard stop of lesson 14-02 behind it. See `E-CTRL-0022`.

### The integral still needs a guard

The motor's torque is limited, so while it is saturated a larger raw command changes nothing. The
integral does not know. It keeps accumulating, and the accumulated value has to be unwound by errors of
the opposite sign before the output leaves the limit. That is windup, and the inner loop has it as much as
a single loop does.

Pushing against a joint that does not move, a speed error of 3 rad/s for five seconds with the motor at its
limit: an integral that always accumulates reaches `3 * 5 = 15`. The guarded one reads **0.000**.

The guard is conditional integration: do not integrate when the motor is saturated and the error is still
pushing the same way.

```cpp
const bool saturated = torque != raw;
const bool pushing_further = saturated && ((raw > 0.0) == (speed_error > 0.0));
if (!pushing_further) integral_ += speed_error * dt;
```

When the error pulls back, the integral can move, so it still unwinds. After those five seconds the test
reverses the target and the torque goes straight to the opposite limit. See `E-CTRL-0023`.

## Build It

`CascadeController::update` in `exercise/solution.hpp`. The gains and the accessors are given. The comments
write out the three steps.

```
rcpp verify 14-09
```

Three tests are about the controller itself: the outer loop, the inner loop, and the guard. The fourth is the
table above, run on a joint written into the test, with the two single loops as baselines.

## Use It

`rc/control/cascade.hpp` joins `rc::control`.

```cpp
rc::control::CascadeController joint{rc::control::CascadeGains{}};
// every tick
const double torque = joint.update(target, measured_position, measured_speed, dt);
```

**What this does not do.** The inner loop has to be faster than the outer one, by a factor of about ten
or more, or the two interact and the outer loop sees the inner loop's lag as a delay of its own, which
lesson 14-07 priced. Here the inner loop's bandwidth is of the order of 80 rad/s against the outer
loop's 10. It does not use feedforward, which lessons 14-05 and 13-07 added to a single loop and which
helps a cascade as well. And the speed it uses has to be measured: a speed derived by differencing a
position adds the noise and the delay of a derivative, which lesson 15-01 measured. A tachometer, or an
encoder with a good estimator, is what the inner loop wants.

## What Breaks First

- **One loop for three jobs.** It overshoots or it sags. See `E-CTRL-0021`.
- **A speed limit imposed after the controller.** It reaches 8 where the limit is 3. See `E-CTRL-0022`.
- **An integral that accumulates while the motor is saturated.** It winds up. See `E-CTRL-0023`.

## Ship It

`rc/control/cascade.hpp` holds `CascadeGains` and `CascadeController`.

Lesson 14-08 found a loop that hunts on a joint that sticks. A cascade does not cure friction either, and it
puts the integral where the friction is felt, in the speed loop, which is often the better place to deal with
it.
