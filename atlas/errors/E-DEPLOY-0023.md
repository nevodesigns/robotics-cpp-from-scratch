id: E-DEPLOY-0023
title: A restart with no wait, or a limit that gives up in a second
match: a service that crashes at once, restarted immediately, starts hundreds of times a minute, and a back-off starts it six times
platforms: linux, windows
teaches: 20-08-boot-into-your-code
---

## Symptom

The disk is full of one error message, or a program that crashed once is gone from the robot for good.

## Cause

A service crashing 0.2 s after each start, restarted after 0.1 s, started 201 times in 60 s; with a limit of 5 in 10 s it gave up at 1.5 s; with waits of 1, 2, 4, 8, 16 and 30 s it started 6 times and was never given up on.

## Fix

Back off, and make a give up visible.

```cpp
policy.factor = 2.0;
```
