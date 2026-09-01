id: E-CTRL-0006
title: Settling time reported at the first entry into the band
match: settling is the last time it left the band
match: a response still outside the band at the end never settled
platforms: linux, windows
teaches: 14-04-choosing-the-gains
---

## Symptom

A tuning report says the loop settles in 0.2 seconds. Watching the machine, it
rings for three, visibly, and an operator can see it.

Every gain set looks good, so the numbers stop being used, which is worse than
not having had them.

## Cause

Settling was measured as the **first** time the output was inside the band:

```cpp
for (std::size_t i = 0; i < output.size(); ++i) {
  if (std::fabs(output[i] - target) <= tolerance) {
    settling_time = i * dt;      // the bug
    break;
  }
}
```

An oscillating response passes **through** the target on its way past it. The
first time it is inside the band is on that first crossing, near the start, with
all the ringing still to come.

So the worse a loop oscillates, the earlier this reports it as settled, and the
error is in the flattering direction, which is the direction that does not get
questioned.

## Fix

Settling is the last time it was **outside** the band:

```cpp
long last_outside = -1;
const double tolerance = std::fabs(band * target);
for (std::size_t i = 0; i < output.size(); ++i)
  if (std::fabs(output[i] - target) > tolerance) last_outside = i;
```

Then two cases that are not the same and are easy to merge by accident.

**Still outside on the final sample** means it never settled. Reporting the end
of the recording is reporting the length of your experiment rather than a
property of the loop, and a longer recording would give a different answer for
the same gains.

**Never outside at all** means it settled immediately, which is a real answer
and not an error.

```cpp
if (last_outside < 0) settling_time = 0.0;                       // never left
else if (last_outside + 1 < output.size())                       // and came back
  settling_time = (last_outside + 1) * dt;
// otherwise it never settled, and the value stays negative
```

Report "never" rather than a number when it did not settle. A tuning table
where one column occasionally says never is far more useful than one where
every row has a plausible figure.
