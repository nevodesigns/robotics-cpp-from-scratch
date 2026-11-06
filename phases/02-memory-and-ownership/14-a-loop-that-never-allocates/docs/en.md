# A Loop That Never Allocates: A Fixed Pool, and What It Costs to Lend and Take Back

> A loop that has to run on time cannot ask the heap for memory, because how long the heap takes is not
> up to the loop. A pool makes the memory first and lends it.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 02-02, 02-03

## The Problem

Lesson 02-02 found leaks, and lesson 02-03 made them impossible by having an object free what it owns. A control loop that makes a message each pass with `new` and frees it at the end of the pass does not leak.

It still allocates, and the loop is hostage to the heap: usually `new` is quick, and now and then it is not, and what decides which is everything that ran before. A loop that must finish within two milliseconds, five hundred
times a second, cannot have a step whose time it does not control.

## The Concept

### Make them first

A **pool** holds a fixed number of messages from the start. Lending one is taking an index off a stack of free indices, and taking one back is putting it on:

```cpp
const int index = free_[--free_count_];     // lend
free_[free_count_++] = index;               // take back
```

Neither touches the heap. How long they take does not depend on what else has happened.

The suite counts every heap allocation the program makes. 100000 cycles of lending a message and giving it back made **0** allocations through the pool and **100000** through `new`. See `E-MEM-0023`.

### A pool has a size

All sixty-four messages can be out at once. The sixty-fifth request gets `nullptr`, and the caller decides what to do: drop the reading, wait or raise a fault. The pool does not decide for it, and it does not lend a message that is
already in use, which is what a pool that wraps round does. After one release the next acquire gives that slot and the request after it is refused again. See `E-MEM-0024`.

### Taking back is checked

A pool that believes its callers puts a slot on the free stack every time it is told to, and a slot returned twice is lent twice. `release` refuses four things:

- a null pointer;
- an address outside the pool, compared with `std::less` because comparing unrelated pointers with `<` has no promised answer;
- an address inside a slot and not at its start;
- a slot that is not out.

The suite tried the first three and a second release of a returned message, and after that 64 lends still gave **64 different** messages and the 65th was refused. See `E-MEM-0025`.

### Clean and warm

A lent message is reset to a clean one, so a borrower never sees the numbers of the last. And the slot most recently returned is the next one lent, since it is on top of the stack. That slot was used a moment ago, so its memory is
the most likely to be in the processor's cache. The suite checks that the same slot came back, with value 0 and id 0.

## Build It

`acquire` and `release` in `exercise/solution.hpp`. The constructor, `in_use` and `capacity` are given.

```
rcpp verify 02-14
```

The test replaces the global allocation functions with counting ones through `rc/test/leak_check.hpp`. That is the one place in this curriculum where counting every allocation is the aim.

## Use It

`rc/core/pool.hpp` joins `rc::core`.

```cpp
MessagePool pool;
Message* m = pool.acquire();
if (m == nullptr) { /* all out: the caller decides */ }
// ... use it ...
pool.release(m);
```

**What this does not do.** It is one type of object and one fixed size. It does not run a destructor, so it is for plain data, and it is not safe to use from two threads (phase 07 is about that). It does not say who is holding a message, so a message
that is never given back is gone from the pool for good, and the pool shows that as `in_use` rising and nothing else. A message is a raw pointer, so nothing stops a caller using one after releasing it: that is lesson 02-07's problem, and it is a reason to wrap the lend in an
object that returns it, as lesson 02-03 did for other resources.

## What Breaks First

- **`new` in the loop.** Correct and not on time. See `E-MEM-0023`.
- **A pool that lends when it is empty.** Two users, one message. See `E-MEM-0024`.
- **A release that is not checked.** A slot lent twice. See `E-MEM-0025`.

## Ship It

`rc/core/pool.hpp` holds `Message` and `MessagePool`.

Lesson 07-04 asks the same question about a whole loop, and measures it. This is how to make the answer "no allocations".
