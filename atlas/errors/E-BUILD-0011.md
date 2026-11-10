id: E-BUILD-0011
title: A build that trusts the timestamp of a restored file
match: a file restored with an old timestamp is stale by time and caught by content
match: expected this to be true: !by_time
platforms: linux, windows
teaches: 04-11-the-build-that-rebuilt-nothing-or-everything
---

## Symptom

A source file is restored from a backup, or a branch is switched, and the build says there is nothing to do. The program runs the old behaviour. Cleaning and rebuilding fixes it, and nobody trusts the incremental build any more.

## Cause

The rule "rebuild when the source is newer than the output" assumes that a changed file always has a new timestamp. A file brought back from a backup, an archive or another machine keeps the time it had there, which can be older than the object built from the file it replaced.

Measured: a file whose text changed and whose timestamp (400) was older than the object (1000): `needs_rebuild_by_time` said **no**, so the old object would have been used. The content key found **no** stored object for the new text.

## Fix

Decide by what is in the file, not when it was written:

```cpp
const Id key = build_key(name, source_text, flags);
if (!cache.lookup(key, &out)) { out = compile(...); cache.store(key, out); }
```
