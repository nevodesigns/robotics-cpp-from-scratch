# The Interface That Reports Itself: A Harness Through Which Every QML Failure Is Data a Test Can Read

> Load QML through one place, and afterwards ask one question: were there any failures? The answer is a list a test can read.

**Type:** Build
**Time:** about 180 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 11-11, 11-10, 11-09, 11-08, 11-07, 11-06, 11-03, 11-01

## The Problem

Each lesson of this phase found a place where QML goes wrong without stopping: a warning in a log, a property set on nothing, a component that is not ready, a value that came too late. This puts the answers in one object, so that a test of an interface loads it through the harness, and fails if anything went wrong, whether QML said so loudly or only to a console.

## The Concept

### The harness

`QmlHarness` loads a file in the background and waits for it (11-07), creates its object with its initial properties set before it starts (11-08), keeps what Qt prints (11-03), and reads, writes and evaluates through the probe that refuses a name that is not a property (11-06). It records its own failures, each with a kind, and adds every warning and critical message that Qt printed while it was alive. `ok()` is whether there are none; `report()` is one line each, as `kind: text`.

### A clean file

A file with a width, a height and a name made its object, had **0** failures and an empty report. A read that worked was not a failure.

### Eight ways to go wrong

Each loaded through the harness in its own engine:

| what | kind | text names |
|---|---|---|
| a syntax error | load | Expected |
| a type that does not exist | load | Buttn |
| a property that does not exist | load | widht |
| a required property that was not given | load | Required property |
| a handler that throws | warning | nothingCalledThis |
| a binding to a name that does not exist | warning | missingThing |
| a binding loop | warning | Binding loop |
| a `console.warn` | warning | careful with this |

**8** of 8 were reported, **8** of the right kind, and **8** with the text that names the mistake. A test that expects one says which. See `E-QT-0093`.

### The probe

A typo read, a typo write and a bad expression: **3** failures of kind `probe`, naming `cuont` and `widht`. A read, a write and an expression that worked were none. An initial property `limt` was a load failure that names it; `clear` forgot it; a good one was then used. See `E-QT-0094`.

### In order

Four steps, three bad: **3** failures in the order they happened, `load`, `probe`, `load`, the last naming `Buttn`; the report had three lines and began `load: `.

### What it cannot see

`Item { width: 0 / 0 }` loaded with **0** failures and a width of **0**. Nothing was printed and nothing was refused, and it is not the width anyone meant. A harness reports what QML says. What QML does not say a test has to ask for, by reading the property. See `E-QT-0095`.

## Build It

`load`, `failures`, `report`, `clear` and the recording of probe failures in `exercise/solution.hpp`.

```
rcpp verify 11-12
```

## Use It

`rc/qt/qml_harness.hpp` joins its module.

```cpp
QQmlEngine engine;
rc::qt::QmlHarness ui(engine);
QObject* panel = ui.load(url, {{"limit", 1.5}});
RC_CHECK(ui.ok());                        // fails with the text of whatever went wrong
RC_CHECK_NEAR(ui.read(panel, "width").value.toDouble(), 320.0, 1e-9);
```

**What this does not do.** It sees what is printed or refused, and not what is simply wrong: a width of zero from a division, an anchor that is not satisfied, an image that is not found and is not an error, a handler that never runs. It is one engine and one thread, and it waits with a nested event loop. A test that provokes a warning on purpose has to look for that one and then clear. And it does not test the look of an interface: lesson 10-12 compares pictures, and those tests are the other half of knowing that an interface is right.

## What Breaks First

- **A failure that only a console shows.** See `E-QT-0093`.
- **A refused probe that nobody noticed.** See `E-QT-0094`.
- **A failure with no message at all.** See `E-QT-0095`.

## Ship It

`rc/qt/qml_harness.hpp` holds `Failure` and `QmlHarness`.

This is what phase 11 was for: a QML interface whose failures are visible from a test rather than from a console nobody is watching. Phase 12 builds on it.
