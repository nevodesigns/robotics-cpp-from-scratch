# The Property Set on Nothing: A Misspelt Name, a Binding That Was Assigned Over, and a Probe That Says So

> Setting a property that does not exist succeeds in making one. Assigning to a bound property ends the binding. Make a test say so when either happens.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 11-03, 11-02

## The Problem

A test of a QML interface sets a property from C++ and asserts on what the interface did. If the property name has a typo the test sets nothing, the interface does nothing, and the assertion that the interface did not change passes. This is the quietest way for a test to be wrong.

## The Concept

### A name that does not exist

`setProperty("widht", 300)` on a QML item: it returned **false**, a dynamic property named `widht` was **created**, and `width` was still **200**. `property("nothingLikeThis")` was invalid, and converted to a double it is **0**. A test that ignores the return value, which most do, has set a property on nothing. See `E-QT-0075`.

The probe refuses: `set(object, "wdth", 300)` was not ok, with the message `no property named wdth`, and created nothing. A misspelt `read` is an error and not an empty value. A property that exists and cannot be written, a QML `readonly property`, is `fixed cannot be written` and keeps its 7. A string `"wide"` written to `width` was refused and the width stayed **200**, and a number that fits was written.

### A binding assigned over

An inner item whose `width` is `outer.width / 2`:

| step | inner width |
|---|---|
| outer is 300 | **150** (it follows) |
| `width = 10`, then outer is 400 | **10** (it does not) |
| `width = Qt.binding(function() { return outer.width / 2 })` | **200** |
| outer is 500 | **250** (it follows again) |

An assignment gives the property a value and takes away what it followed, with no error and no warning. See `E-QT-0076`.

### Evaluating

`evaluate(engine, object, "width * 2")` ran in the object's own QML: 200, and `outer.height + 1` through the id of the file was 101. An undefined name was `ReferenceError: nothingCalledThis is not defined`, a syntax error was `SyntaxError: Expected token \`numeric literal'`, a throw was `Error: boom`, and an empty expression and no object were each an error. None of them threw and none was ok. See `E-QT-0077`.

## Build It

`evaluate`, `read` and `set` in `exercise/solution.hpp`.

```
rcpp verify 11-06
```

## Use It

`rc/qt/qml_probe.hpp` joins its module.

```cpp
const rc::qt::ProbeResult r = rc::qt::set(panel, "speedLimit", 1.5);
RC_CHECK(r.ok);                       // a typo in the name is a failure here, with the name in r.error
RC_CHECK_NEAR(rc::qt::read(panel, "speedLimit").value.toDouble(), 1.5, 1e-12);
```

**What this does not do.** It reports; it does not decide. A test that expects an error asserts on `ok` being false and on the text. It reads through `QQmlProperty`, so a property of a C++ object that has no `Q_PROPERTY` is not one, and a dynamic property is not either. It does not tell you that a binding exists or has been replaced: the test above does that by changing the source and looking, which is the only portable way. And an expression with side effects has them.

## What Breaks First

- **A property set on nothing.** See `E-QT-0075`.
- **A binding that was assigned over.** See `E-QT-0076`.
- **An expression that failed quietly.** See `E-QT-0077`.

## Ship It

`rc/qt/qml_probe.hpp` holds `ProbeResult`, `evaluate`, `read` and `set`.

The harness of the last lesson of this phase reads and writes the interface only through it.
