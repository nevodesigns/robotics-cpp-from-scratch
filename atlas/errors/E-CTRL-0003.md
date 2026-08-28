id: E-CTRL-0003
title: A watchdog that trusts before it has been fed
match: expected .*expired
platforms: linux, windows
teaches: 14-02-limits-and-watchdog
---

## Symptom

A freshly constructed watchdog reports that it has not expired, so a robot whose
command source never started runs on default values with nothing objecting.

## Cause

The expiry check compares the current time against the time of the last feed. If
that time starts at zero and is treated as a real feed, the watchdog begins life
believing it was just fed.

## Fix

Track whether the watchdog has ever been fed, and report expired until it has.
A watchdog that starts out trusting is worse than none, because it hides the
worst failure case rather than catching it.
