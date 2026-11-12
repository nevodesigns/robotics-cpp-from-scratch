id: E-DEPLOY-0006
title: A library found on the host and linked into the robot
match: with libraries looked for in both places, a missing one is replaced by the host's of the wrong architecture, and ONLY says it is missing
platforms: linux, windows
teaches: 20-02-a-toolchain-file-that-cannot-lie
---

## Symptom

The link fails with a message about an incompatible file, or the build passes and the robot cannot load the library.

## Cause

With libusb only on the host, ONLY said missing and BOTH returned the x86_64 library. NEVER returned the host's libz where BOTH returned the aarch64 one.

## Fix

Find libraries in the sysroot only.

```cpp
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
```
