# What moc Generates, and the Error When It Does Not

> This lesson ships broken on purpose, because the fastest way to never fear an error again is to cause it deliberately.

**Type:** Build
**Time:** about 75 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Qt:** 6.2 or newer
**Hardware:** none
**Prerequisites:** 09-01

## The Problem

A Qt class missing one macro fails in two completely different ways, and which
one you get depends on how the signal is used. On the toolchain this repository
targets, both look like this.

Connecting with the modern syntax stops at compile time:

```
error: static assertion failed: No Q_OBJECT in the class with the signal
```

Emitting the signal without such a connection gets all the way to linking:

```
undefined reference to `ThresholdSensor::crossed(double)'
```

Every Qt developer has lost an hour to one of these. Neither is a mysterious
build problem. They have the same three causes, and each takes ten seconds to
check once you know what the tool that produced them was trying to do.

So this lesson starts broken. Your first job is to read the error properly, and
only then to fix it.

## The Concept

### The generated file

`Q_OBJECT` is a macro that expands to declarations, not definitions. It says
"this class has a virtual metaObject function, a static metadata table, and
these signals exist". It declares all of it and defines none of it.

**moc** supplies the definitions. It reads your header, and writes a `.cpp`
file containing:

- the static meta object table, which holds the names and argument types of
  every signal and slot as text, so `connect` can match them at runtime
- the implementation of `metaObject()`, `qt_metacast` and `qt_metacall`
- **a real function body for every signal you declared**, which packages the
  arguments and calls everything currently connected

That last one is the part people miss. A signal is not special syntax. It is an
ordinary member function whose body somebody else writes. Declare it, never let
moc see it, and the linker looks for a body that nobody wrote.

### Two errors, one cause

Qt 6 added a compile time check for this, which is why the modern connect syntax
gives you a sentence in English rather than a linker error. The static assertion
fires because connect is handed a pointer to a member of a class that has no meta
object, and Qt can see that at compile time.

The link error is what happens when nothing forces that check: an emit compiles
happily, because the signal was declared, and only the linker notices that its
body was never written. That is the older and more famous form, it is what you
get in a class that only emits, and it is still what you will meet in code that
uses the string based connect syntax.

Both mean the same thing. moc did not generate code for this class.

### So the error has exactly three causes

1. **The class has no `Q_OBJECT`.** moc scanned the header, found nothing to do,
   and wrote nothing.
2. **moc never saw the header.** With CMake, `AUTOMOC` must be on and the header
   must belong to a target. This repository lists lesson headers as target
   sources in `cmake/RcLesson.cmake` for exactly this reason.
3. **You added `Q_OBJECT` and did not reconfigure.** The build system decided
   which files to moc when it last ran. Reconfigure, or clean and rebuild.

Read in that order, the error stops being frightening. It is the build system
telling you it was never asked to generate something you are using.

### Signals you can inspect

Because moc stores signal names as text, a Qt object can be asked about itself
at runtime:

```cpp
sensor.metaObject()->indexOfSignal("crossed(double)");   // -1 when absent
```

That is how the tests in this lesson prove that moc actually ran, rather than
just checking that your code compiles.

## Build It

`exercise/solution.hpp` contains a `ThresholdSensor` that is missing something.

The logic in `feed` is already written for you. Exactly one thing is missing, and
it is the thing this lesson is about.

First, cause the error and read it:

```
rcpp verify 09-02
```

Copy the first error line, not the last, and run:

```
rcpp explain
```

Then add what is missing and verify again. The tests confirm moc ran by asking
the meta object for the class name and for the signal signature, which is
something a class without the macro cannot answer.

When it passes, break it again on purpose, twice, because the whole point of this
lesson is that you never fear either error again:

1. Remove the macro and comment out the two `QObject::connect` calls in one test.
   Now nothing uses the pointer to member syntax, so the compile time check never
   fires and you get the link error instead.
2. Put the macro back but delete your build directory without reconfiguring, then
   build. Watch the build system work from its previous decision.

## Use It

Every Qt class you write for the rest of this curriculum follows this shape. The
same three causes explain the same error every time, whether the class is a
widget, a serial port reader, or a control panel.

When you later meet a Qt project using `qt_add_executable` or a `.pro` file
rather than this repository's CMake, the question does not change: was this
header given to moc?

## What Breaks First

- **undefined reference to a vtable or to a signal.** One of the three causes
  above. See `E-QT-0001`.
- **connect compiles but returns false at runtime.** The signature you named does
  not match the one moc registered, usually a missing or extra argument type.
  See `E-QT-0003`.
- **You added Q_OBJECT and the error did not change.** The build system is still
  working from its previous decision. Reconfigure. See `E-CMAKE-0001`.

## Ship It

The corrected `ThresholdSensor` is the shape every `rc::qt` component copies:
inherit `QObject`, declare `Q_OBJECT` first, keep the state private, and announce
changes rather than reaching out to whoever needs to know.
