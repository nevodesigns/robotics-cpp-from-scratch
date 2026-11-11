# The Enum That Was Undefined: Naming a C++ Enum in QML, and the Integer It Accepted

> A C++ enum is not a name in QML until it is declared to the meta object. And in QML it is an integer, whatever number is put in it.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 11-06, 09-02

## The Problem

A robot has modes, and the interface wants `mode: RobotMode.Auto`. Writing the enum in C++ is not enough for QML to know it, and when it does know it, a mode is just a number that anything can set to anything.

## The Concept

### Declared and not

Two namespaces of the same shape, one with `Q_NAMESPACE` and `Q_ENUM_NS` (registered as `RobotMode`) and one with only the enum (registered as `PlainMode`). In QML:

| expression | value |
|---|---|
| `RobotMode.Auto` | **1** |
| `RobotMode.Fault` | **9** (the C++ value, not 3) |
| `PlainMode.Auto` | **undefined**, and the file loaded |

Nothing is reported when the second is read: a binding on it is `undefined`, which becomes 0, and the interface does the wrong thing quietly. See `E-QT-0084`.

### Names from the meta object

`mode_name` reads the key of a value from the meta object, so it cannot disagree with the enum: 0, 1, 2 and 9 are Idle, Auto, Manual and Fault, and 3, 99 and -1 are `unknown`. `mode_names` gave Idle, Auto, Manual, Fault in order.

### Reading a name

`mode_from_name` is exact. `Auto` and `Idle` were read; `auto`, `AUTO`, `Auto ` with a space, the empty string, `Fault2` and `1` were each refused, and the result was left alone every time. See `E-QT-0086`.

### Any number

A property that holds a mode, assigned `RobotMode.Manual` and then `99` from QML, stored **99**: nothing in QML refused it. Asked what mode that is, the meta object says `unknown`, and `is_valid_mode` is false. The check belongs in the C++ that receives the number. See `E-QT-0085`.

### Registering

`register_mode` registers once, and asking again, or under another name, does nothing. A second engine, made later, still has the names.

## Build It

`mode_name`, `is_valid_mode`, `mode_from_name`, `mode_names` and `register_mode` in `exercise/solution.hpp`. The enum and its `Q_ENUM_NS` are supplied.

```
rcpp verify 11-09
```

## Use It

`rc/qt/robot_mode.hpp` joins its module.

```cpp
rc::qt::robot_mode::Mode requested;
if (!rc::qt::mode_from_name(text, &requested)) { /* refuse, and say the names there are */ }
setMode(value); // in a setter: if (!rc::qt::is_valid_mode(value)) return;
```

**What this does not do.** It names the modes of one enum. A namespace has one meta object, so another enum in the same namespace would share its registration, and an enum that is a member of a class uses `Q_ENUM` and is named through the class. The numbers are the C++ values, which lets a mode be stored and sent as a number (lesson 09-07), and a number that comes from outside has to be checked. And a file that imports a module that was not registered is an error at load: it is the names inside it that are quiet.

## What Breaks First

- **An enum that QML cannot name.** See `E-QT-0084`.
- **Any number accepted as a mode.** See `E-QT-0085`.
- **A name read loosely.** See `E-QT-0086`.

## Ship It

`rc/qt/robot_mode.hpp` holds `robot_mode::Mode`, `mode_name`, `is_valid_mode`, `mode_from_name`, `mode_names` and `register_mode`.

The harness of the last lesson of this phase checks the names a file uses.
