# The Value That Never Changed: What QML Can Follow Across the Boundary

> The same double, exposed four ways, changed from 1.0 to 7.5 in C++. Three of
> the four panels still show 1.0, and not one of them says a word.

**Type:** Build
**Time:** about 135 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 11-01, 09-02

## The Problem

Lesson 11-01 established that QML has three tiers of failure and that the bottom
one has no diagnostic channel at all. This lesson is about the single most
common way to land in that bottom tier, and it is not a QML mistake. It is a
C++ one.

You have a robot. You want a panel showing its speed. You write a `QObject`,
expose the speed, hand it to the engine, and bind a label to it. The panel comes
up with the right number and then never changes again.

## The Concept

### A binding is not a poll

This is the sentence the whole lesson rests on.

When QML evaluates `source.speed`, it reads the property once and subscribes to
whatever signal that property nominates as its `NOTIFY`. Then it waits. It does
not re-read on repaint, it does not re-read on a timer, it does not re-read
because the value changed. It re-reads when it is told.

If the property nominates no signal, there is nothing to subscribe to, and the
binding waits for a message that can never arrive.

### Four ways to expose one number

One `double`, moved from 1.0 to 7.5 in C++, with the binding counting how often
QML re-ran it:

| how it is exposed | C++ now | QML sees | evaluations | warnings |
|---|---|---|---|---|
| NOTIFY, emitted | 7.50 | 7.50 | 2 | 0 |
| no NOTIFY | 7.50 | 1.00 | 1 | 0 |
| NOTIFY, not emitted | 7.50 | 1.00 | 1 | 0 |
| CONSTANT | 7.50 | 1.00 | 1 | 0 |

Three of four wrong. **Zero warnings on every row**, including the wrong ones,
because none of this is an error by any definition Qt has.

The evaluation count is the clearest tell. A working property gets two, one when
the panel is built and one when the value moves. The broken ones get one, and
one means nothing ever told the binding anything.

Notice the three are wrong for genuinely different reasons. `no NOTIFY` never
declared a signal. `NOTIFY, not emitted` declared one and the setter does not
emit it. `CONSTANT` promised the value would never change and then changed it.
From QML they are indistinguishable.

### What the meta object can see

Here is the good news, and it comes straight out of lesson 09-02. Everything in
a `Q_PROPERTY` line is in the meta object, and you can read it before anything
runs:

| class | notifiable | constant | check says |
|---|---|---|---|
| Telemetry | yes | no | can follow |
| NoNotify | NO | no | cannot follow |
| ForgotToEmit | yes | no | can follow |
| ConstantLie | NO | yes | can follow |

So a test can assert, for every type you hand to QML, that every property is
either notifiable or constant. That catches `NoNotify` without running a panel
and tells you which property by name.

And here is the bad news, which matters more than the good news.

**The check clears three of the four, and two of those three are broken.**

`ForgotToEmit` declares `NOTIFY speedChanged`. That declaration is well formed
and the meta object records it faithfully. Whether the setter ever emits it is a
fact about a function body, and the meta object has never seen a function body.
`ConstantLie` declares `CONSTANT`, which is a promise, and nothing static can
tell you a promise was broken.

This is not a shortcoming to apologise for. It is the boundary of what a
declaration can prove, and a check whose boundary you do not know is worse than
no check, because it is where your confidence comes from.

The check that catches all three is four lines and it has to run:

```cpp
source.setSpeed(7.5);
RC_CHECK_NEAR(root->property("shown").toDouble(), 7.5, 1e-12);
```

Keep both. The static one is cheaper, runs on every type automatically, and
names the property. The running one is the only one that proves the path.

### The guard, and what it costs to leave out

The other half of a correct setter is the comparison, and it is not tidiness.

Every emission of a NOTIFY signal invalidates every binding that reads the
property, and QML re-runs all of them. A setter that emits unconditionally does
that on every write, including the writes that changed nothing.

Writing the same value a thousand times:

| setter | binding evaluations |
|---|---|
| no guard | 1002 |
| guarded on equality | 2 |

Both panels show the correct number. One of them did five hundred times the work
to do it, before counting the bindings downstream, the layouts that depend on
their sizes, and the repaints.

A telemetry field written at 100 Hz whose value has not changed is the ordinary
case in robotics. A stationary robot is the worst case for this bug, which is
part of why it gets found late.

### One thing that does work

Replacing a context property object after the panel exists reaches the bindings:
measured going from 0.00 to 42.00 on the replacement.

Worth knowing, and worth a warning. That happens whatever the object does about
NOTIFY, because replacing the context re-evaluates everything in it. A test that
replaces an object and checks the new value would pass against a source QML
cannot follow at all. Mutating the object afterwards is what proves it is
connected.

## Build It

Implement the `Telemetry` properties and setters, `inspect_properties` and
`properties_qml_cannot_track` in `exercise/solution.hpp`.

```
rcpp verify 11-02
```

The suite loads one panel against four sources, prints what each ended up
showing, reads all four meta objects, and counts binding evaluations while
writing the same value a thousand times.

## Use It

**Declare NOTIFY and emit it, and guard the setter.** Three lines, the same
three lines every time:

```cpp
if (value == speed_) return;
speed_ = value;
emit speedChanged();
```

**Assert `every_property_is_trackable` for every type you register**, and
remember it reads declarations.

**Assert the round trip too**, for at least one property per type. It is the
only check that covers the declaration and the setter together.

**Use CONSTANT only when it is true.** It is right for a serial number or a link
length, and it is a promise QML holds you to.

**Count evaluations when a panel feels slow.** A binding that calls a counting
function turns a suspicion into a number.

## What Breaks First

- **A panel showing a value the robot moved on from.** See `E-QT-0021`.
- **A meta object check trusted further than it can see.** See `E-QT-0022`.
- **A setter that makes QML re-evaluate everything.** See `E-QT-0023`.

## Ship It

`PropertyReport`, `inspect_properties`, `properties_qml_cannot_track` and
`every_property_is_trackable` join `rc::qt` beside the QML host. Every type the
operator panel exposes from here on is checked both ways: the declarations
statically, and at least one value through a running binding.
