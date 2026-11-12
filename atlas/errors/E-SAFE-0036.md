id: E-SAFE-0036
title: A stop that was logged with a reason and nothing before it
match: the events before a stop show how it came about, even after the ring has wrapped many times
match: a clock that went back is counted and the order is kept by the sequence number, and a box of no size is harmless
platforms: linux, windows
teaches: 19-12-what-happened-before-the-stop
---

## Symptom

An incident is investigated and the log says the fence stopped the robot, and nothing about why it was at the fence at that speed.

## Cause

A drive at a wall recorded 235 events with 171 overwritten, and the log verified. The six events before the stop showed the range at 0.90, 0.75 and 0.60 with the speed at 1.50 each time. Times 5, 4, 6 were kept in sequence order and counted as 1 time going back.

## Fix

Record the inputs every tick, and read the window before the stop.

```cpp
const auto last = window_before(dump, kStopCode, 50);
```
