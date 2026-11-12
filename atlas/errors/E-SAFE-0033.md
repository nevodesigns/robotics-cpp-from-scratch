id: E-SAFE-0033
title: A test interval chosen by habit
match: the chance of an undetected failure grows with the time since the last test, and the interval follows from a target
platforms: linux, windows
teaches: 19-11-prove-it-still-works
---

## Symptom

The stop function is tested at each service, once a year, because that is when things are tested, and the chance of it being dead at any moment is never asked.

## Cause

At a rate of 1e-5 per hour the chance of an undetected failure was 1.00e-05 after an hour, 2.40e-04 after a day, 1.68e-03 after a week, 7.27e-03 after a month and 8.39e-02 after a year. To keep it at 1 in 1000 the interval was 100.1 hours, and 1001 hours at a rate of 1e-6.

## Fix

Work the interval out from a rate and a target.

```cpp
const double hours = max_interval(rate_per_hour, 1e-3);
```
