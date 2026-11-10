id: E-BUILD-0008
title: A speed measured on a debug build, reported as the robot's
match: a build that is not fit to be timed says why, and a release build says nothing
match: expected none.size
platforms: linux, windows
teaches: 04-10-what-binary-is-this
---

## Symptom

A control loop is timed on a laptop and reported as taking 300 microseconds. On the robot, built differently, it takes 40. Or the other way round: a number that was good in a release build is quoted for a debug one and a requirement is thought to be met.

## Cause

A build that has not defined `NDEBUG` has its assertions on and, usually, its optimiser off. The same source can be several times slower, by an amount that depends on the code. Nothing in the program says which kind of build it is.

The suite builds the line for this very binary: `gcc-11 c++17 debug none 64bit little`, with the warning **"not optimised: assertions are on and the compiler has not tried to be fast"**. A release build gives no warnings, a debug one gives 1, and a debug build with a sanitizer gives 2.

## Fix

Print the build with every measurement, and have the measuring code say when the build is not fit:

```cpp
for (const std::string& w : speed_warnings(current_build())) std::cout << "warning: " << w << "\n";
```
