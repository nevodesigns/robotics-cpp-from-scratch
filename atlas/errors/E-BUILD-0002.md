id: E-BUILD-0002
title: The build says there is no work to do
match: ninja: no work to do
match: make: Nothing to be done
platforms: linux, windows
teaches: 04-01-what-a-build-system-does
---

## Symptom

You changed something, ran the build, and it reported that everything was
already up to date.

## Cause

This is almost never a fault. It is the build system disagreeing with you about
what changed, and it has two ordinary explanations.

The edit was to a file the graph does not track. A file that is not an input to
any target cannot make anything stale.

Or the change altered the shape of the graph rather than the contents of a file
in it: a new source file, a macro that requires a code generator, a library that
has since been installed. The graph is worked out at configure time, so none of
those take effect until the project is configured again.

## Fix

For a change to the graph, reconfigure:

```
cmake -S . -B build/default
```

For everything else, check that the file you edited is genuinely an input of the
target you expected to rebuild. Listing the targets is a quick way to see what
the build system currently believes:

```
./build/default/bin/rcpp targets --references
```

If a build ever behaves impossibly, delete the build directory and configure
from scratch. That takes seconds here and removes stale state as a suspect.
