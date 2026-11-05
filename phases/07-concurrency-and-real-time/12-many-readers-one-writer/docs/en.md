# Many Readers, One Writer: Why the Mutex Gets Slower and shared_mutex Starves the Writer

> Eight readers on a mutex got a fifth of what one reader got. The lock made for
> many readers starved the writer to 92 writes out of 14,000.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 07-01, 07-02, 07-11, 03-07

## The Problem

Lesson 07-01 shared a reading between threads with a mutex. That is correct, and it is
the right answer for a value read occasionally.

A robot has another kind: one pose, written by one thread at a kilohertz, read by a dozen.
The localiser writes it, the planner, the controller, the display, the logger and the
watchdog all read it. The obvious tools for it do not do what their descriptions promise.

## The Concept

### Measured, three ways

A 32 byte pose whose four fields are always equal, so a torn read could be seen. A writer
that writes every 20 microseconds. 300 ms. Reads per millisecond across all readers, gcc 11:

| readers | 1 | 4 | 8 |
| --- | --- | --- | --- |
| mutex | 40774 | 10093 | 8503 |
| shared_mutex | 37521 | 5786 | 6351 |
| seqlock | 204873 | 790348 | 914603 |

A second run gave the same shape: seqlock 127640, 747010 and 991519.

And what the writer managed, of about 14,000 writes it wanted, with 8 readers:

| | writes completed |
| --- | --- |
| mutex | 13019 |
| shared_mutex | **92** (71 in a second run) |
| seqlock | 14585 |

Three findings, not one.

**The mutex gets slower as readers are added.** Every read takes the same lock, so readers
are serialised and the lock's cache line moves between cores. Eight readers got a fifth of
what one did. `E-THREAD-0018`.

**`shared_mutex`, built for exactly this, was worse, and starved the writer.** Readers kept
arriving before the last one left, so there was never a moment with none, and the writer
waited: 92 writes in 300 ms. It was also slower for the readers than the plain mutex.
Whether a waiting writer is preferred is up to the implementation. `E-THREAD-0019`.

**The seqlock scaled with readers and never made the writer wait.** No run of any of the
three produced a torn read.

### How a seqlock works

A counter: odd while a write is in progress, even when none is.

```
writer:  counter becomes odd  ->  write the data  ->  counter becomes even
reader:  read the counter  ->  read the data  ->  read the counter again
         trust the data only if the first was even and the two are equal
         otherwise throw it away and try again
```

Readers take no lock and write nothing, so they cannot slow each other down or the writer. The
price is the retry, and it is counted. Forced on one thread, a write landing between the data read
and the second counter read:

```
  a write landed in the middle of a read:
    hook fired      yes
    value returned  2 (the old one was 1, the new one is 2)
    retries         1
```

and a reader arriving while a write was halfway done waited, retried 63 times, and returned the
new value whole. Both use lesson 07-11's race points, so neither depends on timing.

### The part that has to be done carefully

A seqlock reader copies the data while a writer may be changing it. If the data is a plain struct
copied with `memcpy`, that is a **data race** in C++, whatever the hardware does.

Experiment, method stated: 200000 writes against a reader, on x86, with the data held two ways.

| data held as | torn reads | gcc 11 tsan | clang 14 tsan |
| --- | --- | --- | --- |
| a plain struct, `memcpy` | **0** | 0 reports in 5 runs | **20 reports** in 5 runs |
| an array of atomic words | 0 | 0 | 0 |

It worked every time, and clang's thread sanitizer was right to say it is a race. GCC 11's did not
see it, because it does not model the standalone fences the algorithm is built on. A clean report
from the tool that misses it proves nothing.

So the data is an array of `std::atomic<unsigned long long>`, written and read with `relaxed`
operations between the counter's fences. That is legal C++, costs nothing extra on x86, and both
sanitizers reported nothing on it in 12 runs. `E-TSAN-0004`.

And so the type is limited to those whose bytes are their value:

```cpp
static_assert(std::is_trivially_copyable<T>::value, "...");
```

`Seqlock<std::string>` does not compile, which `checks/` proves. A string's bytes are a pointer, and
a copy taken mid-write could hold a pointer to memory the writer has just freed.

### What it does not do

- **One writer.** Two would interleave their odd and even steps and corrupt the counter.
- **Readers can starve.** 50000 writes with no pause against 3 readers gave 21117 successful reads
  and 501504 retries, about 24 per read. It suits a value that changes in bursts, not one rewritten
  as fast as possible. Spaced 20 microseconds apart, a few retries per write.
- **Readers get a copy,** so the value has to be small.

## Build It

Two methods.

`write`: read the counter and store it plus one, which is odd. A release fence. Copy the value into
zeroed words with `memcpy` and store each into `data_` with `relaxed`. The race point. Store the
counter plus two with `release`.

`read`: loop. Load the counter with `acquire`; if it is odd, count a retry and go round. Load each
word `relaxed`, an acquire fence, the race point, and if the counter is unchanged, `memcpy` the words
into a `T` and return it. Otherwise count a retry and go round.

The stubs write nothing and return an empty value without looping. The suite checks that the last
value written is the one read and that values advance, because a stub that returns zeros would pass
"never a mixed value".

## Use It

```cpp
#include <rc/rt/seqlock.hpp>

rc::rt::Seqlock<Pose> pose;
pose.write(estimate);                 // the one localiser thread
const Pose now = pose.read();         // the planner, the controller, the display
```

Log `retries()` against reads. A rising ratio means the writer is going too fast for the readers, or
there are too many of them, and that is the number that tells you when the choice stops being right.

Keep the mutex where the value is large, has several writers, or is read rarely: there it is simpler
and costs nothing. Measure the readers you will actually have.

## What Breaks First

`E-THREAD-0018` is the mutex that got slower as readers were added.

`E-THREAD-0019` is the `shared_mutex` that starved its writer.

`E-TSAN-0004` is the seqlock that works, is a data race, and only one sanitizer says so.

## Ship It

`rc/rt/seqlock.hpp`, in `namespace rc::rt`.

From here on, a value published by one thread at a high rate and read by several in this curriculum
goes through a seqlock, and its retries are counted.
