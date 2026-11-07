id: E-TIME-0006
title: Two tasks due at the same instant in an order nobody chose
match: a 100 hertz sensor and a 10 hertz logger over ten seconds run exactly as often as they should
match: expected order
platforms: linux, windows
teaches: 03-18-a-loop-you-can-run-in-a-millisecond
---

## Symptom

The logger records the sensor's previous reading, not its current one, but only on the ticks where both are due together. Every tenth sample is stale, and moving the registration order of two tasks changes it.

## Cause

At a shared instant something has to go first. If the tie is broken by when each event was put in the queue, a repeating task is at a disadvantage: it queues its next tick as it finishes the last one, so a slower task whose event has been waiting since long before wins.

The first version of this scheduler did exactly that. At 100 ms, when the 100 hertz sensor and the 10 hertz logger are both due, the suite saw **logger first, then sensor**, though the sensor was registered first. Over ten seconds the sensor ran 1001 times, the logger 101.

## Fix

Break ties by the order the tasks were registered:

```cpp
e.seq = static_cast<unsigned long>(id);    // the task's index, which is its registration order
```
