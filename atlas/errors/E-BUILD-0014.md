id: E-BUILD-0014
title: A setting from a weaker source that overwrote a stronger one
match: a weaker source never replaces a stronger one, whatever order they arrive in
match: expected v to equal
platforms: linux, windows
teaches: 04-12-where-did-this-value-come-from
---

## Symptom

A port is given on the command line and the program uses another. The config file was read after the arguments, and it quietly won. Or an environment variable is ignored on one machine and honoured on another, depending on which part of start-up ran first.

## Cause

If each source simply stores its values as it is read, the last one read wins, and the order of reading is an accident of how main was written. What should decide is which source is meant to win.

Measured: the command line said 9000, then the file 8080, then the default 80. The port was **9000**, from the command line. The same source twice behaved as a file does: the later line (10) beat the earlier (5).

## Fix

Give each source a rank and refuse to overwrite a stronger one:

```cpp
if (it != entries_.end() && it->second.source > source) return;
```
