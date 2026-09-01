# Choosing the Gains: What Each One Costs, Measured

> Lesson 14-01 built a PID with three numbers in its constructor and never said
> what to put in them. This is that lesson, and the answers are measurements
> rather than advice.

**Type:** Build
**Time:** about 150 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 14-03, 10-02

## The Problem

You have a controller. It takes a proportional gain, an integral gain and a
derivative gain, and nothing so far has said how to pick them.

The usual advice is a paragraph like this: P makes it respond, I removes steady
state error, D damps the oscillation. Every word of that is true and none of it
tells you what to type, because it does not say what each one **costs**.

This lesson runs the experiment. One plant, six sets of gains, four numbers
measured from each, twice: once with a load and once without. The table is the
lesson, and two of its rows contradict advice you will read everywhere.

## The Concept

### First, a plant that can actually misbehave

A first order lag, the thing most tutorials tune against, **cannot overshoot**
however hard you drive it. It has no momentum to carry it past the target. So a
controller tuned against one teaches nothing about the thing that makes tuning
difficult.

The plant here is a mass with a little friction, which is what a joint or a
wheel is:

```cpp
acceleration = (force - load - damping * velocity) / mass;
```

The `load` is what makes a steady offset possible: gravity on a vertical joint,
a spring, a belt under tension. Something that needs a constant force just to
stay still. It matters more than it looks, and the second table below is why.

### Four numbers describe a step response

| number | what it asks |
|---|---|
| rise time | how long from a tenth of the target to nine tenths |
| overshoot | how far past the target it went, as a fraction of the target |
| settling time | when it **last** left the band and stayed inside |
| final error | what is still missing at the end |

A loop is tuned when all four are acceptable, not when any one is best. That is
the whole reason to measure four rather than to watch a chart and feel satisfied.

Two of them have a trap in the definition, and both are in the lesson.

**Overshoot is measured from the target**, not from zero. A peak of 1.8 against
a target of 1.0 is eighty percent past it, not a hundred and eighty. And a
response that stopped short overshot by **nothing**, not by a negative amount.

**Settling is the last exit from the band**, not the first entry. A response
oscillating through the target passes through the band on its way past, and
reporting that as settled says a loop that is still ringing has already
finished. The worse it rings, the earlier the wrong version reports success,
which is the flattering direction and therefore the one nobody questions.

### What the gains cost, on a joint with nothing holding it back

Measured, target of 1.0, band of two percent:

| gains | rise s | overshoot | settle s | final error |
|---|---|---|---|---|
| P 2 | 0.86 | 50.6% | 7.95 | -0.014 |
| P 8 | 0.39 | 71.5% | never | -0.084 |
| P 20 | 0.24 | 81.0% | never | -0.042 |
| PD 20/3 | 0.33 | 25.1% | 1.87 | 0.0000 |
| PD 20/8 | 0.71 | 0.0% | **1.21** | 0.0000 |
| PID 20/6/8 | 0.58 | 10.3% | 6.58 | -0.012 |

Read the first three rows together. Raising the proportional gain makes the rise
faster **and** the overshoot worse, every time, and past a point the loop simply
never settles. It is a trade, not an improvement, and any tuning that only looks
at rise time will keep raising it.

Rows four and five are what the derivative term buys: the overshoot goes from
81 percent to nothing, and the loop finally settles. It costs about half a
second of rise time, and that is the trade being made.

### The row that contradicts the advice

The last row adds an integral term to a loop that was working, and it is
**worse**: overshoot returns, and settling goes from 1.21 seconds to 6.58.

The reason is mechanical. The integral term accumulates while there is error, so
by the time the error reaches zero it holds a value, and that value keeps
pushing until an error of the opposite sign has cancelled it. That is what the
overshoot is made of.

And it bought nothing, because the final error was already 0.0000. There was no
steady offset to remove.

### And the row that justifies it

Now put a five newton load on the same joint:

| gains | final error |
|---|---|
| PD 20/8 | **0.2500** |
| PID 20/6/8 | 0.0089 |

Permanently a quarter of the way short, and no amount of derivative gain changes
it, because the derivative term is zero once the thing has stopped moving.

The number is not arbitrary. Proportional output exists only while there is
error, so the loop stops at exactly the error that produces the force the load
needs:

```text
final error  =  load / kp  =  5 / 20  =  0.25
```

So the rule is not "add I to remove steady state error". It is:

- **Is there something needing a constant force to hold still?** Then an
  integral term is what arrives, and `E-CTRL-0001` is what to watch for once you
  have one.
- **If not, do not add one.** It has nothing to remove, and it will cost
  overshoot and settling time to remove it.

One experiment answers the question: run with P and D only and look at where it
stops.

### An order that works

1. **P alone**, raised until the response is as fast as you want, ignoring the
   overshoot.
2. **D**, raised until the overshoot is acceptable. Rise time will get worse;
   that is the payment.
3. **Look at where it stops.** If it stops on the target, you are finished.
4. **I only if it does not**, raised until the offset closes, then checked for
   the windup from lesson 14-01.

Tuning in simulation first is not a compromise. Ten thousand experiments take a
second here and a day on hardware, and the ones that would have broken something
cost nothing.

## Build It

Implement `analyse` in `exercise/solution.hpp`: the four numbers, from a
recorded response.

```
rcpp verify 14-04
```

The suite checks each number against hand made responses first, including the
two traps in their definitions. Then it runs the whole sweep above and prints it,
with charts of four of the runs, so the table and the pictures are in front of
you together.

## Use It

Any loop you write from here can be measured rather than judged by eye. When a
change makes something feel better, the table says whether it did, and which of
the four it traded away to do it.

The same four numbers are what a tuning report to somebody else should contain.
"It feels good now" does not survive the machine being handed over.

## What Breaks First

- **Settling reported at the first entry into the band.** Every gain set looks
  good. See `E-CTRL-0006`.
- **Overshoot measured from zero.** A perfect response reports a hundred
  percent. See `E-CTRL-0007`.
- **An integral term where there was no offset to remove.** It costs overshoot
  and settling time and fixes nothing. See `E-CTRL-0008`.

## Ship It

`StepResponse`, `analyse` and the `Mass` plant join `rc::control`. From here a
tuning claim in this curriculum comes with four numbers, and the plant is there
to try gains against before anything is asked to move.
