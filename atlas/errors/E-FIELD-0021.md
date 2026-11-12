id: E-FIELD-0021
title: A scenario that lives in the source
match: the test cannot be shared with the colleague who has a different simulator
match: the scenario is a function and not a file
platforms: linux, windows
teaches: 22-07-a-field-scenario-file
---

## Symptom

Nobody can say exactly what was run last month, or run it in the other simulator.

## Cause

The ground, the walls and the faults were written as code. A file of 15 lines holds all of them.

## Fix

Keep the scenario as a file, build both simulators' worlds from it, and keep the file with the results.

```cpp
const ScenarioParse p = parse_scenario(text);
```
