# The Value It Had at the Start: Initial Properties, Component.onCompleted and the Property That Is Required

> create() runs the object. A property set after it is set after everything that read it. Set the values first, and mark the ones that must be set as required.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 11-06, 11-07

## The Problem

A program makes an interface object from a QML file and then sets its speed limit. Some of the interface shows the new limit and some of it shows the old one, and which is which depends on how the QML was written.

## The Concept

### After create()

A QML item with `limit: 1`, a `doubled: limit * 2` binding and `Component.onCompleted: seen = limit`. A limit of 5 set after `create()`: `onCompleted` saw **1**, the default, because it ran during `create()`; the binding gave **10** because bindings follow. Set as an initial property, `onCompleted` saw **5** and the binding gave **10**, and the completion ran once. See `E-QT-0081`.

Something that reads a value once, at the start, keeps what it read. Something that is bound follows. A change after creation moves one and not the other.

### Required

`required property real limit` in the QML makes forgetting it an error. `create()` without it gave null and the error `Required property limit was not initialized`; `create_with` and an empty map gave the same, with the same text, and no object; given, it made the object and `seen` was 3. See `E-QT-0082`.

### A name that is not a property

Initial properties `limt`, `nothing_like_this` and `limit` were refused as a set: null, and `no property named limt` and the other, and not a word about the right one. The engine alone would have warned and gone on. See `E-QT-0083`.

### A component that is not ready

A syntax error gave null and the reason `Expected token`; a component with no source gave null and `the component is not ready`. A null for the reason is allowed. With nothing to set, `create_with` made the same object as `create()`: the same `seen`, the same `doubled`, one completion.

## Build It

`create_with` in `exercise/solution.hpp`.

```
rcpp verify 11-08
```

## Use It

`rc/qt/qml_create.hpp` joins its module.

```cpp
QQmlComponent component(&engine, url);
QString why;
QObject* gauge = rc::qt::create_with(component, {{"limit", 5.0}, {"label", "speed"}}, &why);
if (gauge == nullptr) { /* why says which name or which required property */ }
```

**What this does not do.** It sets initial properties by name and value, and a value of the wrong type is for the engine to convert or to refuse. It makes one object: a component that is instantiated many times by a `Repeater` or a `Loader` has its values from the QML, or through a `setSource` call with a map. It does not make a property required: that is a decision in the QML file, and a component that is also used elsewhere has to agree. And `completeCreate` runs the handlers, so a handler that throws is in the log (lesson 11-03) and not in the result.

## What Breaks First

- **A property set after the start.** See `E-QT-0081`.
- **A required property that was forgotten.** See `E-QT-0082`.
- **A name that is not a property.** See `E-QT-0083`.

## Ship It

`rc/qt/qml_create.hpp` holds `create_with`.

The harness of the last lesson of this phase creates every object this way.
