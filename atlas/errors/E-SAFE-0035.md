id: E-SAFE-0035
title: A log that cannot tell what is missing from what was changed
match: an altered event, a deleted one and two swapped are each found, and the newest events cut off are not
platforms: linux, windows
teaches: 19-12-what-happened-before-the-stop
---

## Symptom

The log verifies, and an event that mattered is not in it, or a value was changed and nobody can show that it was.

## Cause

Twenty events: a changed value was found as altered at 7, a deleted event and two swapped ones as gaps at 7. The last five cut off were not noticed, since the end of a chain has nothing after it to disagree.

## Fix

Number every event, chain them, and keep the latest chain value somewhere else.

```cpp
e.chain = chain_next(last_chain_, e.seq, time, code, value);
```
