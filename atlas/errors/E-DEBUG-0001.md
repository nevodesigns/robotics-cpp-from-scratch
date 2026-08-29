id: E-DEBUG-0001
title: A backtrace with no file names or line numbers
match: \?\? \(\) from
match: No symbol table info available
match: in \?\?
platforms: linux, windows
teaches: 04-02-reading-a-crash
---

## Symptom

A crash produces frames made of addresses and question marks, with no function
names, files or line numbers. A sanitizer report shows raw addresses instead of
source locations.

## Cause

The binary carries no debug information. Either it was built without it, or it
was built in a release configuration that discards it, or the frame pointers
were optimised away so the stack could not be walked.

## Fix

Build with debug information, which every preset in this repository already
does:

```
cmake --preset default
```

For a sanitizer report, add frame pointers so the stack can be unwound reliably:

```
-g -fno-omit-frame-pointer
```

If a release binary crashed in the field and cannot be rebuilt, keep the symbols
that were produced alongside the release and symbolise the addresses afterwards.
Discarding them at build time is what makes a field crash unreadable, and it is
worth deciding deliberately rather than by default.
