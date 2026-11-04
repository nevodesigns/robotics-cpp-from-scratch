# The Tool That Watches It Run: Sanitizers, and What Each One Will Not Tell You

> Writing this lesson, the author discovered that his own sanitizer lane had
> not been instrumented for an entire working session. The option is
> `RC_SANITIZE_ADDRESS`; he had been passing `RC_SANITIZE`, which CMake
> accepts in silence as a variable nothing reads. Continuous integration used
> the right one throughout, so the coverage was real and the claim was not.

**Type:** Build
**Time:** about 110 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 01-06, 02-02, 02-09, 04-02

## The Problem

You have been relying on these since lesson 02-02 without being told what
they are. Ten lessons in this curriculum mention a sanitizer, continuous
integration builds a job with two of them on every push, and until now the
only section about them anywhere was called *"the sanitizer lane does not
cover this"*.

A sanitizer is not a debugger and not a static analyser. It is extra code the
compiler weaves into your program, which checks things while the program runs
and reports the first one that is wrong.

The headline is the part nobody says out loud: **the two overlap, neither is a
superset of the other, and there are whole classes of defect that neither one
sees.**

## The Concept

### What each one found, measured

The same read past the end of a `new[]` block, and the same signed overflow,
run three ways each on gcc 11 at `-O1`:

| defect | plain build | address | undefined |
| --- | --- | --- | --- |
| read past the end of a block | printed `read 0`, carried on | `heap-buffer-overflow` | reported it **sometimes**, see below |
| signed overflow | printed `-1293967296` | printed `-1293967296`, said nothing | `signed integer overflow: 3001 * 1000000 cannot be represented` |

The "sometimes" is the interesting row, and it was a surprise. With the block
size known at compile time, the undefined behaviour sanitizer **did** report
the overflow, through its object-size check. With the size known only at
runtime it reported **nothing at all**. The address sanitizer caught both,
because it puts poisoned redzones around every allocation rather than
reasoning about declared object sizes.

So: the undefined behaviour sanitizer finds some out-of-bounds reads and
cannot be relied on for them.

### The whole table

Five common defects. This is what the lesson asks you to encode, and every row
was measured rather than looked up.

| defect | found by | in this curriculum's lane |
| --- | --- | --- |
| heap overflow | address | yes |
| use after free | address | yes |
| signed overflow | undefined | yes |
| **uninitialised read** | **neither** | no |
| **data race** | thread, which cannot be combined with address | no |

**Three of five.** The two it misses are the two this curriculum has measured
most carefully, and that is not a coincidence: a defect no tool reports is one
you have to design against instead. Lesson 02-09 answered the uninitialised
read by making the storage loud, lesson 03-11 by pushing it into a constant
expression where the compiler refuses it, and phase 07 handles races.

And the limit that applies to all of it: **a sanitizer only ever reports what
the test actually ran.** The lane is worth exactly as much as the suite's
coverage, which is why this one runs over every reference test rather than a
smoke test.

### What they cost, measured

Best of 5, `-O2`, a tight loop over 200000 ints, 40 passes:

| | time | relative |
| --- | --- | --- |
| no sanitizer | 8.2 ms | 1.0x |
| address | 14.6 ms | 1.8x |
| **undefined** | **34.0 ms** | **4.1x** |
| both | 48.2 ms | 5.9x |

That ordering is the reverse of the usual assumption, which has the address
sanitizer as the expensive one. On this workload the per-operation arithmetic
checks cost more than the memory shadow checks.

State the caveat as loudly as the number: this is one workload, and it is
arithmetic-heavy. A pointer-chasing workload would very likely order them the
other way. The useful conclusion is not a ratio, it is that **both are cheap
enough to run on every push and neither is cheap enough to ship.**

### The instrumentation is not a passive observer

This is the part that changes how you write tests.

The address sanitizer **replaces the allocator**. Freed blocks go into a
quarantine rather than being handed straight back; allocations are surrounded
by poisoned redzones; memory is poisoned on the way out.

Measured on lesson 02-09's own heap test, which writes `0x7e` into 32 bytes,
frees them, and asks for 32 bytes again:

| | zero on arrival | still holding the pattern |
| --- | --- | --- |
| no sanitizer | 27 of 32 | **16 of 32** |
| address sanitizer | 0 of 32 | **0 of 32** |

Both are correct. Neither is a fact about C++. A test that asserted the first
pair would fail in the sanitizer lane, and a test that said nothing about
which build it was in would be publishing a number without saying what it
measured.

It is the same instrumentation that stops lesson 03-10 demonstrating a missing
virtual destructor: the delete is reported as `new-delete-type-mismatch` and
the program stops, which is right of the sanitizer and useless as a
measurement.

### Asking which build you are in

There is no portable way, and the two compilers disagree about the spelling.
Measured across eight configurations:

| | `__SANITIZE_ADDRESS__` | `__has_feature(address_sanitizer)` |
| --- | --- | --- |
| gcc 11 | defines it | **has no `__has_feature` at all** |
| clang 14 | **does not define it** | has it |

And for the undefined behaviour sanitizer: **neither compiler defines
anything.** Dumping every predefined macro under `-fsanitize=undefined` on
both produces nothing matching `SANITIZ`.

So the header reports that one as *undetectable* rather than as off. A test
must not conclude the undefined behaviour sanitizer is absent, only that it
cannot find out.

### The preprocessor trap, which caught this lesson

The obvious way to write the detection does not work:

```cpp
#if defined(__has_feature) && __has_feature(address_sanitizer)   // wrong
```

gcc refuses it:

```
error: missing binary operator before token "("
```

The `&&` does not protect the right hand side. The preprocessor parses the
**whole** expression before evaluating any of it, so `__has_feature(...)` must
be syntactically valid even where it does not exist. An unknown bare
identifier becomes `0`, which is why `#if SOME_UNDEFINED_MACRO` is legal. An
unknown identifier **followed by a parenthesis** is a syntax error.

The tests have to be nested, and there is no shorter correct form in C++17:

```cpp
#define RC_EXPECT_ASAN 0
#if defined(__has_feature)
#  if __has_feature(address_sanitizer)
#    undef RC_EXPECT_ASAN
#    define RC_EXPECT_ASAN 1
#  endif
#endif
#if defined(__SANITIZE_ADDRESS__)
#  undef RC_EXPECT_ASAN
#  define RC_EXPECT_ASAN 1
#endif
```

`__has_include` is the exception: C++17 requires it, so it needs no guard.

### The guard that costs nothing

Because the detection is `constexpr`, a branch on it is resolved at compile
time and one side is removed. So a suite carries both paths and pays for
neither:

```cpp
if (rc::test::allocator_measurements_trustworthy()) {
  // the real allocator: the number means something
} else {
  RC_CHECK(recovered == 0u);   // the quarantine: assert that instead
}
```

That is better than maintaining two suites or excluding the test from a lane,
because both claims stay in one file where they can be compared.

### How a lane goes quietly dead

The epigraph is not a joke at the author's expense; it is the most useful thing
in the lesson.

```bash
cmake -S . -B build/asan -DCMAKE_BUILD_TYPE=Debug -DRC_SANITIZE=address,undefined
```

The option here is `RC_SANITIZE_ADDRESS`. CMake accepts the line above without
complaint, because an unknown `-D` is a legal way to pass a variable a project
might read later. The result is an ordinary Debug build in a directory called
`asan`, and nothing ever fails.

The tell was a measurement that refused to move: the 02-09 heap test reporting
the same 16 of 32 in both lanes, when genuine instrumentation gives 0 of 32.

Two defences, both cheap.

**Ask the build, not the command line.**

```bash
ninja -C build/asan -t commands <target> | grep -o '\-fsanitize=[a-z,]*' | sort -u
```

Empty output means an uninstrumented build whatever the directory is called.

**Have the suite print what it was compiled with**, which is what this
lesson's header is for. A suite that says so cannot quietly become a suite
that checks nothing.

## Build It

Five pieces, and the first two are preprocessor work.

Define the detection macro, nested, both spellings. Report it from a
`constexpr` function. Say plainly that the undefined behaviour sanitizer
cannot be detected. Derive the allocator trustworthiness from the detection
rather than writing a second one. Then fill in the five-row table from the
measurements above, which are printed in the reference for exactly that
purpose.

A warning about the first two: a detection that always answers "no" is correct
in the build you run by hand, so the suite checks the macros itself,
independently, and can only catch a wrong answer in a build that actually has
the sanitizer on. Configure one:

```bash
cmake -S . -B build/asan -DCMAKE_BUILD_TYPE=Debug -DRC_SANITIZE_ADDRESS=ON
```

## Use It

```cpp
#include <rc/test/sanitizer.hpp>

std::cout << "built with: " << rc::test::sanitizer_name() << "\n";
```

Put that line in any suite whose numbers depend on the allocator, and guard
any such measurement with `allocator_measurements_trustworthy()`.

## What Breaks First

`E-DEBUG-0010` is the lane that was never switched on, including how to ask
the build what it actually did.

`E-MEM-0017` is the allocator measurement that changed under instrumentation.

`E-BUILD-0003` is `missing binary operator before token "("`, and why the
`&&` does not protect what follows it.

## Ship It

`rc/test/sanitizer.hpp`, in `namespace rc::test`.

The five-row table stays in the lesson rather than the library. It is
knowledge about tools, which belongs in a document and a test where it can be
read and argued with, not in a header where it would look like an API.

This lesson has no `checks/` directory, and that is deliberate: its subject is
runtime instrumentation, so the guarantees are demonstrated by running, not by
refusing to compile.
