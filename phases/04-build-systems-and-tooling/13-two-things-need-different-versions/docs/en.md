# Two Things Need Different Versions: Version Ranges, the Diamond, and a Resolver That Backs Up

> Lesson 04-09 said which version is newer and which will do. A project has many packages, each needing others
> within a range, and something has to choose one version of each that nobody objects to.

**Type:** Build
**Time:** about 150 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 04-01, 04-09

## The Problem

A robot project needs a motion library and a sensor library. Both need a maths library. The motion library works with any maths version from 1.0 up to but not including 3.0. The sensor library needs 1.5 or later and below 2.5. Which maths version goes in?

Neither library gets to choose alone: there is only one copy of the maths library in the program. This is the **diamond**, and it is where dependency managers earn their keep or fail.

## The Concept

### A range

A requirement is a **range**: at least the low end and below the high end, with no high end meaning anything newer. The low end is included and the high end is not, so `[1.0.0, 2.0.0)` allows 1.9.9 and not 2.0.0. `contains` answers whether a version is inside.

### What two requirements allow together

The overlap of two ranges is the larger of the low ends up to the smaller of the high ends:

| a | b | both allow |
|---|---|---|
| [1.0, 3.0) | [1.5, 2.5) | [1.5, 2.5) |
| from 1.2, no top | [1.0, 2.0) | [1.2, 2.0) |
| [1.0, 2.0) | [2.0, 3.0) | nothing: they touch and do not overlap |
| [1.0, 1.5) | [2.0, 3.0) | nothing |

`is_empty` says when a range has nothing in it. See `E-BUILD-0017`.

### The newest that suits everyone

With the motion library needing `[1.0, 3.0)` and the sensor library `[1.5, 2.5)`, and the maths library available as 1.0, 1.4, 1.8, 2.4, 2.9 and 3.1, the resolver chose **2.4.0**: the newest version in the overlap.

### Backing up

Taking the newest of everything is a guess. The suite's repository has A 2.0.0, which needs C in `[2.0.0, 3.0.0)`, and A 1.0.0, which needs C in `[1.0.0, 2.0.0)`; and B, which needs C in `[1.0.0, 2.0.0)`. The newest A cannot live with B. The resolver gave A 2.0.0 up and chose A **1.0.0**, B 1.0.0 and C **1.7.0**, in 6 steps. Reaching a dead end shows that a choice was wrong, not that there is no answer. See `E-BUILD-0018`.

### Saying who disagreed

When nothing works the message names the package and each side:

```
C: no version satisfies everyone, since A 1.0 needs [1.0.0, 2.0.0), B 1.0 needs [2.0.0, 3.0.0)
```

A package that is not in the repository at all is a different message, `Z: no version is available at all`. Two packages that need each other resolved in 3 steps, because each is chosen once. See `E-BUILD-0019`.

## Build It

`contains`, `is_empty` and `intersect` in `exercise/solution.hpp`. The resolver, the types and the messages are given: read `solve` and see where it calls what you wrote.

```
rcpp verify 04-13
```

## Use It

`rc/core/resolve.hpp` joins `rc::core`.

```cpp
std::vector<Dep> roots = {...};
Resolution r = resolve(roots, repository);
if (!r.ok) std::cerr << r.problem << "\n";
```

**What this does not do.** It picks the newest version of each package, and tries older ones in that order, and the work it does can grow very fast on a large repository with many conflicts: real resolvers use cleverer search and learn from each dead end. It does not know about pre-release rules beyond what
`compare` knows, or optional dependencies, or two different major versions of one package side by side, which some systems allow. The repository is a plain list, and a real one is a server. And an answer that satisfies the ranges is not an answer that works: the ranges are promises by people.

## What Breaks First

- **Requirements treated one at a time.** The second one fails. See `E-BUILD-0017`.
- **A resolver that never backs up.** A solvable project is declared impossible. See `E-BUILD-0018`.
- **A conflict with no names.** Nobody can fix it. See `E-BUILD-0019`.

## Ship It

`rc/core/resolve.hpp` holds `Range`, `contains`, `is_empty`, `intersect`, `Dep`, `Package`, `Resolution` and `resolve`.

Lesson 04-03 made a package with a version, 04-09 compared versions, and this chooses among them. Phase 04's end is a library installed and consumed by a project that never saw its source; this is how that project's other libraries agree on what to install.
