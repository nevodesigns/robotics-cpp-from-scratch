id: E-BUILD-0019
title: A conflict reported without saying who disagreed
match: when no version suits everyone, the answer names the package and who disagreed
match: expected r.problem
platforms: linux, windows
teaches: 04-13-two-things-need-different-versions
---

## Symptom

The build says "dependency conflict" and nothing else. The person has four hundred packages and no idea which two are fighting over which one.

## Cause

The resolver knew everything: which package, and who asked for which range. It threw it away, because "failed" was enough to stop. A conflict is only useful to a person if it names the package and each side.

The message the suite saw: `C: no version satisfies everyone, since A 1.0 needs [1.0.0, 2.0.0), B 1.0 needs [2.0.0, 3.0.0)`. A package that is not in the repository at all was reported differently: `Z: no version is available at all`. Two packages that need each other ended in 3 steps and did not go round for ever.

## Fix

Keep who asked with every requirement, and write them all out when none fits:

```cpp
text += wants[i].from + " needs " + show(wants[i].range);
```
