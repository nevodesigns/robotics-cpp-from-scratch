id: E-DEBUG-0004
title: A timing section left open by an early return
match: the guard closes the section on an early return
platforms: linux, windows
teaches: 04-05-which-part-is-slow
---

## Symptom

One section in a timing report has an impossible total, often larger than the
whole loop. The section after it reads close to zero. The numbers change
completely depending on whether a particular branch was taken, and the branch
is an error path that only fires when a sensor is slow.

```cpp
timer.begin("read sensors");
if (!sensor.read(&value)) return false;    // the end never runs
timer.end("read sensors");
```

## Cause

The section was opened and never closed, so its start time was still sitting
there when the next measurement began.

Every way out of a scope that is not the bottom of it does this: an early
return, a `break`, a `continue`, a thrown exception. Error paths are exactly
where early returns live, so the instrument is most wrong precisely when
something is going wrong, which is when you are reading it.

And a begin and end pair separated by forty lines is very easy to leave
unbalanced during an edit that adds a guard clause at the top.

## Fix

**Use a scope guard.** The destructor runs on every path out, which is the
whole point of RAII from lesson 02-03:

```cpp
{
  const auto guard = timer.measure("read sensors");
  if (!sensor.read(&value)) return false;    // still closed
}
```

Measured: a function that gives up halfway recorded 40 microseconds and one
call, and the next section started clean rather than inheriting anything.

**Delete the copy and move operations on the guard.** A section is a place in
the code, and two guards for one section record it twice. The guard is still
returnable by value, because C++17 guarantees the elision for a prvalue, which
lesson 01-11 measured.

**Give the guard its own block when the section is shorter than the
function.** The closing brace is where the measurement ends, so make it
visible rather than relying on the end of the function.

**Never expose a begin and an end at all.** If the only way to open a section
is a function that returns a guard, the unbalanced version cannot be written.
