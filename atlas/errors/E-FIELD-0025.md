id: E-FIELD-0025
title: A log that is lost when the power is
match: the log of the failure cannot be read because it ends in the middle of a line
match: a truncated last line stops the reader
match: a log cut by a power failure still reads, and a bad line stops the read with its number
platforms: linux, windows
teaches: 22-08-recording-and-replaying-a-run
---

## Symptom

The run that failed is the one whose log will not load.

## Cause

The write was cut by the loss of power, and the reader treated the stump as corruption and refused the whole log.

## Fix

End every line with a newline, drop an unfinished last line, and say so. A log cut in the middle of a line read as 999 entries with the truncated flag.

```cpp
const rc::field::LogParse p = rc::field::parse_log(text);
if (p.truncated) note("the last line was cut short and dropped");
```
