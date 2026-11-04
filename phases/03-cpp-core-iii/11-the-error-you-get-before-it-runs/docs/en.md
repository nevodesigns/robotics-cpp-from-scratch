# The Error You Get Before It Runs: constexpr, and What the Compiler Refuses to Compute

> Writing this lesson's own suite, the benchmark reported 0.0 microseconds for
> both sides at `-O2`, because the optimiser deleted the work. The agreement
> test passed with both functions stubbed to return 0. And audit rule L022
> caught a `constexpr` declared inside a function, which is the fifth time
> this repository has made that exact mistake.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 01-10, 02-08, 02-09, 03-07

## The Problem

Everything this curriculum has measured so far, it measured while the program
was running.

There is another time at which work can happen, and it is free: while the
program is being built. A table computed then arrives in the binary already
filled in, and the machine that has to be ready when power comes up does not
spend its first milliseconds doing arithmetic that never changes.

That is the advertised reason for `constexpr`, and it is the smaller half.

## The Concept

### The real reason: undefined behaviour is not a constant expression

A constant expression has to be **computable**. The rules of what is
computable exclude undefined behaviour, and that single fact is worth more
than the speed.

At runtime, the compiler is entitled to assume undefined behaviour does not
happen, so it compiles your code and something arbitrary occurs. In a constant
expression it cannot assume: it has to produce a value, and when there is none
to produce it stops and the build fails.

| the expression | at runtime | in a constant expression |
| --- | --- | --- |
| signed overflow | undefined; 00-04 measured the wrap, 01-13 measured `atoi` giving -1 | **compile error** |
| an index past the end | undefined; the sanitizer may catch it | **compile error** |
| a division by zero | undefined | **compile error** |
| reading something never written | undefined, and nothing catches it | **compile error** |

The last row is the one to remember, and `checks/a_value_never_written.cpp`
ships it.

Lesson 02-09 measured that case at length: a struct member read inside a
function it was passed to produced **no warning on gcc or clang at any
optimisation level**, the address sanitizer ran the program clean and printed
the garbage, and the memory sanitizer, which is the tool for this, is clang
only, cannot be combined with the address sanitizer, needs an instrumented
standard library, and missed that exact case on this machine.

In a constant expression both compilers refuse it:

```
gcc:   uninitialized variable 'x' in 'constexpr' function
clang: uninitialized variable in a constexpr function is a C++20 extension
```

Clang's wording is the more informative one. C++20 relaxed the rule so the
declaration is allowed, and reading one during constant evaluation is still not
a constant expression, so the error moves rather than disappearing. This
curriculum is C++17, where the declaration is refused outright.

So: anything you can push into a constant expression is checked for nothing.

### const is not constexpr

They sound like one word and answer two questions.

**`const` is about writing.** Nobody may change this through this name. It
says nothing about when the value is known.

**`constexpr` is about knowing.** The value is computable during translation,
and a declaration that cannot deliver that is an error.

```cpp
const int size = readings.size();     // const, not a constant expression
int buffer[size];                     // refused, and C++ has no VLAs
```

The confusing part is that `const` sometimes works:

```cpp
const int kCount = 8;                 // initialised by a literal
int buffer[kCount];                   // accepted
```

An integral `const` initialised by a constant expression is itself usable as
one, by a rule inherited from C++98. **Integral only**, which is why the same
thing with a `double` fails on both compilers:

```
gcc:   the value of 'kd' is not usable in a constant expression
clang: constexpr variable 'cd' must be initialized by a constant expression
```

A feature that appears to work until the first time it matters. Say which you
mean, and prefer `constexpr` for anything that really is a build-time constant,
because then a later change that makes it runtime is an error rather than a
silent demotion.

### constexpr is permission, not instruction

This is the part that quietly does not work.

`constexpr` on a function says it *may* be evaluated during translation if it
is used somewhere that requires a constant. Used anywhere else it is an
ordinary function and it runs like one.

```cpp
const std::uint8_t a = bits_in_byte(0xAA);   // may run at either time
const std::uint8_t b = bits_in_byte(n);      // runs at runtime, necessarily
```

The first line is the trap: folding it is permitted, not folding it is
permitted, and nothing in the source records which happened. The same applies
to the variable, which is where tables lose:

```cpp
const     std::array<std::uint8_t, 256> t = make_table(...);  // may be built at startup
constexpr std::array<std::uint8_t, 256> t = make_table(...);  // must be built at compile time
```

Write `constexpr` and a build that could not do the work at compile time is a
build that fails. That is the guarantee you wanted.

### What you can prove, and what you cannot

**You can prove an evaluation happened at compile time**, by putting it where
a runtime value would be refused.

**You cannot prove one did not happen.** There is no portable way to assert
that the compiler declined to fold something.

That asymmetry decides how the suite is written, and it is worth copying. The
compile-time facts are `constexpr` variables at file scope:

```cpp
constexpr std::uint8_t kBitsIn255 = kBitsPerByte[255];
constexpr int kOnesInAllOnes = count_set_by_table(0xFFFFFFFFu);
```

Each compiles if and only if the compiler could work the value out. The values
are then checked at runtime, with `RC_CHECK`.

Putting the values in `static_assert` instead would have been the stronger
check and the worse lesson: an unfinished exercise would not have built at
all, and a suite that cannot run tells a learner nothing about which function
to write. Proving *when* an evaluation happened and checking *what* it produced
are two different jobs.

### Building the table

```cpp
template <class T, std::size_t N, class Compute>
constexpr std::array<T, N> make_table(Compute compute) {
  std::array<T, N> table{};
  for (std::size_t i = 0; i < N; ++i) table[i] = compute(i);
  return table;
}
```

Three rules from C++14 and later make that possible, which is why none of this
was worth teaching in 1998: a `constexpr` function may contain a loop,
`std::array::operator[]` is `constexpr`, and a lambda is implicitly `constexpr`
when it only does things a constant expression allows.

The braces on `table{}` are not decoration. An array left uninitialised cannot
be read in a constant expression, so without them this does not compile, which
is 02-09's defect caught at build time.

The suite prints what came out:

```
  256 entries, computed during compilation

    kBitsPerByte[255]              8
    kBitsPerByte[0xAA]             4
    count_set_by_table(0xFFFFFFFF) 32
    count_set_by_table(0xAAAAAAAA) 16
```

### What it costs, measured

100000 values, best of 7 interleaved rounds, four table lookups against the
same count written as 32 iterations:

| | `-O0` | `-O2` |
| --- | --- | --- |
| gcc 11 | 6.9x faster | **20.7x faster** |
| clang 14 | 8.7x faster | 7.6x faster |

The direction held in all four configurations, so the suite asserts the
direction. It does not assert the ratio, because the ratio is a fact about one
machine, and a number here would be a claim about yours.

The speed is still the smaller half of the argument. The table costs 256 bytes
of read-only data and **no startup work at all**.

### Two ways this lesson's own suite was wrong

Both are the kind of mistake that passes review, so they are in the lesson
rather than quietly fixed.

**The benchmark measured nothing at `-O2`.** The first version ended each timed
loop with `if (total < 0) std::cout << "";`, which reads like it keeps the work
and does not. Both sides reported **0.0 us** and the test failed, because the
accumulation was dead and the optimiser removed it. A `volatile` store fixed
it: the compiler may not elide that, so everything the total depends on has to
happen. A benchmark whose result nothing reads is a way of timing an empty
loop.

**The agreement test passed with both functions stubbed to return 0.** Checking
that the table and the loop agree says nothing when both are equally broken.
Agreement is evidence that one of them is not *uniquely* wrong, and it is not
evidence that either is right. The test is now anchored to known answers first,
and then compares.

### The portability trap, for the fifth time

A `constexpr` local used inside a lambda needs no capture on gcc and clang,
because using it for its value is not an odr-use, and **MSVC rejects it as
C3493**. Audit rule L022 asks for file scope whether or not a lambda is
involved, because detecting the lambda case properly needs a parser and the fix
is harmless either way.

It caught this lesson. That is five times in this repository, four of them
after the atlas entry had already been written, which is the argument for
putting a rule in the tool rather than a note in a document. `E-CPP-0023` is
the entry.

## Build It

Four pieces, smallest first.

`bits_in_byte`, eight iterations of shifting and masking, which is 01-10
again. Keep it `constexpr`: that is what lets the table be built during
compilation.

`make_table`, three lines, all of which have to be things a constant
expression allows.

`count_set_by_table`, four lookups, one per byte.

`count_set_by_loop`, the 32 iteration version, so the suite can compare them.

Then look at what happens if you drop the `constexpr` from the table variable.
The program still works. That is the whole problem.

## Use It

```cpp
#include <rc/core/table.hpp>

constexpr std::uint8_t bits_in_byte(std::size_t value);

constexpr std::array<std::uint8_t, 256> kBitsPerByte =
    rc::core::make_table<std::uint8_t, 256>(bits_in_byte);
```

Reach for this when a table is small, fixed, and derivable by arithmetic: bit
counts, parities, a gamma curve, a fixed conversion. Not when the values come
from a file or a calibration, which are exactly the things the compiler cannot
know.

## What Breaks First

`E-CPP-0057` is the expression the compiler refused, with the four cases and
why the refusal is usually good news.

`E-CPP-0058` is the `constexpr` function that ran at runtime anyway, and how
to insist.

`E-CPP-0059` is `const` mistaken for `constexpr`, including why the integral
case works and the `double` case does not.

`E-CPP-0023` is the MSVC C3493 trap, which predates this lesson and caught it
anyway.

## Ship It

`rc/core/table.hpp`, in `namespace rc::core`.

The one function in it is three lines. The reason it is worth a header is the
comment above it, which is the only place in this curriculum that records why
compile-time evaluation is a correctness tool before it is a speed one.
