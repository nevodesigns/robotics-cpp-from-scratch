id: E-SAFE-0003
title: Nobody can say which hazards have nothing done about them
match: what has nothing done about it is listed worst first, and a low risk with no mitigation is not an omission but a catastrophe with none is
match: the log ranks the worst first, and breaks ties by severity and then by name so that the order is always the same
match: an autonomous ground vehicle's first hazards, scored, are the ones the rest of this phase has to answer
platforms: linux, windows
teaches: 19-01-what-can-go-wrong
---

## Symptom

The project has an emergency stop and a long document, and when asked which hazards are not covered, nobody can say.

## Cause

Of six hazards, unaddressed() gave U4, U1, U5, worst first: a runaway on loss of link, a collision with a person and a very unlikely fire. The fire is there only because of the floor. Eight ground vehicle hazards scored 1 unacceptable, 3 high and 4 medium, with 8 open.

## Fix

Make open hazards a query, not a reading.

```cpp
for (const std::string& id : log.unaddressed()) report(id);
```
