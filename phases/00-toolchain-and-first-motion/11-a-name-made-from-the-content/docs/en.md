# A Name Made From the Content: How Version Control Knows Two Things Are the Same

> When something is called by what is in it, two things with the same name are the same thing, with no
> comparing. That one idea is under version control, caches and downloads.

**Type:** Build
**Time:** about 90 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 00-01, 00-04

## The Problem

A robot project changes every day. By the end of a month there are thirty versions of a hundred files, and the question is which of them is the one that worked
on Tuesday, and what changed since.

Keeping thirty copies of the folder answers the first question slowly and the second not at all. Comparing two folders file by file answers the second slowly. Both questions get
easy if every file has a short name that is made from what is in it, so that the same name means the same content.

That is what a version control tool does. This lesson builds the idea in a few lines, so that when you use the tool you know what it is doing.

## The Concept

### A name made from the content

A **hash** turns any text into a fixed size number. FNV-1a is one of the simplest: start at a fixed number, and for each byte xor it in and multiply by a prime.

```cpp
hash ^= byte;
hash *= 1099511628211ULL;
```

The same text always gives the same number, on any machine. The test checks it against the published value for the text "a".

### One change changes everything

A good hash has the property that makes it useful: changing one character changes about half of the bits of the name. Changing one character of a 200 character text in
every place in turn, a mean of **32.4 of the 64 bits** differed, between 16 and 44. A thousand different texts got a thousand different names.

That is why equal names can be trusted to mean equal content, and why a name made from, say, the length of the text could not. See `E-CPP-0078`.

### Naming a whole version

A version of the project is a set of files. Name each file from its name and its content together, and combine the names with xor, which gives the same answer in whatever order
they are combined:

- the same three files listed in two orders have the same snapshot name;
- editing a file, renaming it or removing one gives a different name;
- a file's content name does not depend on what the file is called, and its name in the snapshot does.

A folder has no order, so a name that depends on one is a bug. See `E-CPP-0077`.

### Why saving every version is cheap

Two versions of a hundred files with one file edited: 200 files listed, and **101** different contents. The ninety-nine that did not change have the same name in both, so they are stored once.

A version is then a list of names, and the contents are kept in one place under their names. This is why a commit in a version control tool is quick and small, even for a large project. See `E-CPP-0079`.

## Build It

`content_id`, `differing_bits` and `snapshot_id` in `exercise/solution.hpp`. `scramble` and `file_id` are given.

```
rcpp verify 00-11
```

## Use It

`rc/core/content_id.hpp` joins `rc::core`. You will meet the idea next in the tool you use for this repository:

```
git add -A
git commit -m "what changed"
git log --oneline
```

A commit is a snapshot named from its content, which is why `git log` shows a short code beside each one. This is a toy next to the real thing: the real tool uses a longer hash than 64 bits, stores the
contents in compressed form, and names each commit from its snapshot, its parent and its message. It is worth running `git log --oneline` in this repository and looking at the codes.

**What this does not do.** FNV-1a is not safe against somebody trying to make two files collide, and only a cryptographic hash is. A 64 bit name could, in a very large store, eventually give two different
contents the same name. It reads text up to its end marker, so a file with a zero byte in it is cut short; a real tool names the bytes and their length.

## What Breaks First

- **A name made from the order or the time.** Equal things look different. See `E-CPP-0077`.
- **A hash where a small change gives a small difference.** Different things look equal. See `E-CPP-0078`.
- **Storing every version in full.** Space grows with the number of versions and not with the changes. See `E-CPP-0079`.

## Ship It

`rc/core/content_id.hpp` holds `Id`, `content_id`, `differing_bits`, `File`, `scramble`, `file_id` and `snapshot_id`.

The rest of this curriculum is saved one lesson to a commit. This is what each of those commits is.
