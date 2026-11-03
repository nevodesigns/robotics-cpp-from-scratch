# The Pointer That Was Right Yesterday: Invalidation, and Borrowing From Something That Moves

> Erasing one reading from the middle of a buffer left every pointer valid and
> every one of them pointing at a different number. No sanitiser reports it,
> because nothing was freed.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 01-11, 02-01

## The Problem

Lesson 01-11 ended on a rule: **borrow what you read, return what you make.**
It is the right default, and this lesson is its price.

Lesson 02-01 already had you hand back a pointer into the caller's array. The
question neither lesson asked is what happens to that pointer when the caller's
array changes underneath it.

The answer is three failures, and they get progressively harder to find.

## The Concept

### One: the buffer moves

A `std::vector` keeps its elements in a single buffer. When it runs out of room
it allocates a bigger one, copies everything across, and frees the old one.
Every pointer, reference, iterator and span that referred to the old buffer now
refers to memory that has been handed back.

Measured, with three readings and no slack:

| | capacity | buffer |
|---|---|---|
| before | 3 | where it was |
| after one `push_back` | 6 | somewhere else |

One insertion. That is all it takes.

### Two: it does not happen every time

This is why it reaches a release. Twenty insertions into an empty vector:

| size | capacity | buffer moved |
|---|---|---|
| 1 | 1 | yes |
| 2 | 2 | yes |
| 3 | 4 | yes |
| 4 | 4 | no |
| 5 | 8 | yes |
| 6, 7, 8 | 8 | no |
| 9 | 16 | yes |
| 17 | 32 | yes |

**Six reallocations out of twenty insertions.** The other fourteen left every
borrowed pointer perfectly valid. So code that borrows across an insertion works
most of the time, passes the test somebody wrote, and fails the day a sample
buffer gets one element longer.

The growth factor above is your standard library's choice and not a rule of the
language, which is why the suite prints it rather than asserting it.

### Three: the one with no diagnostic at all

`erase` does **not** reallocate. It shifts every element after the erased one
down by one, in the same buffer.

Measured on five readings, with room reserved so nothing could possibly
reallocate:

| | index 2 holds | buffer |
|---|---|---|
| before erasing index 1 | 30 | the original |
| after erasing index 1 | **40** | the same, unchanged |

Read that again. A pointer saved to `&samples[2]` is still pointing at live
memory, inside the same buffer, owned by the same container. It is pointing at a
different reading.

Nothing was freed, so **there is no use after free**. Every tool that finds the
first failure finds nothing here. The address sanitiser is clean. The program
carries on with a number that looks entirely reasonable: a fault attributed to
the wrong motor, a waypoint on the wrong leg of a route.

A live pointer to the wrong element is worse than a dangling one that crashes.

And note what does **not** help: an index. An index into a vector is invalidated
by an erase exactly as a pointer is, and for the same reason. An index saves you
from reallocation and not from this.

### Which containers move their elements

Measured over 100 insertions each:

| container | first element |
|---|---|
| `std::vector` | moved |
| `std::deque` | unchanged |
| `std::list` | unchanged |

The vector is the one that moves and it is still the one to reach for first. The
other two buy element stability with a pointer chase per element, and phase 07
measures what that costs when you are walking a buffer at 500 Hz.

### Asking instead of finding out

The artifact is one question, asked at the only moment it has an answer:

```cpp
if (rc::core::would_reallocate(samples, 1)) { /* pointers are about to die */ }
```

It has to be asked **before** the insertion, because afterwards every pointer
that could have told you is already stale. It is computed from the size and the
capacity, so it does not depend on observing anything.

Checked against what actually happened on every one of a thousand insertions,
eleven of which reallocated: no prediction wrong.

And the usual answer, which is to make the question not arise:

```cpp
rc::core::reserve_for(samples, expected_count);
```

Measured: twenty insertions after reserving for twenty moved the buffer **zero**
times. This is the right answer for a sample buffer whose length you know, and
it is the same thing that keeps allocation out of a control loop.

### Why comparing addresses is not enough

There is a trap in writing the detector itself.

An allocator is free to hand a freed block straight back for the next request of
the same size, and frequently does, because it is the cheapest thing it can do.
So a vector that reallocated may land at the address it just released: the
elements were copied, every borrowed pointer was invalidated, and the address is
identical.

```cpp
const void* before = samples.data();
step();
assert(samples.data() == before);     // can pass anyway
```

So `StorageId` carries the capacity as well, and `same_storage` compares both. A
reallocation that produced the same capacity would have had no reason to happen.

Better still, predict rather than detect. A prediction cannot be fooled by an
address being reused.

## Build It

Implement `storage_of`, `same_storage`, `would_reallocate` and `reserve_for` in
`exercise/solution.hpp`.

```
rcpp verify 02-07
```

The suite grows a vector one reading at a time and checks the prediction against
what happened on every single insertion, reserves and shows nothing moves,
erases from the middle with the storage pinned, compares three containers, and
then runs the prediction over a thousand insertions.

Nothing in that suite reads through a pointer after the operation that
invalidated it. Every claim is made by comparing storage identities and by
reading the container, which is how it demonstrates the bug without committing
it.

## Use It

**Reserve, then borrow freely.** If you know roughly how many readings are
coming, say so once and the question disappears.

**Ask before inserting** when you cannot. `would_reallocate` is the only form of
the question that has an answer.

**Never hold a pointer, reference, iterator, span or index across a
modification of the container.** Take it, use it, drop it, inside one operation.

**Find elements by identity, not by position**, when they can be removed. Store
the motor number, not where that motor was in the list this cycle.

**Collect removals and apply them once.** `std::remove_if` followed by a single
`erase` does the whole pass with one shift. A loop that erases while iterating
is this bug with extra steps.

**Compare capacity, not just addresses,** in any test that claims nothing was
reallocated.

## What Breaks First

- **A pointer into a vector that grew.** See `E-MEM-0011`.
- **A live pointer to the wrong element after an erase.** See `E-MEM-0012`.
- **A storage comparison that compared only the address.** See `E-MEM-0013`.

## Ship It

`StorageId`, `storage_of`, `same_storage`, `would_reallocate` and `reserve_for`
join `rc::core`. Phase 02 now covers the whole arc of owning and borrowing: who
frees it, who may copy it, who may move it, who shares it, and what happens to
everyone who was looking at it when it changed.
