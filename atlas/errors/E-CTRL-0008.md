id: E-CTRL-0008
title: Integral gain added where there was no offset to remove
match: without a load, adding integral gain only makes things worse
match: with a load, integral gain is what arrives
platforms: linux, windows
teaches: 14-04-choosing-the-gains
---

## Symptom

A loop that was behaving is given an integral term because a tutorial said the
integral term removes steady state error, and it gets worse: overshoot appears
where there was none, and it takes several times longer to settle.

Reducing the gain helps a little. Removing it entirely fixes it.

## Cause

The integral term does one job: it produces output that persists after the error
has gone. That is exactly what a **steady load** needs, and it is exactly what a
loop without one does not.

Measured, on a mass with damping, against a target of one:

| gains | overshoot | settles | final error |
|---|---|---|---|
| PD 20/8, no load | 0.0% | 1.21 s | 0.0000 |
| PID 20/6/8, no load | 10.3% | 6.58 s | -0.0122 |

Adding the integral term brought back overshoot that was gone and made settling
five times slower, in exchange for fixing an error that was already zero.

The reason is that the term is accumulating on the way to the target. By the
time the error reaches zero, the integral holds a value, and that value keeps
pushing until an error of the opposite sign has cancelled it, which is what
overshoot is.

## Fix

Ask first whether there is a steady offset to remove, which is a question about
the machine rather than about the controller.

Proportional control produces force only while there is error, so anything
needing a **constant** force to hold still stops short by exactly the error that
produces it:

```text
final error  =  load / kp
```

Measured on the same rig with a five newton load and a proportional gain of
twenty: a final error of 0.2500, which is 5/20 exactly. No amount of derivative
gain changes that, because the derivative term is zero once the thing has
stopped moving.

| gains | final error |
|---|---|
| PD 20/8, five newton load | 0.2500 |
| PID 20/6/8, same load | 0.0089 |

So the rule is not "add I to remove steady state error". It is:

- **Is there a steady offset?** Gravity on a vertical joint, a spring, a belt
  under tension, friction that needs breaking. If yes, an integral term is what
  arrives, and `E-CTRL-0001` is what to watch for once you have one.
- **If not, do not add one.** It has nothing to remove and it will cost
  overshoot and settling time to remove it.

The check for whether it is needed costs one experiment: run the loop with P and
D only, and look at where it stops. If it stops on the target, the integral term
has no work.
