# Two Functions, One Name: Namespaces, the Two Colons, and Why using namespace std Is Not Written Here

> A call to `clamp(5, 0, 3)` stopped reaching the function the lesson wrote and
> ran another one, returned a different type, and said nothing.

**Type:** Build
**Time:** about 95 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 01-02, 01-04

## The Problem

Every lesson so far has written `std::` in front of the standard library's names
and `rc::` in front of this curriculum's, and nobody has said what the two colons
are. Lesson 00-03 showed `std::sqrt` in a compiler message and asked you to treat
the prefix as part of the name. Rule L018 forbids `using namespace std;` in every
lesson, and says only that the prefix should be "written out".

This is the page that says why. It is placed here, after the lessons that needed
it, because it needs functions from 01-02 to be about.

The short version: the prefix is not decoration. It is what stops a call from
quietly reaching a function you did not write.

## The Concept

### A namespace is a place for names

Two subsystems each need a function that keeps a value in range. One keeps a speed
inside plus and minus one. The other keeps an angle inside a quarter turn. Both
want to be called `limit`.

```cpp
namespace drive { double limit(double speed); }
namespace arm   { double limit(double angle); }
```

Two functions, one name, and no conflict, because each lives in its own
**namespace**. The two colons say "the one in there":

```
  the same call, limit(5.0), in two namespaces:
    drive::limit(5.0)   1
    arm::limit(5.0)     1.5708
```

`std` is where the standard library keeps its names. `rc::core` is where this
curriculum keeps its own. That is all a prefix is, and it is the reason two
people can both write a function called `clamp`.

### The leading colons

`::clamp` with nothing before the colons means "the one in the global namespace,
and nothing else". `drive::limit` uses it:

```cpp
inline double limit(double speed) { return ::clamp(speed, -kMaxSpeed, kMaxSpeed); }
```

Lesson 01-02's `clamp` lives in the global namespace, and the suite counts its
calls. `drive::limit` and `arm::limit` must each make exactly one, through the
leading colons, for a reason the next sections measure.

### A using-directive imports every name

```cpp
using namespace drive;
```

makes everything in `drive` visible without its prefix. It is the shortcut that
looks harmless, and it fails in two ways.

**Loudly.** Make two namespaces visible and both can offer the same name:

```cpp
using namespace drive;
using namespace arm;
limit(1.0);          // which one?
```

```
error: call of overloaded 'limit(double)' is ambiguous       (gcc)
error: call to 'limit' is ambiguous                          (clang)
```

Both fit equally, so both compilers refuse, which
`checks/two_namespaces_both_used.cpp` proves. A global variable called `count`
under `using namespace std;` is refused the same way, `reference to 'count' is
ambiguous`, because `std` has an algorithm of that name. The line that fails is
not the line that changed: a header you included started offering a name.

`E-CPP-0068`.

**Quietly.** This is the one that matters.

### The better match wins, and it is not always yours

Lesson 01-02 wrote `clamp(double, double, double)`. C++17 has `std::clamp` as well.
Experiment, method stated: a small program with the global `clamp`, run on gcc 11
and clang 14, calling `clamp(5, 0, 3)` with plain ints, once without
`using namespace std;` and once with it.

| | the global `clamp` ran | result type |
| --- | --- | --- |
| as written | yes | **double**, 8 bytes |
| after `using namespace std;` | **no** | **int**, 4 bytes |

The ints fit `std::clamp` exactly and the global one only after converting them to
double, so the better match won, and the better match was the function the author
did not write. No warning on either compiler.

The suite reproduces the same shape with a namespace of its own, `general`, holding
an `int` overload, so the measurement does not depend on what a given standard
library happens to contain:

```
  clamp(5, 0, 3), written the same way twice:

                                   result type   global clamp ran
    as written                     double        1 time
    after using namespace general  int           0 times
```

Nothing in the line changed. A `using` statement elsewhere in the scope did.

And the learner's `drive::limit`, with its leading colons, still reached the
global `clamp` exactly once while that directive was in scope. That is what the
colons buy.

`E-CPP-0069`.

### A name can be found through its arguments

```
  distance(v.begin(), v.end()) with no std::   3
  std::distance(v.begin(), v.end())            3
```

The first line has no `std::` and works. The compiler looks for a function in the
namespaces of the call's arguments as well as the current one, which is called
**argument dependent lookup**, and a `std::vector`'s iterators belong to `std`.

Change the container to an `int` array and the arguments are plain pointers, which
belong to no namespace:

```
error: 'distance' was not declared in this scope; did you mean 'std::distance'?
```

`checks/a_name_that_only_worked_for_a_vector.cpp` is that refusal. Code that leans
on this is correct for one container and broken for the next, and the error is
far from the change that caused it.

Argument dependent lookup is not a bug. It is why `std::cout << text` finds the
right `operator<<` without a prefix, and why a few names, `swap` above all, are
meant to be called unqualified. Until you know which, write the prefix.

`E-CPP-0070`.

### The tools that are fine

| | |
| --- | --- |
| `std::clamp(v, lo, hi)` | the prefix. It cannot be found by accident or lost by changing a type. |
| `using drive::limit;` | a using-declaration. Names one function, and is fine in a small scope. |
| `namespace fs = std::filesystem;` | an alias. Shortens a long prefix and imports nothing. |
| `using namespace x;` for your own `x`, in a `.cpp` | acceptable, if you know what `x` contains. |
| `using namespace std;` | not written in this curriculum, and in a header it reaches every file that includes it. |

## Build It

Two functions, one name.

`drive::limit` keeps a speed between minus `kMaxSpeed` and `kMaxSpeed`. `arm::limit`
keeps an angle between minus `kQuarterTurn` and `kQuarterTurn`. Each calls the
global `clamp`, with the leading colons, once.

The stubs answer zero. The suite checks the values, counts the calls to the global
`clamp` to prove each function went through it, and then puts
`using namespace general` in scope and checks that `drive::limit` still reaches the
same function exactly once.

## Use It

```cpp
#include <rc/core/args.hpp>

rc::core::Arguments args(argc, argv);
namespace fs = std::filesystem;
```

From here on that is what you will read: the prefix written out, an alias for the
long one, and no using-directive for a namespace you do not own.

Two limits.

**This does not make a name safe.** It stops a call being redirected by a
using-directive. It does not stop two libraries defining the same name in the
global namespace, which is why `rc` has a namespace at all.

**Lookup has more rules than are covered here.** The part that bites is the one
measured: a better match in a namespace you did not write.

## What Breaks First

`E-CPP-0068` is the call that is ambiguous between two namespaces, and the `count`
that stopped compiling because a header started offering a name.

`E-CPP-0069` is the call that quietly reached a different function, with the
measurement of what `std::clamp` did to a call meant for the global one.

`E-CPP-0070` is the name that worked for a vector and not for an array, with the
compiler suggesting the fix and no explanation of why it ever worked.

## Ship It

No header graduates from this lesson. Its output is a habit, the prefix written
out, and the three tools in the last table.

From here on, a call in this curriculum names its namespace, and a leading `::`
means the global one and nothing else.
