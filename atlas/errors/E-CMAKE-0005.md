id: E-CMAKE-0005
title: A package find_package refuses because it has no version
match: the consumer could not find the package
match: compatible with requested version
match: version: unknown
platforms: linux, windows
teaches: 04-03-your-own-package
---

## Symptom

```text
CMake Error at CMakeLists.txt:3 (find_package):
  Could not find a configuration file for package "steplib" that is
  compatible with requested version "1.0".

  The following configuration files were considered but not accepted:

    /opt/steplib/lib/cmake/steplib/steplibConfig.cmake, version: unknown
```

The package is installed. CMake found the file, in the right place, and refused
it anyway. The line that gives it away is the last one: **version: unknown**.

Dropping the version from the request makes it work, which is the fix everybody
applies and which leaves the package unusable by anyone who does ask.

## Cause

The package has a config file and no **version** file.

`find_package(steplib 1.0)` looks for `steplibConfig.cmake` and then for
`steplibConfigVersion.cmake` beside it. Without the second, CMake has no way to
decide whether what it found satisfies the request, so it does not guess: it
reports the version as unknown and moves on.

The version in `project(steplib VERSION 1.0.0)` is not enough by itself. It is
what the version file is generated *from*, and generating it is a separate step.

## Fix

```cmake
include(CMakePackageConfigHelpers)

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/steplibConfigVersion.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/steplibConfig.cmake
              ${CMAKE_CURRENT_BINARY_DIR}/steplibConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/steplib)
```

`COMPATIBILITY` is a real decision rather than boilerplate. `SameMajorVersion`
says 1.4 satisfies a request for 1.0 and 2.0 does not, which is what most
libraries mean. `ExactVersion` says only the same version will do, which is what
you want when the package is generated code that must match its generator.

Two neighbouring mistakes that produce a similar silence.

The config file must be named **exactly** `<name>Config.cmake` or
`<lowercase name>-config.cmake`. `find_package` looks for those names and
nothing else, and a file called `steplib-Config.cmake` is invisible.

And it must be installed somewhere `find_package` searches, which is why
`${CMAKE_INSTALL_LIBDIR}/cmake/<name>` is the conventional destination rather
than an arbitrary directory.
