id: E-FIELD-0036
title: A number with no source
match: nobody knows whether the track width was measured or guessed
match: a model number with no source
platforms: linux, windows
teaches: 22-12-describing-a-robot-once
---

## Symptom

The simulated robot behaves differently from the real one and nobody can say which of its numbers to doubt.

## Cause

The numbers had no record of where they came from. In the reference file 3 of 11 are typical, and that is written beside each.

## Fix

Refuse a number without a source or the word typical, and replace typical values with measured ones as they become known.

```cpp
track_width 0.160 source <where it came from>
```
