id: E-FIELD-0034
title: An interface with no contract
match: the program works on the simulator and not on the board
match: two implementations of one interface disagree about what stop does
platforms: linux, windows
teaches: 22-11-the-contract-test-for-an-interface
---

## Symptom

A program that works on the simulator misbehaves on the board in a way nobody can explain.

## Cause

The interface said what can be called and nothing about what happens. Six drives that each break one promise were all caught by one contract.

## Fix

Write the contract as a test that takes any implementation, and run it on every one.

```cpp
const auto broken = check_drive_contract(make_my_drive, expectations);
```
