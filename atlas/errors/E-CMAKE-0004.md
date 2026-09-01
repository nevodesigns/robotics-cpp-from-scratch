id: E-CMAKE-0004
title: An installed package with no include directory
match: the consumer found the package and could not build
match: steplib/step.hpp: No such file or directory
platforms: linux, windows
teaches: 04-03-your-own-package
---

## Symptom

The package installs without a word. `find_package` finds it. The target exists
and links. And the consumer's compiler cannot find the header:

```text
main.cpp:1:10: fatal error: steplib/step.hpp: No such file or directory
```

The header **is** installed. It is sitting in the install prefix, correctly
placed, and nothing is telling the compiler to look there.

Worse: on the machine that built the package this often works, because the
consumer picks the headers up from the source tree it happens to share with.
That is why this ships.

## Cause

The build tree was described and the install tree was not:

```cmake
target_include_directories(steplib INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
```

`BUILD_INTERFACE` is dropped when the target is exported, on purpose, because
that path is meaningless outside this build. With nothing to replace it, the
exported target carries **no include directory at all**, which is a perfectly
valid thing for a target to say and is not what was meant.

`E-CMAKE-0003` is the same mistake caught at configure time. This is the version
that gets past it, and it is quieter for exactly that reason: half the fix looks
like the whole fix.

## Fix

```cmake
target_include_directories(steplib INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
```

Then test it in a way that cannot pass by accident. Install the package to a
temporary prefix, **delete the source tree**, and build a consumer against what
is left:

```
cmake -S source -B build -DCMAKE_INSTALL_PREFIX=$tmp/install
cmake --build build --target install
rm -rf source
cmake -S consumer -B build-consumer -DCMAKE_PREFIX_PATH=$tmp/install
cmake --build build-consumer
```

Deleting the source is the part that matters. A package that only works while
its source tree is still present passes every test run on the machine that made
it, and fails for the first person who installs it anywhere else.

That is what lesson 04-03 does, and it is worth doing to any package before
handing it to anyone.
