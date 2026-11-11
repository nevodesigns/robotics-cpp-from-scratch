# A Node With a Life Cycle: Configure, Activate, and What Failure Does to Each Step

> A robot that starts every node at once and hopes is a robot whose start up order is a matter of luck.

**Type:** Build
**Time:** about 130 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 17-17, 17-10, 01-06, 02-08

## The Problem

A node in the last lesson is created and starts at once. A driver for a camera cannot: it has to find the device, open it, read its settings, and only then begin to publish, and any of those can fail. A node that starts publishing before its device is open publishes nothing useful, or garbage. A node that fails halfway has opened half of what it needs and leaks it.

ROS 2's answer is the **managed node**, or lifecycle node. Its start up and shut down are steps, taken one at a time, from outside, each of which can fail, with defined states between. A supervisor (the next lesson) can bring a whole robot up in order, notice which node did not make it, and take the rest down cleanly.

## The Concept

### Four states and five steps

```
configure:  unconfigured -> inactive      (gets what it needs: opens the device, reads parameters)
activate:   inactive -> active            (starts its work)
deactivate: active -> inactive            (stops, but keeps what it has)
cleanup:    inactive -> unconfigured      (gives it all back)
shutdown:   unconfigured, inactive or active -> finalized
```

An **inactive** node is configured and idle: its device is open and it is not publishing. The distinction is what makes a safe pause possible: deactivate stops the output without the cost of closing and reopening the hardware. The way up and down went through exactly these states:

```
-> inactive -> active -> inactive -> unconfigured -> finalized
```

Each step runs the node's own callback for it, and the node's author puts the real work in them. See `E-ROS-0004`.

### A request from the wrong place

A transition can only start from certain states. Asked from any other, it is **refused, and its callback is not run**. Six requests out of order (activate before configure, deactivate before activate, cleanup too early, configure twice, cleanup while active, activate twice) were all refused, and the only callbacks that ran were the configure and the activate that were legal:

```
six requests out of order: legal 000000; the callbacks that ran: configure activate
```

If the callback ran anyway, activating an unconfigured node would try to start a device that was never opened. See `E-ROS-0004`.

### Three answers from a callback

A callback returns **success**, **failure** or **error**, and they are not the same:

| says | the node ends up |
|---|---|
| success | in the state the transition leads to |
| failure | where it was: nothing changed, and it may be tried again |
| error | in error processing, and its own recovery decides |

A configure that failed left the node unconfigured, and when the device came back the same request took it to inactive. An activate that failed left it inactive; a deactivate that failed left it **active**, still running because it could not stop; and a failed shutdown left it active: a node does not become finalized because it was asked and could not comply. See `E-ROS-0005`.

### Error

An **error** is a different thing from a failure: it says the node's own state can no longer be trusted. The node goes to **error processing**, and the node's error callback decides. If recovery succeeds the node goes back to unconfigured, as if new, and can be configured again. If it does not, the node is finalized, and nothing more can be asked of it:

```
an error with a recovery that works: unconfigured, brought up again; an error whose recovery fails: finalized
```

A finalized node refuses every later request, including a second shutdown. See `E-ROS-0005`.

### Shutdown from anywhere

A robot has to be able to stop whatever state a node is in: shutdown is legal from unconfigured, inactive and active, and each gave finalized. Not from finalized: there is nothing left to do. See `E-ROS-0006`.

### Speaking only when active

The last piece is what makes the states matter on the bus. A **lifecycle publisher** only sends while the node is active. Five messages sent over the life of one node: one while unconfigured, one while inactive, two while active, one after being deactivated:

```
on the wire: c d; sent 2, dropped 3
```

The three dropped are counted and not silent. A node that published while inactive would send readings from a device that was not open, or that had just been told to stop. See `E-ROS-0006`.

## Build It

`trigger` and `publish` in `exercise/solution.hpp`. The states, the callbacks and the legality table are given.

```
rcpp verify 17-18
```

Nothing is timed.

## Use It

`rc/ros/lifecycle.hpp` opens with `rc::ros`.

```cpp
LifecycleNode camera;
camera.on(Transition::configure, [&] { return open_device() ? CallbackResult::success : CallbackResult::failure; });
camera.trigger(Transition::configure);
camera.trigger(Transition::activate);
camera.publish(frame, send);
```

**What this does not do.** Real lifecycle states include the transitional ones (configuring, activating and the rest) that a node is in while its callback runs, and a callback that takes time is seen from outside in one. It has one error callback for all transitions where a real one is told which transition failed. It does not tell anybody when the state changes, where a ROS 2 managed node publishes a transition event. And it does not say what a callback should do: the leak of a configure that fails
halfway is the callback's author's to avoid, by giving back what it took before it returns failure.

## What Breaks First

- **A transition run from the wrong state.** A device started that was never opened. See `E-ROS-0004`.
- **A failure treated as an error, or the reverse.** A node abandoned that could have been retried, or one retried that could not be trusted. See `E-ROS-0005`.
- **A publisher that speaks while inactive.** Output from hardware that is not ready. See `E-ROS-0006`.

## Ship It

`rc/ros/lifecycle.hpp` holds `LifeState`, `Transition`, `CallbackResult`, `TransitionResult`, `name_of` and `LifecycleNode`.

A node's start up and shut down are now steps with answers, and something can drive them.
