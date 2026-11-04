id: E-DEBUG-0010
title: The sanitizer that was never switched on
match: a lane that looks instrumented and is not
platforms: linux, windows
teaches: 04-07-the-tool-that-watches-it-run
---

## Symptom

A build configured with the sanitizers finds nothing, ever. The tests pass,
the lane is green, and it stays green through changes that should have set it
off.

Or, more quietly, a measurement that ought to differ under instrumentation
comes out identical, and you conclude the sanitizer does not affect it.

## Cause

The option was not the option.

```bash
cmake -S . -B build/asan -DCMAKE_BUILD_TYPE=Debug -DRC_SANITIZE=address,undefined
```

This repository's option is `RC_SANITIZE_ADDRESS`. The line above sets a
variable nothing reads, CMake accepts it without complaint, and the result is
an ordinary Debug build in a directory called `asan`.

CMake does warn about unused command line variables, but only as a note at the
end of a long configure, and it is easy to miss. Nothing downstream fails,
because an unknown `-D` is a perfectly legal way to pass a variable a project
might read later.

This happened in this repository, to the author of lesson 04-07, for a whole
working session. Every commit message in that session claimed three lanes
including a sanitizer lane, and the third lane was a second plain Debug build.
The continuous integration job used the correct option throughout, so the
coverage was real; the local claim was not.

The tell, in hindsight, was a measurement that refused to move. Lesson 02-09
measures bytes coming back from a freed heap block, and the supposed sanitizer
lane reported the same 16 of 32 as the plain build. Under genuine
instrumentation it reports 0 of 32, because the freed block goes into a
quarantine.

## Fix

Ask the build what it is doing, rather than the command line what you asked
for.

```bash
ninja -C build/asan -t commands <some-target> | grep -o '\-fsanitize=[a-z,]*' | sort -u
```

Empty output means an uninstrumented build whatever the directory is called.
For the Makefile generator the same information is in
`CMakeFiles/<target>.dir/flags.make`.

Better, ask the program itself, which is what `rc/test/sanitizer.hpp` is for:

```cpp
#include <rc/test/sanitizer.hpp>
std::cout << "built with: " << rc::test::sanitizer_name() << "\n";
```

A suite that prints what it was built with cannot quietly become a suite that
checks nothing. That one line would have caught this in the first minute.

Two related habits.

**Make the lane prove itself.** A sanitizer lane with no deliberately failing
case anywhere in the project cannot distinguish "nothing is wrong" from "I am
not looking". A single test that is expected to trip the instrumentation, run
in that lane only, turns the lane into something that reports its own health.

**Prefer a preset to a remembered flag.** `cmake --preset asan` cannot be
misspelled into silence, because a preset that does not exist is an error.
