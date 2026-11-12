id: E-DEPLOY-0010
title: A manifest that cannot tell it was cut short or edited
match: a truncated manifest, an edited one and a malformed one are each refused with their reason
match: a manifest is the same whatever order the files come in, and reads back as it was made
match: the hash is FNV-1a, which has published values
platforms: linux, windows
teaches: 20-04-what-is-in-the-box
---

## Symptom

A robot runs a release with half its files, because the download stopped and the reader skipped what it did not understand.

## Cause

A manifest cut before its end line was refused as no end line, one digit of a size changed was bad checksum, and ten more malformed texts each had their own reason. The same release in two orders gave the same text.

## Fix

End the manifest with a hash of everything above it, and refuse anything else.

```cpp
text += end + hex16(hash64(text));
```
