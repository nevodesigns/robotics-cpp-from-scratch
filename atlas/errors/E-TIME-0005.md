id: E-TIME-0005
title: Ticks lost because time was added up as a double
match: whole nanoseconds land exactly where seconds as doubles do not
match: expected double_ticks to equal
platforms: linux, windows
teaches: 03-18-a-loop-you-can-run-in-a-millisecond
---

## Symptom

A task meant to run every tenth of a second for 0.3 seconds runs three times instead of four. Over a long run a task is a tick short, or a tick long, and the number changes if the period changes by a hair.

## Cause

0.1 is not exact in binary, and adding it three times gives 0.30000000000000004, which is later than 0.3. A loop that tests `t <= end` after adding steps loses the last tick.

Measured: a tick every 0.1 s up to 0.3 s ran **4** times with whole nanoseconds and **3** times with doubles added up.

## Fix

Keep time as whole nanoseconds (`std::int64_t`, lesson 03-05). A period of a tenth of a second is 100000000, three of them are exactly 300000000, and the comparison is exact.
