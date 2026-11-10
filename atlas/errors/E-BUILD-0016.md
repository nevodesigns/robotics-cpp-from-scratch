id: E-BUILD-0016
title: A value with no record of where it came from
match: a value can say where it came from
match: expected a to equal
platforms: linux, windows
teaches: 04-12-where-did-this-value-come-from
---

## Symptom

A robot behaves with a log level nobody set, in a place nobody can find: the config file says one thing, the launch script another, and a variable exported months ago in somebody's shell a third. Everyone has a different guess about which is in force.

## Cause

Once the sources are merged into one value, the question "where did that come from" cannot be answered, unless the value remembers.

Each setting here keeps its source, and `explain` writes it out: `log_level = debug (environment)`, `timeout = 30 (default)` and `nothing is not set`. A log line at start-up that prints these for every setting turns a long argument into a glance.

## Fix

Store the source with the value and print it:

```cpp
return key + " = " + value + " (" + source_name(source) + ")";
```
