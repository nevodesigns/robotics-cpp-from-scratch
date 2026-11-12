id: E-DEPLOY-0026
title: One message said a million times
match: one error said a thousand times a second is written five times and then counted, and said again after the window
platforms: linux, windows
teaches: 20-09-a-log-that-cannot-fill-the-disk
---

## Symptom

The log is nothing but one line, and the program spends its time writing it.

## Cause

An error repeated 1000 times a second for 10 s was written 5 times and suppressed 9995 times, with the count handed over once; another message was not held back and a message after the window opened a new one.

## Fix

Write the first few and count the rest.

```cpp
if (guard.allow(id, now)) write(line);
```
