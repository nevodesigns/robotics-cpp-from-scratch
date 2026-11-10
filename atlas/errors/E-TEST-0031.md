id: E-TEST-0031
title: A fault that only one check can see
match: three of the six faults depend on a single check, and taking that check away hides them
match: expected unseen_without
platforms: linux, windows
teaches: 05-12-six-faults-and-a-matrix
---

## Symptom

A test is judged noisy and loosened, or slow and deleted, or rewritten for a new design. Months later a fault that it alone was catching reaches a robot.

## Cause

The suite looked well covered, three checks for six faults, and several of the faults were seen by one check only. A check that is the only witness for a fault is carrying more than its share, and nothing says so until the table is drawn.

Measured: the flipped sign and the weak actuator were seen only by the response check, the removed limiter only by the limit check: **3 of 6** had a single witness. Taking away the response check hid **2** faults, the limit check **1**, and the model check **0**, which duplicates the response check on every fault it sees and says why: the sensor disagrees with the plant.

## Fix

Read the table before changing a test, and add a second witness for each fault with one:

```cpp
const std::vector<std::size_t> alone = faults_with_one_witness(caught);
```
