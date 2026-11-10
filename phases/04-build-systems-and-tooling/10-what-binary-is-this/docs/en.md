# What Binary Is This: Which Compiler, How Optimised, What Sanitizer, and Which Numbers May Be Compared

> A time, a crash or a number in a log is about one build of the program. The build is rarely written
> beside it, and the same code in another build can be several times slower.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 01-05, 04-06, 04-07

## The Problem

A loop is timed and reported as 300 microseconds. Somebody else times it and gets 40. A change is said to have made it 15 percent slower, and the two timings came from different builds. A crash report arrives from a robot and nobody can say which compiler made the binary.

Lesson 04-06 found that a check disappears from a release build, and lesson 04-07 that a sanitizer changes what a program measures. What is missing is the program being able to say, in one line, what it is.

## The Concept

### What changes the answer

Six facts about a build change what a program does or how fast it does it: the compiler, its version, the C++ standard, whether it is optimised (`NDEBUG` set, which a release build does), a sanitizer, and the pointer width. Byte order completes the line, and none of the machines this curriculum claims is big endian.

`current_build()` gathers them from the compiler's own macros: `__clang__` is checked first because clang also defines `__GNUC__`, and `_MSVC_LANG` is read for the standard because MSVC's `__cplusplus` does not move. The byte order is found by looking at the bytes of the number one, not by asking a macro.

### One line

```
gcc-11 c++17 debug none 64bit little
```

The suite built that line for its own binary, and for the sanitizer build it read `gcc-11 c++17 debug address 64bit little`, with the address sanitizer found through the macro `__SANITIZE_ADDRESS__`. Built with clang, it read `clang-14 c++17 debug none 64bit little`.

Five builds written and read back came out unchanged. See `E-BUILD-0010` for what the reader does with a line that is not the format: **12** malformed lines were refused, and each left the output alone.

### When two speeds can be compared

A speed belongs to a build. `comparable_for_speed` is true only if both builds have the same compiler and version, are both optimised or both not, have the same pointer width, and neither has a sanitizer. A sanitizer slows a program by an amount that depends on what it does, so even the same sanitizer build compared with itself is not a speed to trust. See `E-BUILD-0009`.

### Saying so

`speed_warnings` gives one string for each reason not to trust a speed:

| build | warnings |
|---|---|
| release | 0 |
| debug | 1: not optimised |
| debug with a sanitizer | 2 |

The binary of this lesson, built with the default settings, printed `not optimised: assertions are on and the compiler has not tried to be fast`. See `E-BUILD-0008`.

## Build It

`describe`, `parse_build`, `comparable_for_speed` and `speed_warnings` in `exercise/solution.hpp`. The `BuildInfo` type and `current_build` are given.

```
rcpp verify 04-10
```

## Use It

`rc/core/build_info.hpp` joins `rc::core`.

```cpp
std::cout << "build: " << describe(current_build()) << "\n";
for (const std::string& w : speed_warnings(current_build())) std::cout << "warning: " << w << "\n";
```

**What this does not do.** It records what the compiler's macros say, not the flags that were given: two builds with the same line can have different optimisation levels (`-O1` and `-O3` are both "release") or different warning flags. It says nothing of the standard library in use, or of the machine the program runs on, whose
processor and load change a speed as much. And a line printed by a program can only be trusted if that program is the one being measured: copy a log from one robot to another and it describes the wrong binary.

## What Breaks First

- **A speed from a build that was not fit to be timed.** See `E-BUILD-0008`.
- **Two speeds compared across builds.** See `E-BUILD-0009`.
- **A build line parsed without checking.** See `E-BUILD-0010`.

## Ship It

`rc/core/build_info.hpp` holds `BuildInfo`, `current_build`, `describe`, `parse_build`, `comparable_for_speed` and `speed_warnings`.

Lesson 04-05 learnt where the time goes. This is how to say what the time is the time of.
