# The Handler That Outlived Its Item: Connections Made Twice, and in Items That Are Gone

> A connection lasts until it is broken. Connect once, keep the connections of a scope together, and in QML let a Connections element say what a handler belongs to.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 11-03, 09-01

## The Problem

An interface part is created, destroyed and created again as the user moves about, and each time it connects to a sensor. The robot works, and then a handler runs twice, and then a log fills with errors from parts of the interface that are no longer on the screen.

## The Concept

### Twice

One signal and the same slot connected three times ran it **3** times. `connect_once` made the first connection (true) and refused the second and the third (false), and the slot ran **1** time. It works for member function pointers, because `Qt::UniqueConnection` recognises a connection by its sender, signal, receiver and slot; a lambda is a new function every time. See `E-QT-0087`.

### In an item that is gone

An item that connects in `Component.onCompleted` with `sensor.reading.connect(function() { tally.hit() })`, loaded in a `Loader`, then reloaded three times, then one signal:

- the handler of the live item ran **1** time,
- and **3** warnings came from the handlers of the three items that were gone, each a `ReferenceError`, because the names those handlers used went with their items, and the handlers did not.

See `E-QT-0088`. With `Connections { target: sensor; function onReading() { tally.hit() } }` inside the item: the live handler ran **1** time and there were **0** warnings. The element is part of the item and goes with it. The warnings were counted with the capture of lesson 11-03.

### Keeping them together

`ScopedConnections` keeps the connections made in a scope, a member function and a lambda with no receiver among them, and does not keep one that failed to connect: **2** kept. A signal made **2** calls. After `clear` there were **0** kept and a signal made no call; a new connection was made, ran, and when the scope was destroyed one more signal made no call: **12** calls in all. See `E-QT-0089`.

### A receiver

A lambda connected with a receiver as its context ended when the receiver was deleted: one more signal made no call (**1** in all). With no receiver, it lasts until something ends it: the scope's `clear` took a handler from 101 calls to no more.

## Build It

`connect_once`, and `ScopedConnections::add`, `count` and `clear`, in `exercise/solution.hpp`.

```
rcpp verify 11-10
```

## Use It

`rc/qt/connections.hpp` joins its module.

```cpp
rc::qt::ScopedConnections connections;
connections.add(QObject::connect(&sensor, &Sensor::reading, this, &Panel::on_reading));
rc::qt::connect_once(&sensor, &Sensor::faulted, this, &Panel::on_fault);
```

**What this does not do.** It does not make a lambda connection unique: two lambdas that do the same thing are two connections. It does not know about a QML `connect`, which is a connection made by JavaScript in a context that can be destroyed: for QML the answer is `Connections`, or `disconnect` in `Component.onDestruction`. A scope that ends does not delete what the lambdas captured, so a lambda has to capture only what outlives the scope. And a connection that is made again after `clear` is a new one, with the same effect as the first.

## What Breaks First

- **The same slot connected again.** See `E-QT-0087`.
- **A handler of an item that is gone.** See `E-QT-0088`.
- **A connection with nothing to end it.** See `E-QT-0089`.

## Ship It

`rc/qt/connections.hpp` holds `connect_once` and `ScopedConnections`.

The harness of the last lesson of this phase counts the warnings that a stale handler would cause.
