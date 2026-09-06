# One Definition, and the Two Copies You Did Not Ask For

> Three bumps, and no counter said three. Nothing failed to link and nothing
> warned.

**Type:** Build
**Time:** about 150 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 04-01, 03-06

## The Problem

A header is text. It is copied into every file that includes it, so anything it
defines is defined that many times, and the linker's rule is that one entity may
have one definition in the program.

There are three ways to satisfy that rule and they are not equivalent:

| written as | what the program gets |
|---|---|
| nothing | two definitions, and the link fails |
| **`static`** | **one per file, silently** |
| `inline` | one, shared |

The first is loud and gets fixed in minutes. The second ships.

This is also the first lesson in the curriculum whose tests need **two
translation units**, because the entire class of fault is invisible from one.

## The Concept

### The one that links

A counter behind a `static` function in a header, bumped twice from one file and
once from another:

| | |
|---|---|
| bumps from the first file | 2 |
| bumps from the second file | 1 |
| count the first file can see | **2** |
| count the second file can see | **1** |

Three bumps happened and no counter says three.

`static` at namespace scope, or on a free function, says "give this file its
own". The linker is then satisfied, because there is no shared entity to
conflict about, and both files are perfectly consistent with themselves.

That is what makes it hard to see. A registry that is missing entries somebody
is certain were added, a cache that misses things that were put in it, a
singleton initialised more than once: all the same fault, and nothing anywhere
reports it.

The addresses of the two functions may or may not differ, because a linker is
allowed to fold two functions whose generated code is identical. **The counts
are the thing to look at**, which is a small lesson in choosing what to assert.

### The one that fails loudly

```cpp
// in a header
int wheel_speed() { return 1; }     // a definition
double wheel_base = 0.35;           // a definition
```

With one includer this links. With two it does not, and the second file did
nothing wrong. The compiler never sees the problem, because each file on its own
is correct.

Two snippets in `checks/` prove it. Each defines the same thing twice in one
file, which is what the linker sees from across two, and the build compiles them
and expects to fail.

### The one you meant

```cpp
static Registry& instance() {
  static Registry only;
  return only;
}
```

A function-local static inside an **inline** function. The `inline` makes the
function one entity across the whole binary, and the standard then guarantees
its local static is one object, constructed once, however many translation units
call it.

The tests measure exactly that: two files, each registering something before
`main` runs, neither knowing the other exists, and one registry containing both.
The address of `instance()` is the same in both files, which is what makes the
static inside it one object rather than one per file.

A member function defined inside a class is already inline, which is why
header-only classes work without anybody writing the keyword. C++17's `inline`
variables do the same for data, and `constexpr` variables at namespace scope are
implicitly inline. Plain `const` at namespace scope has **internal** linkage,
which puts it back in the middle row.

### Names are not addresses

The registry looks entries up by name, and that is a second place to go wrong:

```cpp
if (entry.name == name) return entry.value;      // comparing addresses
if (std::strcmp(entry.name, name) == 0) ...      // comparing the name
```

Two identical string literals are not required to be the same object. Most
compilers pool them within a file and many across a binary, so the first version
appears to work until a build setting changes.

So the test hands in a name spelled out character by character:

```cpp
char spelled_out[] = {'s', 'o', 'm', 'e', 't', 'h', 'i', 'n', 'g', '\0'};
RC_CHECK(registry.find(spelled_out) == &value);
```

One line, and it is the difference between a test that passes because the
compiler pooled the literals and a test that checks the lookup.

### Where this already lives in the repository

`rc/test/leak_check.hpp` keeps its counters behind inline functions for exactly
this reason, and it is documented as belonging to one translation unit per
binary: two copies of those counters would report leaks that are not there.

And `librc` compiles every header twice, into two translation units, and links
the pair. That gate exists because a header in this repository once needed it.

## Build It

Implement `instance`, `add` and `find` in `exercise/solution.hpp`.

```
rcpp verify 04-04
```

The suite has two `.cpp` files. Both include the same header, both register
something before `main` runs, and the tests ask each of them what it can see.
The build also compiles two programs from `checks/` and expects both to fail.

## Use It

**`inline` for anything a header defines** that there should be one of.

**`static` in a header almost never.** When it appears, ask what it is for.

**A declaration in the header and a definition in one `.cpp`** for anything
large, or anything whose definition you do not want every includer to depend on.

**Compare names by content**, and test with a name that cannot share an address.

**Add a second translation unit to the test.** Four lines, and it finds this
whole class of fault.

## What Breaks First

- **A header-only global that is one per file.** See `E-LNK-0003`.
- **A definition in a header, and a link that fails on the second file.** See
  `E-LNK-0004`.
- **Comparing names by pointer.** See `E-LNK-0005`.

## Ship It

`Registry` and `Registered` join `rc::core`. Anything from here that needs one
of something, shared by files that do not know about each other, has a pattern
that is correct rather than one that happens to work in the file it was tested
in.
