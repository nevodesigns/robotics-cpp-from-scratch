# Platforms

The curriculum promises specific toolchains, and continuous integration proves
the promise on every change. This file records what each promise means and how
the differences are handled.

## The matrix

| Identifier | Operating system | Compiler | CMake | Qt from apt | ROS 2 |
|---|---|---|---|---|---|
| `ubuntu-22.04-gcc11` | Ubuntu 22.04 LTS | GCC 11 | 3.22 | 6.2 | Humble |
| `ubuntu-24.04-gcc13` | Ubuntu 24.04 LTS | GCC 13 | 3.28 | 6.4 | Jazzy |
| `windows-msvc2022` | Windows 11 | MSVC 2022 | bundled | installer | none |
| `ubuntu-22.04-clang14` | Ubuntu 22.04 LTS | Clang 14 | 3.22 | 6.2 | Humble |
| `ubuntu-24.04-clang18` | Ubuntu 24.04 LTS | Clang 18 | 3.28 | 6.4 | Jazzy |
| `macos-clang` | macOS | AppleClang | brew | brew | none |

A lesson lists the identifiers it supports in `lesson.json`, and CI builds
exactly that set. A lesson claiming a toolchain it does not build on fails the
change. There are no aspirational support badges here.

## The language baseline is C++17

Every supported toolchain implements C++17 completely, including the GCC 11 that
ships with Ubuntu 22.04. That means both supported Ubuntu releases compile
identical code with no version gates, which removes an entire class of problem
that a C++20 baseline would have created.

The facilities that arrived later are provided by `rc/core/compat.hpp`:

| You write | Standardised as | Available from |
|---|---|---|
| `rc::span` | `std::span` | GCC 10, MSVC 2019 16.6, in C++20 mode |
| `rc::format` | `std::format` | GCC 13, MSVC 2019 16.10 |
| `rc::expected` | `std::expected` | GCC 12, MSVC 2022 17.3, in C++23 mode |

On a toolchain that has the standard version, `rc::` is an alias for it. On one
that does not, it is the implementation in that header, which the learner is
expected to read. Rule L017 stops a lesson using the standard facility directly,
because doing so would raise the compiler requirement and skip the part that
teaches.

Moving the whole curriculum to C++20 later is a small, deliberate change: raise
the standard in `CMakeLists.txt`, and the aliases begin resolving to the
standard types with no lesson edits at all.

## Qt

The baseline is the Qt 6.2 API, because that is what `apt install qt6-base-dev`
gives on Ubuntu 22.04. A beginner never meets the online installer or an account
wall. CI additionally builds every Qt lesson against a pinned Qt 6.8 LTS, so the
upgrade path is proven rather than assumed.

Anything newer than 6.2 appears only behind a `QT_VERSION_CHECK` guard, and the
guard is explained where it is used.

Qt is licensed under the LGPL for open source use. What that means for a binary
you distribute, in particular dynamic linking and the right to relink, is a
lesson in phase 12 rather than a footnote.

## Windows

Phases 00 through 16 and phase 19 support Windows fully with MSVC 2022.

ROS 2 is not supported on Windows by this curriculum. It can be built there, but
the experience is materially worse and the documentation assumes Linux. Windows
learners who reach phase 17 use WSL2, which is a first class supported path:

- Qt applications display through WSLg with no X server setup.
- USB devices attach with `usbipd-win`, so real hardware still works in phase 08.
- The Ubuntu inside WSL2 is a supported toolchain, so everything else is
  unchanged.

## ROS 2 and the migration that is coming

ROS 2 releases pair with exactly one Ubuntu LTS. Humble ends in May 2027 and
Jazzy in May 2029, and Lyrical Luth arrived in May 2026 for Ubuntu 26.04.

Because that clock is a certainty rather than a risk, phase 17 sits behind a thin
`rc::ros` adapter rather than scattering `rclcpp` calls through many lessons. A
distro migration is then one adapter and a container tag, not a rewrite of
twenty lessons.

## Adding a toolchain

1. Add the identifier to `known_platforms()` in `tools/rcpp/cmd_audit.cpp`.
2. Teach `rc_detect_platform` in `cmake/RcPlatform.cmake` to recognise it.
3. Add a configure preset in `CMakePresets.json`.
4. Add the lane to `.github/workflows/ci.yml`.
5. Only then add the identifier to any lesson, once it actually builds.
