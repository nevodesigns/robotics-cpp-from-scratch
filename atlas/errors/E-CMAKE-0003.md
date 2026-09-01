id: E-CMAKE-0003
title: An exported target with a path into the source directory
match: INTERFACE_INCLUDE_DIRECTORIES property contains path
match: which is prefixed in the source directory
platforms: linux, windows
teaches: 04-03-your-own-package
---

## Symptom

Configuring a project that installs a library stops with:

```text
CMake Error in CMakeLists.txt:
  Target "steplib" INTERFACE_INCLUDE_DIRECTORIES property contains path:

    "/home/you/steplib/include"

  which is prefixed in the source directory.
```

The path is correct. The library builds. It is the **install** that CMake is
refusing to let you set up.

## Cause

An include directory was given as a plain path:

```cpp
target_include_directories(steplib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
```

That is right for anything building inside this project and wrong for anything
installing it, because it would be written into the exported package as an
absolute path on **the machine that built it**. A consumer on another machine
would be pointed at a directory that does not exist, or worse, at one that does
and contains something else.

CMake refuses at configure time rather than letting you ship it, which is
generous. There is a version of this it cannot catch: see `E-CMAKE-0004`.

## Fix

Describe both places, because there are genuinely two:

```cmake
target_include_directories(steplib INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
```

`BUILD_INTERFACE` applies while this project is being built, and is dropped from
the exported package. `INSTALL_INTERFACE` applies only in the exported package,
and is relative to wherever the package was installed, which is why it does not
name an absolute path.

Two habits that go with it.

**`include(GNUInstallDirs)`** and use `CMAKE_INSTALL_INCLUDEDIR` rather than
writing `include` yourself. It costs one line and it is what makes the package
land correctly on a system that uses `lib64`, or under a prefix somebody chose.

**Anything that is a path into your source tree is a build time thing.** The
same split applies to a generated header, which lives in the build directory
during the build and in the include directory afterwards.
