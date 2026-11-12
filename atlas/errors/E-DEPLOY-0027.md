id: E-DEPLOY-0027
title: A budget that the flood pushes the history out of
match: whatever the sizes of the records, the log never goes over its budget, and one record bigger than a file is kept until the next arrives
match: a policy is usable when a file can hold something and the total allows a whole file, and its budget is the smaller of two limits
platforms: linux, windows
teaches: 20-09-a-log-that-cannot-fill-the-disk
---

## Symptom

The disk is safe, and the log has nothing from before the fault, which is the part that explains it.

## Cause

100000 records of 1 to 900 bytes against a budget of 5000 bytes never broke it, holding at most 5000. A record of 9000 bytes was kept until the next one. The budget of 20 files of 10 MB was 200 MB, and the policy with 5 files was 50 MB.

## Fix

Use the guard and the budget together.

```cpp
budget(policy)
```
