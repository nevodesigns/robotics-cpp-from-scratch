id: E-TEST-0027
title: A saved log that no run ever matches
match: two runs of the same program differ in every line until the log is normalised, and not after
match: expected normalized_different
platforms: linux, windows
teaches: 05-11-the-expected-output-that-changed
---

## Symptom

A test compares the log of a run with a file saved from a good run. It passes once, on the machine where the file was made, and fails ever after, on every other. The difference is a number in every line.

## Cause

A log carries things that are true of one run only: the time, a memory address, a process number. The comparison treats them as part of the answer.

Measured: two runs of the same program, 4 lines each: **4** lines differed as written and **0** after the time (`t=<number>`) and an address (`0x...`) were replaced by fixed words.

## Fix

Replace what is volatile with a fixed word before comparing, and nothing more:

```cpp
out += "t=<time>";            // for t= followed by a number
out += "0x<addr>";            // for 0x followed by hexadecimal digits
```
