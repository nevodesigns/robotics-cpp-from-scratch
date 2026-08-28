# Limits and the Watchdog: Failing Into a Safe State

> The question is not whether your control loop will stop running. It is what the motors are doing one second after it does.

**Type:** Build
**Time:** about 105 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** none, though this is the lesson that matters most once there is hardware
**Prerequisites:** 14-01

## The Problem

Your controller from the last lesson works. It holds speed, it rejects
disturbance, it settles cleanly.

Now the program that computes it stops. The process is killed, the network cable
carrying the command is unplugged, a thread deadlocks, the operator's laptop
goes to sleep, or the code simply takes a wrong branch and stops calling update.

The last command the motor received was 0.9. Nothing tells the motor otherwise.
It is still driving at 0.9 when it reaches the wall, and it will keep doing so
until something physical stops it.

This is not a hypothetical failure. It is the ordinary one, and the reason
robots need engineering rather than programming.

## The Concept

### A command is only valid for a short time

The fix is to treat every command as perishable. A command carries an implicit
expiry, and the actuator layer, not the controller, enforces it.

A **watchdog** is the object that does this. It has one job:

- It is **fed** every time a fresh command arrives.
- If it has not been fed within its timeout, it has **expired**.
- Once expired, the output is forced to a safe value regardless of what the last
  command was.

Three lines of state, and it converts "the robot keeps going" into "the robot
stops" for an entire class of failure.

### The safe value is a decision, not a default

Zero is the safe output for a wheel motor. It is not the safe output for
everything:

- A robot arm holding a load has a safe state of holding position, not going
  limp, because limp means dropping whatever it holds.
- A heater's safe state is off.
- A valve may be safe open or safe closed, depending on what it carries.

So the safe value is a parameter that the person who understands the machine
must set on purpose. A library that silently defaults it to zero is making a
safety decision on behalf of somebody who never knew a decision was being made.

### Watchdogs are layered

The watchdog you are about to write is a software one, inside the same program
as the controller. It catches a stalled control loop, and it cannot catch its own
process being killed.

That is why real machines layer them. The motor controller has its own hardware
watchdog that stops the drive if the serial link goes quiet. Beyond that sits an
emergency stop that cuts power through physical contacts, wired so that a broken
wire stops the machine rather than disabling the stop. Software cannot fail safe
on its own, and phase 19 is about the parts that can.

What you write here is the innermost layer of a set, and it is still worth
having, because it catches the most common failure with the least machinery.

## Build It

Implement two things in `exercise/solution.hpp`.

`RateLimiter` holds the current output and limits how fast it can change:

- `apply(double target, double dt)` moves towards the target by at most
  `max_rate * dt`, and returns the new value.
- `reset(double value)` sets the current value without limiting.

`Watchdog` enforces the expiry:

- `feed(double now)` records that a command arrived at time `now`.
- `expired(double now)` reports whether the timeout has passed.
- `guard(double command, double now)` returns the command when fresh, and the
  safe value when expired.
- Before the first feed, it must count as expired. A watchdog that starts out
  trusting is worse than none, because it hides the case where commands never
  arrived at all.

```
rcpp verify 14-02
```

## Use It

`ros2_control` will halt a controller whose commands go stale. CAN based motor
drives implement a heartbeat that stops the drive when frames stop arriving.
Industrial safety controllers do it in hardware, certified to a standard, because
software is not permitted to be the last line.

You now know what all of them are doing, which means you can check whether a
given drive actually does it, and how long its timeout is. That question is worth
asking of any hardware you are handed.

## What Breaks First

- **The watchdog trusts the world before the first command.** A freshly
  constructed watchdog must report expired. See `E-CTRL-0003`.
- **The timeout is compared with the wrong sign, so it never expires.** See
  `E-CTRL-0004`.
- **The rate limiter oscillates around the target instead of settling.** Take the
  whole remaining change when it is smaller than one step. See `E-NUM-0005`.

## Ship It

`RateLimiter` and `Watchdog` join `rc::control`, and from this point on every
capstone in this curriculum is required to route its actuator commands through
both. That requirement is enforced by the capstone's own tests, not by a note in
a document.
