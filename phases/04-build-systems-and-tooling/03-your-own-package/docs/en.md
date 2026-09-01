# Your Own Package: Code Somebody Else Can Use

> Everything you have written so far lives inside this repository's build. This
> is the lesson that gets it out.

**Type:** Build
**Time:** about 150 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 04-01, 01-02

## The Problem

You have a header worth keeping. Another project wants it.

The obvious answer is to copy the file, and it works, once. Then the header
changes and there are two of it. Then a third project copies the second one.
Then somebody needs to know which C++ standard it requires, and the only place
that is written down is a CMakeLists in a project they have never seen.

A **package** is the alternative: install it once, and let other projects ask
for it by name.

```cmake
find_package(steplib 1.0 REQUIRED)
target_link_libraries(consumer PRIVATE steplib::steplib)
```

Two lines, and the consumer now has the headers, the include path and the
language standard, without knowing where any of it is.

Getting there is about twenty lines, and there are three specific ways to get it
wrong that all produce a package which works perfectly **on the machine that
built it**.

## The Concept

### A target carries its own requirements

The idea underneath all of this is that a target knows what using it requires,
and hands that to whoever links it.

```cmake
target_compile_features(steplib INTERFACE cxx_std_17)
```

`INTERFACE` means "whoever links this needs it". So a consumer that links
`steplib::steplib` gets C++17 without asking, and nothing in the consumer's
CMakeLists mentions a standard.

That is the test of whether a package is finished: **the consumer's CMakeLists
should be boring**. If it has to state an include directory or a standard, the
package did not carry them and every consumer will have to repeat it.

A header only library is an `INTERFACE` library: it compiles nothing itself and
exists entirely to carry requirements. `librc` in this repository is one, and it
is worth reading now that the shape means something.

### The include directory is in two places, and that is not a technicality

While your project is building, the headers are in your source tree. After
installation they are in the install prefix. Those are different paths and the
exported package must not confuse them.

```cmake
target_include_directories(steplib INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
```

Three ways to write that, and only one is right:

| what you write | what happens |
|---|---|
| a plain path | CMake **refuses at configure time**: it would export an absolute path into your source tree |
| `BUILD_INTERFACE` only | installs cleanly, and the exported target has **no include directory at all** |
| both | correct |

The middle row is the dangerous one, and it is dangerous precisely because half
the fix looks like the whole fix. The package installs, `find_package` succeeds,
the target links, and the consumer's compiler cannot find the header. On the
machine that built it, it often works anyway, because the consumer picks the
headers up from the source tree they happen to share.

### A config file, and a version file, which are not the same thing

`find_package(steplib)` looks for a file named exactly `steplibConfig.cmake`.
For a header only library its whole job is to include the targets file that
`install(EXPORT ...)` generated.

`find_package(steplib 1.0)` also looks for `steplibConfigVersion.cmake` beside
it, and **refuses the package if it is not there**:

```text
The following configuration files were considered but not accepted:
  /opt/steplib/lib/cmake/steplib/steplibConfig.cmake, version: unknown
```

The version in `project(steplib VERSION 1.0.0)` is not enough on its own. It is
what the version file is generated from, and generating it is a separate line.

The fix everybody reaches for is to drop the version from the request, which
works and leaves the package unusable by anyone who does ask for one.

### The namespace is an error message

```cmake
install(EXPORT steplibTargets NAMESPACE steplib:: ...)
```

That is what makes `steplib::steplib` exist on the consumer's side, and the
double colon is doing real work.

A plain name that CMake cannot find is assumed to be a library to look for at
link time, so a missing package becomes an obscure linker error. A name
containing `::` **must** be a CMake target, so a missing package is an error at
configure time, naming the package.

Use the namespaced name in consumers, always, for that reason alone.

### Test it by deleting the source

This is the part that turns the lesson from advice into something you can
check.

```
cmake -S source -B build -DCMAKE_INSTALL_PREFIX=$tmp/install
cmake --build build --target install
rm -rf source                     # the important line
cmake -S consumer -B build-consumer -DCMAKE_PREFIX_PATH=$tmp/install
cmake --build build-consumer
```

A package that only works while its source tree is present passes every test run
on the machine that made it, and fails for the first person who installs it
anywhere else. Deleting the source is what makes that impossible to miss, and it
costs nothing.

## Build It

The library is written. What you write is:

- `package/CMakeLists.txt`, building it and installing it as a findable package.
- `consumer/CMakeLists.txt`, finding that package and linking to it.

```
rcpp verify 04-03
```

The test does exactly what the block above does, in a temporary directory, using
the same CMake that built this repository. It reports which stage failed, and
the three ways of getting this wrong each fail at a different one: configuring
the package, configuring the consumer, or building the consumer.

## Use It

This is how anything you write leaves this repository. It is also how to read
somebody else's: a package is a config file, a targets file and a version file,
and knowing what each does turns a directory of generated CMake into something
you can debug.

For a compiled library the shape is identical, with `add_library(name STATIC)`
and sources instead of `INTERFACE`. Nothing about the install, the export or the
config file changes.

## What Breaks First

- **A plain include path.** CMake refuses to export a path into your source
  tree. See `E-CMAKE-0003`.
- **`BUILD_INTERFACE` without `INSTALL_INTERFACE`.** The package installs and
  carries no include directory. See `E-CMAKE-0004`.
- **A config file with no version file.** `find_package` reports version
  unknown and refuses it. See `E-CMAKE-0005`.

## Ship It

Nothing here graduates into `librc`, because what this lesson produces is a
second project rather than an addition to this one. What you keep is the ability
to take any of the last fifty lessons of work and hand it to somebody else.
