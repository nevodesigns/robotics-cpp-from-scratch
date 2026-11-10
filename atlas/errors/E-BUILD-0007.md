id: E-BUILD-0007
title: A compatible version that was newer and broke the build
match: compatible means the same major version and at least as new, with 0.x more strict, and no pre-release unless one was asked for
match: expected this to be true: !compatible
platforms: linux, windows
teaches: 04-09-which-version-is-newer
---

## Symptom

A project asks for version 1.4 of a library, and the package finder gives it 2.0, or a release candidate, because both are newer. The build fails in code that worked, or a 0.4 library replaces a 0.3 one and the interface has changed.

## Cause

"At least as new" is not "compatible". A change of the major number is allowed to break the interface, so 2.0.0 does not satisfy a request for 1.4.0. Below 1.0.0 nothing is promised at all, so a request for 0.3 should only be met by 0.3.x. And a pre-release newer than the request is still not something a request for a release should be given.

Measured: asked for 1.4.0 among 1.2.0, 1.9.4, 2.0.1, 1.10.0, 1.11.0-rc.1 and 0.9.0, the chosen version was **1.10.0**. 1.2.0 is older, 2.0.1 and 0.9.0 have another major number, and 1.11.0-rc.1 is a candidate.

## Fix

```cpp
if (have.major != wanted.major) return false;
if (!have.pre.empty() && wanted.pre.empty()) return false;
if (wanted.major == 0 && have.minor != wanted.minor) return false;
return compare(have, wanted) >= 0;
```
