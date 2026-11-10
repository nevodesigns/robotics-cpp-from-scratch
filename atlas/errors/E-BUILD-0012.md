id: E-BUILD-0012
title: A rebuild caused by a touch or a revert
match: build, edit, revert: the timestamp rule compiles three times and the content cache twice
match: expected time_rebuilds
platforms: linux, windows
teaches: 04-11-the-build-that-rebuilt-nothing-or-everything
---

## Symptom

Switching branches and back, or reverting a change, rebuilds half the project though the files end up exactly as they were. A large project takes minutes to come back to a state that was built an hour ago.

## Cause

Every write gives a file a new timestamp, whatever it wrote. A touch, a revert and a checkout all look like changes to a timestamp rule.

Measured: a file built, edited and reverted was compiled **3** times by timestamp and **2** by content: the revert found the object of the first build, a cache hit. After a plain touch the timestamp said rebuild and the content key hit.

## Fix

Key the cache by content, so that going back to a state that was built before finds what was built then.
