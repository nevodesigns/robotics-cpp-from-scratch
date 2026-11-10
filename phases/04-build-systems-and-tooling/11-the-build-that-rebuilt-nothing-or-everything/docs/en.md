# The Build That Rebuilt Nothing or Everything: Timestamps, Content and a Cache That Never Serves Stale Output

> A build tool must decide, for every file, whether the output it already has is still right. Deciding by when the
> file was written is wrong in both directions.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 00-11, 04-01

## The Problem

Lesson 04-01 found a build that said there was no work to do when there was. This lesson asks why a build tool gets that wrong, and why the opposite complaint, that it rebuilds everything after a branch switch, has the same cause.

The usual rule is: rebuild if the source is newer than the output. It is cheap and it works until a file's time stops meaning what the rule needs it to mean.

## The Concept

### The timestamp rule

`needs_rebuild_by_time(source_time, output_time)` is true when the source is newer. A source and the output built from it a moment later can share a second, so equal times count as up to date.

### Too much: a touch, a revert

Every write gives a file a new time, whatever it wrote. A file built, edited and reverted was compiled **3** times by timestamp and **2** by content: the revert found the object of the first build. After a plain `touch` the timestamp said rebuild and the content key was a hit. See `E-BUILD-0012`.

### Too little: a restored file

A file restored from a backup keeps the time it had there. A file whose text changed and whose timestamp (400) was older than the object (1000): the timestamp rule said **no** rebuild, so the old object would have been used. The content key found no object for the new text. See `E-BUILD-0011`.

### A name made from the content

Lesson 00-11 gave every piece of text a name made from what is in it. A build output depends on more than the text, so the key joins three names:

```cpp
scramble(a ^ (b * 0x9E3779B97F4A7C15ULL) ^ (c * 0xC2B2AE3D27D4EB4FULL));   // name, text, flags
```

The suite checked that the key changes with the flags (`-O2`, `-O0`, `-O2 -DNDEBUG`), with the file name and with the text, and that swapping the name and the text, or the name and the flags, does not give the same key. The multiplications are there so that two equal parts cannot cancel. See `E-BUILD-0013`.

### The cache

`BuildCache` stores an output under its key and counts hits and misses. Over 300 random builds of one file with 4 versions and 2 sets of flags, where "compiling" writes the whole input into the output so a stale object can be recognised: **8** compiles instead of 300, **292** hits and **0** stale objects served.

## Build It

`needs_rebuild_by_time`, `build_key` and `BuildCache::lookup` and `store` in `exercise/solution.hpp`.

```
rcpp verify 04-11
```

## Use It

`rc/core/build_cache.hpp` joins `rc::core`.

```cpp
const Id key = build_key(name, text, flags);
std::string object;
if (!cache.lookup(key, &object)) { object = compile(...); cache.store(key, object); }
```

**What this does not do.** It keys on one file's text and flags. A real compiler output also depends on every header the file includes, the compiler's own version and the environment, and a tool like ccache hashes the preprocessed text for that reason. A 64 bit name can in theory collide, and a build cache that serves the wrong object is worse than one that
rebuilds, so a real one uses a longer hash. It never forgets anything, so it grows. And hashing every file on every build costs time that a timestamp comparison does not: tools do both, and hash only a file whose time has changed.

## What Breaks First

- **A restored file with an old time.** The old object is used. See `E-BUILD-0011`.
- **A touch or a revert.** Rebuilds for no reason. See `E-BUILD-0012`.
- **A key without the flags.** A debug object in a release build. See `E-BUILD-0013`.

## Ship It

`rc/core/build_cache.hpp` holds `needs_rebuild_by_time`, `build_key` and `BuildCache`.

Lesson 04-01 explained why a build can say there is nothing to do. This is why, and what to key it on.
