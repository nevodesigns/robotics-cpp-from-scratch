id: E-FIELD-0034
title: An interface with no contract
match: the program works on the simulator and not on the board
match: two implementations of one interface disagree about what stop does
match: the fake drive of lesson 22-01 keeps every promise
match: each broken drive is caught, by the promise it breaks
match: a change in what is expected changes the verdict: a drive that is slower than asked for is caught
platforms: linux, windows
teaches: 22-11-the-contract-test-for-an-interface
---

## Symptom

A program that works on the simulator misbehaves on the board in a way nobody can explain. The simulator's drive and the board's drive were written by different people.

## Cause

The interface had no contract. It said what can be called and nothing about what happens: what a stale command does, what a NaN does, how fast speed may change.

An interface lists functions. What they do in the odd cases, a stale command, a NaN, a step in speed, is not in the list. Two implementations can both match the interface and differ in exactly those cases.

## Fix

Write the contract as a test that takes any implementation, and run it on every one. Six drives that each broke one promise were all caught by one contract.

```cpp
const std::vector<std::string> broken = rc::hal::check_drive_contract(make_my_drive, expectations);
```
