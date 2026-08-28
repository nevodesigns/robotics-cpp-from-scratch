# The Heap, and Why Every Manual Delete Is Eventually Wrong

> You will not forget the delete. You will write it, and then add an early return above it eighteen months later.

**Type:** Build
**Time:** about 90 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 02-01

## The Problem

A robot runs for weeks. A leak of a few hundred bytes per control cycle is
invisible on a desktop and fatal on a machine that never restarts. At a hundred
cycles a second, a hundred bytes each, the process gains thirty megabytes an
hour and dies some time on Thursday.

Nobody writes a leak on purpose. Leaks arrive when a function grows a second way
out, and the cleanup was written on only one of the paths.

## The Concept

### Two places objects live

Every local variable so far has lived on the **stack**. The stack is a region
that grows and shrinks as functions are entered and left, and it is entirely
automatic: when a function returns, everything it declared is destroyed, in
reverse order of creation, with no instruction from you.

The **heap** is memory you ask for explicitly and must give back explicitly:

```cpp
Reading* readings = new Reading[count];   // ask
delete[] readings;                        // give back
```

The heap exists for two reasons the stack cannot serve:

1. The size is not known until the program runs.
2. The object must outlive the function that created it.

### The two rules of new and delete

**Every `new` needs exactly one `delete`, on every path.** Zero is a leak. Two is
a crash, and often a security hole.

**The bracket forms must match.** `new[]` pairs with `delete[]`, `new` pairs with
`delete`. Mixing them is undefined behaviour, and it usually does not crash
immediately, which is worse than if it did.

### Why manual delete loses

Look at this. It is correct today:

```cpp
double* buffer = new double[count];
fill(buffer, count);
const double result = average(buffer, count);
delete[] buffer;
return result;
```

Now somebody adds a validation check at the top of the body, with an early
return. Or `fill` starts throwing on bad input. Or a second exit is added for a
timeout. Each of those is a reasonable change made by somebody who never looked
at the bottom of the function, and each one leaks.

The problem is structural, not a lapse of attention: **the cleanup is attached to
a place in the text rather than to the lifetime of the object.** Every path out
has to remember, and paths get added by people who were not thinking about it.

The fix is the next lesson, and it is one of the genuinely great ideas in this
language: attach the cleanup to the object itself, so the compiler runs it on
every path out, including the ones nobody has written yet.

### Seeing a leak

Two ways, and you will use both.

Run the address sanitizer, which this repository has a preset for:

```
cmake --preset asan
cmake --build build/asan
```

It reports leaked blocks with the stack that allocated them.

Or count. The tests for this lesson use a type that increments a counter when it
is constructed and decrements when it is destroyed. After the function under
test returns, the counter must be zero. That technique is deterministic, needs no
tooling, and works identically on every platform, which is why the same harness
appears in every later lesson of this phase.

## Build It

`exercise/solution.hpp` has three functions that allocate. Each one has a way out
that skips the cleanup. Make all of them give back everything they take, on every
path, without changing what they return.

- `average_of_readings` returns the mean, and returns 0.0 early when the count is
  not positive.
- `count_above` counts readings above a limit, and gives up early when it finds
  more than `give_up_after`, because the caller only wanted to know whether there
  were many.
- `first_valid_or_default` returns the first reading marked valid, and returns a
  default when there is none.

The tests check the answers **and** that nothing was left allocated.

```
rcpp verify 02-02
```

You are writing manual `new` and `delete` here exactly once in this curriculum,
so that lesson 02-03 means something. After that you will essentially never
write them again, and you will know precisely what the thing that replaces them
is doing.

## Use It

Production C++ almost never contains a bare `new`. It contains `std::vector`
when the size varies, and `std::unique_ptr` when a single object must outlive its
creator, and both give the memory back on every path automatically.

That is not a rule handed down to be obeyed. It is the conclusion of the argument
above, and you have now seen the argument.

## What Breaks First

- **Memory that is never given back.** A path out of the function skips the
  delete. See `E-MEM-0004`.
- **new paired with the wrong delete.** `new[]` needs `delete[]`. See
  `E-MEM-0005`.
- **The same block given back twice.** Usually a pointer copied and both copies
  deleted. See `E-MEM-0002`.

## Ship It

The counting harness in the tests is the tool, not the functions. Every later
lesson in this phase reuses it to prove that ownership actually works, and you
will reach for the same trick the first time you suspect a leak in real code.
