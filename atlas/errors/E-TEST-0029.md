id: E-TEST-0029
title: A failure that says the log differs and not where
match: a real change is found after normalising, and the difference says which line
match: expected changes
platforms: linux, windows
teaches: 05-11-the-expected-output-that-changed
---

## Symptom

The comparison fails with "logs differ" and a path to each file. A person opens both, in two windows, and reads three hundred lines to find the one that moved.

## Cause

A yes or no was all the comparison was asked for. The difference between two logs is a list of lines kept, removed and added, and it can be found exactly: the longest common subsequence is the most lines they have in common in order, and everything else is a change.

Measured: with the final speed changed from 1.0 to 1.25, the saved log against the new run gave **2** changes, the line `speed 1.000000` removed and the line `speed 1.250000` added. On 500 random pairs of line lists the difference rebuilt both sides every time and was never longer than the worst case.

## Fix

Print the difference, with a mark on each line, and not a verdict:

```cpp
for (const DiffLine& line : diff(saved, now)) std::cout << line.op << " " << line.text << "\n";
```
