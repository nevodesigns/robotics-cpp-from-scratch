# The Allocation You Did Not See: Count It, Do Not Time It

> The loop body that touches nothing but stack had a worse worst case than the
> one that allocates. Both worst cases were the operating system.

**Type:** Build
**Time:** about 150 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 07-03, 02-02

## The Problem

Lesson 07-03 measured a loop's jitter and produced a histogram with a long tail.
The obvious next question is what is in the tail.

The obvious next move is to time smaller and smaller pieces until the slow one
falls out. That does not work, and this lesson is mostly about why, because the
reason changes how you find every problem of this kind.

## The Concept

### The stopwatch cannot hear it

Two loop bodies, a hundred thousand iterations each. One builds a small vector
every tick. The other writes into a fixed array and cannot allocate at all:

| loop body | p50 | p99 | p99.9 | worst |
|---|---|---|---|---|
| a fixed array | 0.250 us | 0.300 | 0.450 | **60.376** |
| a vector each tick | 4.050 us | 6.050 | 18.000 | 36.044 |

Read the last column twice. The body that cannot allocate has a worst case two
hundred and forty times its own median, and a **worse** worst case than the body
that allocates on every single pass.

That 60 microseconds is not in the code. It is a scheduling decision, a page
fault, an interrupt: the machine deciding to do something else for a moment.

So there is no timing threshold that separates "this allocated" from "the kernel
was busy". The thing you are listening for is a hundred times quieter than the
room.

(Those figures are from this suite, which builds unoptimised. With optimisation
the same two bodies measured 0.150 and 0.400 microseconds. The gap narrows; the
argument does not change, because the tail is the same tail.)

### So count instead

An allocation is a discrete event, so count the events:

```cpp
#include <rc/test/leak_check.hpp>

rc::test::AllocationCount count;
loop_body();
RC_CHECK(count.none());
```

Deterministic, identical on every toolchain, and it fails at the line that
allocated rather than somewhere downstream. It is a property you assert once and
keep, instead of an investigation you repeat.

Note that this is a **different question from a leak**. A loop allocating and
freeing a hundred blocks per tick leaks nothing: the live counters come back to
exactly where they started, and `LeakCheck` reports a clean bill. It is still
unfit to run at 500 Hz. Two counters, two questions.

### What the count actually finds

Rarely a call to `new`. Here is one pass of each of five loop bodies:

| loop body | allocations |
|---|---|
| a fixed array | 0 |
| a reserved `vector<double>`, cleared and refilled | 0 |
| a reserved `vector<string>`, the same thing | **64** |
| the same, with strings that fit inside the object | 0 |
| an unreserved vector grown to 4096 | 13 |

**`reserve` reserves the vector and not the strings.** Room for sixty four
string objects was made once; each of those then went to the heap for its
characters, every tick, from code that has `reserve` written in it and looks
finished.

### The boundary you cannot see

Whether a string allocates depends on its length, and the limit is an
implementation detail:

- **15 characters** on libstdc++ and on the Microsoft library,
- **22** on libc++.

`std::function` has a boundary of the same kind. A lambda capturing one double
fits inside it on every library this curriculum builds against; three doubles
does not on all of them.

So a loop that is allocation free on a laptop can allocate on a robot with a
different standard library, from source nobody changed. That is why this
lesson's suite **searches** for the limit rather than stating it:

```cpp
for (std::size_t length = 1; length < 128; ++length) {
  AllocationCount count;
  std::string subject(length, 'x');
  if (count.total() > 0) return length - 1;
}
```

The test reports what it found on the machine it ran on, which is the only
machine whose answer matters.

### Storage that was built once

What replaces the allocation is not cleverness, it is deciding earlier.

```cpp
rc::rt::Pool<Reading, 8> pool;

Reading* item = pool.acquire();
if (item == nullptr) { ++dropped; return; }
// ...
pool.release(item);
```

Eight slots constructed once, an index handed out on request, nothing from the
heap afterwards. A million acquires and releases in the test move the allocation
counter by zero.

Two things it must do, and they are the same idea twice.

**Refuse rather than grow.** A pool that allocates when it runs out has given up
the only property it had, and it does so at the worst moment, which is precisely
when the system is already late. It counts the refusal instead, and
`exhaustions()` staying at zero is a fact about capacity worth reporting rather
than only asserting: a pool sized for last year's worst case is the first thing
a new feature outgrows.

**Refuse what it did not hand out.** Releasing the same slot twice puts one
index on the free list twice, and the next two callers get the same address.
Two owners then write to one object, each believing it is theirs, and the damage
appears somewhere else entirely, long after the mistake. A bounds check and a
`used_` flag turn that into a counter at the line that was wrong.

## Build It

Implement the constructor, `acquire` and `release` in `exercise/solution.hpp`.

```
rcpp verify 07-04
```

The suite counts what five loop bodies allocate, hunts for this toolchain's
small-string limit, times two bodies to show why timing is the wrong tool, and
then exhausts a pool and hands it things it never owned.

## Use It

**Assert the count in CI, on every toolchain you ship to.** The number that
matters is the one from the robot's standard library, and reasoning cannot
supply it.

**Prefer a fixed `char` array to `std::string` in the hot path.** `char name[24]`
allocates never, on every library, and its size is visible in your source
instead of in somebody else's internals.

**Take callables by template parameter inside a loop.** `std::function` is the
right tool at a boundary crossed once and the wrong one every tick.

**Size the pool by measurement.** Run the worst case you can arrange, read
`in_use()` at its peak, add a margin you can defend, and write down which run
the number came from.

## What Breaks First

- **Hunting an allocation with a stopwatch.** See `E-RT-0002`.
- **A container that looks preallocated and is not.** See `E-RT-0003`.
- **A pool that runs out quietly.** See `E-RT-0004`.

## Ship It

`Pool` joins `rc::rt` beside the lock-free queue and the histogram, and
`rc::test::AllocationCount` joins the leak checker. The loops built from phase 14
onward can now be proved not to allocate, rather than believed not to.
