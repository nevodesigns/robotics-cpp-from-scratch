id: E-BUILD-0015
title: A setting with a typo that did nothing
match: a key nobody reads is found, and the nearest known name is offered
match: expected unknown.size
platforms: linux, windows
teaches: 04-12-where-did-this-value-come-from
---

## Symptom

An operator sets `speed_limt=0.5` to slow a robot down, nothing happens, and the robot is run at full speed. The program printed no error: it was given a setting it had never heard of, and a setting nobody reads is silently dropped.

## Cause

A map of strings accepts any key. The program only looks up the keys it knows, so a misspelt one just sits there.

The suite set six keys, one right and five not: `speed_limt` (a letter missing), `wheelbase` (an underscore missing), `prot` (two letters swapped), `LOG_LEVEL` (the wrong case) and `colour_scheme` (nothing like any name). **5** unknown keys were found. The nearest known name was offered for **3** of them. For `LOG_LEVEL` and `colour_scheme` nothing was offered, because nothing is within two edits.

## Fix

After all sources are read, list the keys that are set and not known, and say what was probably meant:

```cpp
for (const std::string& key : settings.unknown_keys(known))
  std::cerr << "unknown setting " << key << ", did you mean " << suggest(key, known) << "?\n";
```

Edit distance counts the single-character changes between two words. A swap of two letters is two of them.
