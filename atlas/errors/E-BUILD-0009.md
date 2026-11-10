id: E-BUILD-0009
title: Two speeds compared that came from different builds
match: speeds may be compared only between builds that are alike and neither carries a sanitizer
match: expected this to be true: comparable_for_speed
platforms: linux, windows
teaches: 04-10-what-binary-is-this
---

## Symptom

A change is said to have made the loop 15 percent slower, and it had not: the before number came from the build server's release build and the after number from a developer's sanitizer build. Or two machines with different compiler versions disagree and one is blamed.

## Cause

A speed belongs to a build. A different compiler, a different version of it, optimisation on or off, a different pointer width and above all a sanitizer each change it, and by different amounts for different code.

`comparable_for_speed` is true only for builds with the same compiler and version, both optimised or both not, the same pointer width, and no sanitizer on either. The suite checked a matching pair and then six that differ in one way each: debug, another compiler, another version, 32 bit, a sanitizer, and the same sanitizer build against itself, which is **still not** a speed to trust.

## Fix

Store the build line with the number, and compare only when `comparable_for_speed` says yes.
