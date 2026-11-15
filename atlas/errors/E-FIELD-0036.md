id: E-FIELD-0036
title: A number with no source
match: nobody knows whether the track width was measured or guessed
match: a model number with no source
match: the reference robot parses, every number has its source, and the typical ones are counted
match: the kind of the robot is read from its line, and not the word before it
platforms: linux, windows
teaches: 22-12-describing-a-robot-once
---

## Symptom

The simulated robot behaves differently from the real one, and nobody can say which of its numbers to doubt. The model's numbers came from many places and nobody wrote down which.

## Cause

The numbers had no record of where they came from.

A typical value is a guess about a robot that is not this one. Without a source beside each number, nobody can tell the guesses from the measurements when the simulation and the robot disagree.

## Fix

Refuse a number without a source or the word typical, and replace typical values with measured ones as they become known. In the reference file 3 of 11 are typical, and it says so beside each.

```cpp
track_width 0.160 source <where it came from>
```

The line above is from the robot file of 22-12; parse_robot refuses it without a source.
