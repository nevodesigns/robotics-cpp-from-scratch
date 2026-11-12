id: E-FIELD-0025
title: A log that is lost when the power is
match: the log of the failure cannot be read because it ends in the middle of a line
match: a truncated last line stops the reader
platforms: linux, windows
teaches: 22-08-recording-and-replaying-a-run
---

## Symptom

The run that failed is the one whose log will not load.

## Cause

The write was cut by the loss of power and the reader treated the stump as corruption. A cut log read as 999 entries with the truncated flag.

## Fix

End every line with a newline, drop an unfinished last line, and say so.

```cpp
if (!body.empty() && body.back() != '\n') out.truncated = true;
```
