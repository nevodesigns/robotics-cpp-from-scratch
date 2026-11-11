# The Log Nobody Could Search: Categories, Filter Rules, a Severity That Is Not in Order and a Log That Keeps the Last Thousand

> Bound the log, give every message a category, and put severity in order: information is numbered above critical.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 11-03, 12-02

## The Problem

A station that has run for a week has a log that nobody can open, or one that was cut off at the disk limit, and the message that matters is among a hundred thousand that do not. The log has to be bounded and has to be searchable by part of the program and by how bad the message is.

## The Concept

### Categories and rules

A message belongs to a category such as `station.link` or `station.plan`, and a rule turns a category off. With `station.link.debug=false`, one debug message from each category gave **1** kept, the plan's, and the arguments of the message were worked out **1** time: a disabled message does not print, and does not evaluate what it was going to print. A message whose arguments do something, such as counting, sending or removing, does it only when the category is on. See `E-QT-0108`.

### The last rule wins

`*.debug=false` followed by `station.link.debug=true`: **1** message from the link and **0** from the plan. The same two rules the other way round: **0** and **0**. A general rule that comes last takes back what a specific one said. See `E-QT-0109`.

### Severity

The values of `QtMsgType` are: debug **0**, warning **1**, critical **2**, fatal **3** and info **4**. Information is numbered above a fatal error. Of a debug, an info, a warning and a critical, the ones at least as bad as a warning are **2**; the filter `type >= QtWarningMsg`, written on the numbers, gave **3**, the information among them. `severity` puts them in the order of how bad they are: 0, 1, 2, 3, 4. See `E-QT-0110`.

### The ring

`LogRing` keeps the last `capacity` messages, oldest first, and counts what it let go: **5000** messages into a ring of 1000 left **1000**, the first of them number 4000 and the last 4999, in order, and **4000** dropped. A capacity of nothing is one. A query by category prefix and severity: the link had **2002** messages (**2000** of them from four threads that printed 500 each), **1** critical; the plan had **1** at a warning or worse, and its information was not one; the prefix `station` took both parts, **2004**.

### Hooking it up

`LogHook` sends everything Qt prints to a ring, with the category from the context of the message, for as long as it exists, and puts the old handler back.

## Build It

`severity`, `LogRing::push` and `LogRing::entries` in `exercise/solution.hpp`. `LogHook` is supplied.

```
rcpp verify 12-06
```

## Use It

`rc/qt/log_ring.hpp` joins its module.

```cpp
rc::qt::LogRing ring(2000);
rc::qt::LogHook hook(ring);
// for a window that shows "the link, warnings and worse":
for (const rc::qt::LogEntry& e : ring.entries(QtWarningMsg, "station.link")) show(e);
```

**What this does not do.** It keeps text in memory and writes nothing: a station that must keep the log of a run needs a file, rotated, and the ring is for the screen. It is bounded in the number of messages and not their length, so a message of a megabyte is a megabyte. Rules turn categories off for the whole process, and a station that turns debug on to look at one problem turns it on everywhere that matches, and has to turn it off. It holds the messages of Qt and of the program that go through the message handler, and not what a library writes straight to standard error. And a ring that keeps the last thousand has lost the first, which is sometimes the cause.

## What Breaks First

- **A message that does something, in a category that is off.** See `E-QT-0108`.
- **Rules in the wrong order.** See `E-QT-0109`.
- **A severity compared as a number.** See `E-QT-0110`.

## Ship It

`rc/qt/log_ring.hpp` holds `LogEntry`, `severity`, `LogRing` and `LogHook`.

The capstone of this phase has a log pane that is a ring.
