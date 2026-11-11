id: E-SAFE-0002
title: A hazard with no owner, or one half in the log
match: a hazard that is not well formed is refused for the first thing wrong with it, and is not kept
platforms: linux, windows
teaches: 19-01-what-can-go-wrong
---

## Symptom

The log has entries with a blank owner, or with a severity of 7, and a count of hazards that is not the count of hazards that were thought about.

## Cause

Six wrong entries and one good one were tried: an empty id, a repeated id, severity 5, likelihood 0, no owner, one wrong in everything (refused as no_id, the first check) and one with both numbers 0 (bad_severity). The log held 1 afterwards.

## Fix

Refuse in a fixed order and keep nothing on refusal.

```cpp
if (h.owner.empty()) return AddResult::no_owner;
hazards_.push_back(h);
```
