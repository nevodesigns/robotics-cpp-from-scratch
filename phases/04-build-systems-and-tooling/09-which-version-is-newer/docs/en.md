# Which Version Is Newer: Why 1.9 Is Older Than 1.10, Where a Release Candidate Sits, and What Compatible Means

> Lesson 04-03 made a package with a version, and a consumer that asked for one. This is the other half:
> deciding which of several versions is newer, and which of them will do.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 01-05, 04-03

## The Problem

A robot project uses a dozen libraries and each has a version. A new one is installed, and something must decide whether it is newer than the old one, and whether it is a fit for what the project asked for. Those decisions are made by tools, and a tool that makes
them wrongly picks an old library, a release candidate, or a version with a changed interface, without saying so.

## The Concept

### A version is three numbers

`1.10.2` is major 1, minor 10, patch 2. A new major number may break the interface, a new minor adds things without breaking, and a patch fixes. `parse_version` reads them and refuses malformed text, leaving its output alone. It refused all 15 of: empty text, `1`, `1.2`, `1.2.3.4`, `1.x.0`, `1..3`,
`01.2.3` (a leading zero), `1.02.3`, `-1.0.0`, `1.0.0-` (an empty label), `1.2.3-`, `a.b.c`, a version with a space at either end and a number too long to hold. An optional `+build` part is accepted and ignored.

### Numbers compare as numbers

`"1.9.0" < "1.10.0"` as text is **false**: the character `'1'` is less than `'9'`, and ten starts with a one. As versions, `compare` gives **-1**, so 1.9.0 is older. A script that picks the newest by text picks 1.9 over 1.10. See `E-BUILD-0005`.

### A release candidate comes first

The part after the dash is a pre-release label, and a version with one is **older** than the same numbers without it: `1.0.0-rc.1` comes before `1.0.0`. Between two labels, the dot separated identifiers compare one at a time. Two numbers compare as numbers, a number is lower than a word, and two words
compare as text; if everything is equal so far the label with fewer identifiers is older.

`"1.0.0-beta.11" < "1.0.0-beta.2"` as text is **true**, which is wrong, and `compare` gave 1. Eight versions from the specification's own example, `alpha`, `alpha.1`, `alpha.beta`, `beta`, `beta.2`, `beta.11`, `rc.1` and the release, shuffled and sorted with `compare`, came out in that order. A number is compared as digits, longer is bigger,
so `rc.99999999999999999999` and `rc.100000000000000000000` cannot overflow. See `E-BUILD-0006`.

### Compatible is not newer

`compatible(wanted, have)` is the rule of CMake's `SameMajorVersion`:

- the major numbers must be equal, since a new major may break the interface: 2.0.0 does not satisfy a request for 1.4.0 though it is newer;
- below 1.0.0 nothing is promised, so the minor numbers must match as well: 0.4.0 does not satisfy 0.3.1;
- we must be at least as new;
- a pre-release is not offered to a request for a release.

Asked for 1.4.0 among 1.2.0, 1.9.4, 2.0.1, 1.10.0, 1.11.0-rc.1 and 0.9.0, the version chosen was **1.10.0**. The first is older, the third and sixth have another major number, and the fifth is a candidate. See `E-BUILD-0007`.

## Build It

`parse_version`, `compare` and `compatible` in `exercise/solution.hpp`. The `Version` type and the helpers in `version_detail` are given.

```
rcpp verify 04-09
```

## Use It

`rc/core/version.hpp` joins `rc::core`.

```cpp
Version wanted, have;
parse_version("1.4.0", &wanted);
parse_version(installed_text, &have);
if (!compatible(wanted, have)) { /* say which, and why */ }
```

**What this does not do.** It handles `major.minor.patch` and nothing else: not `1.2`, not four numbers, not a date, which many libraries use. It is the rule of one package finder and not of all: some allow any newer version, some require an exact match, and ROS and Python have their own. It ignores the `+build` part, which the
semantic version rules also ignore for ordering. And a version number is a promise by a person, so a library that breaks its interface in a patch release is not caught by any of this.

## What Breaks First

- **Versions compared as text.** 1.9 beats 1.10. See `E-BUILD-0005`.
- **A pre-release sorted as a release.** A candidate is chosen. See `E-BUILD-0006`.
- **Newer taken for compatible.** A new major or a candidate gets in. See `E-BUILD-0007`.

## Ship It

`rc/core/version.hpp` holds `Version`, `parse_version`, `compare` and `compatible`.

Lesson 04-03 gave the package a version. This is how a consumer decides whether it is the one that was asked for.
