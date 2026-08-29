id: E-TIME-0002
title: Seconds and milliseconds confused
match: expected .*timeout
match: expected .*to be within .* of 0\.001
platforms: linux, windows
teaches: 03-05-time-and-clocks
---

## Symptom

A timeout fires immediately or never. A control loop runs a thousand times too
fast or too slow. A watchdog that should expire in half a second expires in eight
minutes.

## Cause

A duration crossed an interface as a plain number, and the two sides disagreed
about its units. One wrote seconds, the other read milliseconds. Nothing checks
this, because a double is a double.

A factor of a thousand in a timeout is not a subtle error, and it has damaged
real hardware.

## Fix

Carry durations in a type that knows its own units, so the compiler converts or
refuses:

```cpp
void set_timeout(std::chrono::milliseconds timeout);

set_timeout(std::chrono::seconds(2));        // fine, converted
set_timeout(2);                              // will not compile
```

Convert to a plain double exactly once, at the boundary where the value becomes
an input to arithmetic:

```cpp
const double dt = std::chrono::duration<double>(now - last).count();
```

Where a plain number is unavoidable, put the unit in the name: timeout_ms rather
than timeout. It is weaker than a type and much better than nothing.
