# The Button That Must Be Held: An Enable Switch, Key Repeat That Looks Like Letting Go, and a Robot That Must Not Start by Itself

> The robot moves only while a person holds the key. Key repeat is not a release, a lost window is, and nothing that stopped the robot may start it again by itself.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 12-02, 10-09

## The Problem

A robot driven from a station should move only while someone holds the means to stop it: an enable key that has to be held, and that stops the robot when it is let go. It sounds simple. The keyboard repeats a held key, a window can lose focus with the key still down, and the reasons to stop can all go away while the person is still holding on.

## The Concept

### Key repeat

On some platforms a held key is delivered as a press, then pairs of release and press, all marked as auto-repeat, then the real release. A handler that takes every release as the end of the hold saw the key let go **5** times in a held press; `DeadmanKey` ignores events marked as auto-repeat and saw it let go **0** times before the real release, which ended the hold. A repeat of a held key is neither a press nor a release, and a key that is not the enable key changes nothing. See `E-QT-0114`.

### Focus

When the window loses focus the release of the key goes to another window. The hold is over: after `on_focus_lost()` the key was not held, a release arriving later changed nothing, and the key had to be pressed again.

### What the interlock allows

`Interlock` allows speed only while the key is held, the link is alive and no emergency stop is pressed. Of the **8** combinations of those three, speed was allowed in **1**. The request is held between minus the limit and the limit (9 became 1.5 with a limit of 1.5), and a request that is not a number, or infinite, is no request: 0. A limit below nothing is nothing.

### It must not start by itself

The link is lost with the key held, and comes back with the key still held: the allowed speed was **0**. Only a press arms the interlock, and a press arms it only while the link is alive. A key held down from before the link was good gave **0** too, until it was let go and pressed again. See `E-QT-0115`.

### The emergency stop

Pressing it latches. Letting go of the button left it **latched** and the speed at 0. A reset while the key was held was **refused**, because a reset must not be a start. After the key was let go the reset worked, the speed was still 0 (a reset does not arm), and pressing the key did. See `E-QT-0116`.

## Build It

`DeadmanKey::on_key` and `on_focus_lost`, and `Interlock::set_enable`, `set_link_alive`, `set_estop`, `reset` and `permitted` in `exercise/solution.hpp`.

```
rcpp verify 12-08
```

## Use It

`rc/qt/interlock.hpp` joins its module.

```cpp
rc::qt::DeadmanKey key(Qt::Key_Space);
rc::qt::Interlock interlock(1.5);
// in keyPressEvent and keyReleaseEvent:  key.on_key(*event);  interlock.set_enable(key.held());
// from the heartbeat monitor:  interlock.set_link_alive(health != lost);
const double speed = interlock.permitted(requested_mps);       // what is sent to the robot
```

**What this does not do.** It is the station side of an enable switch, and a station that hangs cannot stop a robot: the robot has to stop on its own when it stops hearing from the station, and the commands of lesson 12-03 need to carry that. It is not a safety function and does not make a robot safe by itself: an emergency stop that matters is a physical one, wired to the power, and a program that calls itself one is not. A key that is not delivered because another program has the keyboard looks like a release. And it knows nothing of two people with two keys.

## What Breaks First

- **A held key seen as released.** See `E-QT-0114`.
- **A robot that starts by itself.** See `E-QT-0115`.
- **A stop that is cleared by letting go.** See `E-QT-0116`.

## Ship It

`rc/qt/interlock.hpp` holds `DeadmanKey` and `Interlock`.

The capstone of this phase drives its robot through them.
