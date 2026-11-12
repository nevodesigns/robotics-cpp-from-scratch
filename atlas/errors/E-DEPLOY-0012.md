id: E-DEPLOY-0012
title: A file that was never in this release
match: the files on the robot are checked against the manifest, and each way they can differ is named
match: every one of a thousand single bit flips in a 4096 byte file is caught
platforms: linux, windows
teaches: 20-04-what-is-in-the-box
---

## Symptom

The robot behaves like the old release in one place: a leftover file is loaded by name, or one byte changed in transit.

## Cause

Against the manifest, a byte changed in place was hash, a file a byte short was size, a file that never arrived was missing, and one left by release 1.3 was extra. All 1000 single bit flips in 4096 bytes were caught.

## Fix

Check size and hash for every entry, and list what is extra.

```cpp
const auto d = verify_files(manifest, files);
```
