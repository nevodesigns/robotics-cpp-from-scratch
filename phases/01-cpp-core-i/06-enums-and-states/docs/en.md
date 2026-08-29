# Enums: Making the Impossible Unrepresentable

> If a robot has four states, a variable that can hold four billion values is the wrong variable.

**Type:** Build
**Time:** about 90 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 01-05

## The Problem

A robot is idle, or driving, or stopping, or faulted. Written with numbers, that
becomes:

```cpp
int state = 0;   // 0 idle, 1 driving, 2 stopping, 3 faulted
```

The comment is the only thing holding it together, and comments do not compile.
Within a month somebody writes `state = 4`, or compares `state` against a
different set of numbers that meant something else, or adds a fifth state and
misses one of the places that switch on it.

Worse, every one of those is legal. An `int` can hold four billion values and
exactly four of them mean anything, so the type is asserting something false
about the program.

The fix is a type that can hold only the four.

## The Concept

### enum class makes a real type

```cpp
enum class DriveState {
  Idle,
  Driving,
  Stopping,
  Faulted,
};
```

`DriveState` is now its own type. Three things follow, and all three are the
point.

**Its values are scoped.** They are written `DriveState::Idle`, so two different
enums can both have an `Idle` without colliding.

**It does not convert to a number by accident.** `state == 3` will not compile,
and neither will passing a `DriveState` to a function expecting an `int`. That is
the whole difference from the older plain `enum`, which converts freely and lets
every one of the mistakes above through.

**A switch over it is checked.** With no `default` label, a compiler warns when a
new value is added and some switch has not been updated. That warning is worth
more than it sounds: adding a state is exactly when you want to be told every
place that has to change.

Converting deliberately is still possible with `static_cast`, and having to write
it is the point: the conversion becomes a decision somebody made rather than
something that happened.

### Leave the default off

```cpp
const char* name(DriveState state) {
  switch (state) {
    case DriveState::Idle:     return "idle";
    case DriveState::Driving:  return "driving";
    case DriveState::Stopping: return "stopping";
    case DriveState::Faulted:  return "faulted";
  }
  return "unknown";   // reached only if somebody casts a bad value in
}
```

Adding `default:` silences the compiler forever, which trades a warning today for
a bug later. Handle every case, and put the fallback after the switch rather than
inside it, where it catches a value cast in from outside without disabling the
check.

### A state machine is a function, not a pile of flags

Two booleans give four combinations, three give eight, and most of them are
nonsense. `is_moving` and `is_stopped` can both be true, and nothing prevents it.

One enum plus one function that decides transitions cannot represent a
contradiction:

```cpp
DriveState next(DriveState current, Event event);
```

Every rule about what may follow what lives in one place, which is where a
reviewer can check it and where a test can cover it. The exercise builds exactly
that, and the rule that carries the safety is the one from lesson 14-02: once
faulted, the machine stays faulted until somebody deliberately resets it.

## Build It

Implement in `exercise/solution.hpp`:

- `name(DriveState)` and `name(Event)`, returning a word for each, with no
  `default` label in the switch.
- `next(current, event)`, the transition table:
  - `Start` moves `Idle` to `Driving`.
  - `Stop` moves `Driving` to `Stopping`.
  - `Arrived` moves `Stopping` to `Idle`.
  - `Fault` moves any state to `Faulted`.
  - `Reset` moves `Faulted` to `Idle`, and does nothing anywhere else.
  - Anything else leaves the state unchanged, which is a decision rather than an
    oversight: an unexpected event is not a reason to move.
- `is_moving(state)`, true only for `Driving` and `Stopping`.
- `accepts_commands(state)`, false when `Faulted`.

```
rcpp verify 01-06
```

## Use It

`ConfigError` in lesson 03-02 is this, used for a different job: a closed set of
reasons something failed. So is `TickQuality` in 03-05 and `ReadError` in 05-02.
Once you see it, a closed set of possibilities is everywhere in robot code.

Larger state machines eventually want a table rather than a switch, and hardware
oriented ones often need each state to carry data, which is what `std::variant`
is for. Both are the same idea grown up: represent what can happen, and nothing
else.

## What Breaks First

- **Comparing an enum class against a number.** It will not compile, and that is
  the type doing its job. Compare against a named value. See `E-CPP-0021`.
- **A new state added and a switch not updated.** Only the compiler notices, and
  only if there is no `default` label. See `E-CPP-0022`.
- **A single equals sign in a condition.** Assignment rather than comparison,
  always true. See `E-CPP-0002`.

## Ship It

`DriveState` joins `rc::control` and is what every later phase reports through.
The safety rule inside it, that a faulted machine stays faulted until somebody
resets it deliberately, is the same principle as the watchdog: recovery is a
decision, never a default.
