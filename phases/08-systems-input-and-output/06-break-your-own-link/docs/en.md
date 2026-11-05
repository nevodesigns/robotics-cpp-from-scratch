# Break Your Own Link: What One Fault Costs

> A link that has only met a perfect port has not been tested. Build the port
> that breaks, and find out what a single lost byte actually costs.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 08-04

## The Problem

Lesson 08-04 built a link that survives partial reads, partial writes and
silence, and its tests pass. Every one of them ran on a fake port that did
exactly what it was told.

A real cable does not. A connector works loose, a motor starts and the line
picks up noise, a buffer overruns, and a byte goes missing or one bit of it is
turned over. Nothing in the 08-04 suite ever did either of those things, so a
question that matters has no answer yet: **when the link loses one byte, what
does that cost?**

A guess would be one message. It is not one, and you will measure what it is.

## The Concept

### Break it on purpose, the same way every time

`FaultPort` wraps any `BytePort` and damages what passes through it:

| fault | what it does |
|---|---|
| drop | a byte vanishes, and the next one takes its place |
| flip | one bit of a byte is inverted |
| read chunk | a read returns at most this many bytes, so frames arrive in pieces |
| write chunk | a write accepts at most this many bytes, which is a short write |

The first two are the damage. The last two are not damage but behaviour a real
port has all the time, and they are here so that one decorator can reproduce all
of it.

Everything is counted, because a fault you cannot count you cannot price.

### The same seed, the same faults

A test that injects faults has to be repeatable, or a failure cannot be
investigated. The generator is a five line `xorshift32`, written out by hand
instead of taken from `<random>`, and the rates are **parts per million** so that
each decision is an integer comparison.

That is not fussiness. The standard fixes the random *engines* but not the
*distributions*, so the same seed through `uniform_real_distribution` is allowed
to give different values on gcc, clang and MSVC. With an integer generator and an
integer comparison, a number measured on this machine holds on the others. That
is what lets the next section be a test rather than a story. See `E-IO-0020`.

### What one fault costs

Ten thousand frames of ten bytes each go through `FaultPort` into the `Link` from
lesson 08-04. The numbers are what the test prints.

| damage | faults | delivered | lost | lost per fault |
|---|---|---|---|---|
| none | 0 | 10000 | 0 | 0.00 |
| 1 byte in 500 dropped | 226 | 9573 | 427 | 1.89 |
| 1 bit in 500 flipped | 226 | 9759 | 241 | 1.07 |
| 1 byte in 100 dropped | 1103 | 8100 | 1900 | 1.72 |
| 1 bit in 100 flipped | 1113 | 8880 | 1120 | 1.01 |

**A flipped bit loses one frame. A dropped byte loses nearly two.**

The reason is in the parser. A frame says how long it is, and the parser believes
it. When a byte vanishes from the middle of one, the parser is still waiting for
the bytes it was promised, so it takes them from the start of the *next* frame.
The damaged frame fails its checksum, and the following one has lost its
beginning, so it is gone too. A flipped bit leaves the length alone, so the
checksum rejects exactly that frame and the stream stays in step.

Why under two and not exactly two: a byte dropped from the *start* byte of a frame
costs only that frame, and at the higher rate two drops sometimes land in the same
pair of frames. See `E-IO-0019`.

### A checksum that passes is not proof

One more number from the same run, and it is the one to remember.

At one dropped byte in a hundred, **873 misaligned frames failed the checksum and
3 passed it**, with the wrong content.

Each misaligned frame is, to an eight bit checksum, arbitrary data compared with
an arbitrary byte, and arbitrary data matches one time in 256. 876 divided by 256
is 3.4. Three got through, and each of those is a message the link reports as
valid and the robot acts on.

A flipped bit does not do this: **CRC-8 detects every single bit error**, and
1113 flips produced no wrong frame at all. A test that only flips bits would
conclude the checksum is perfect. See `E-IO-0021`.

## Build It

`MemoryPort` is given to you: a queue of bytes the device end feeds and the code
under test reads, which records what is written. Implement `FaultPort` in
`exercise/solution.hpp`.

- `read` takes bytes from the inner port **one at a time**, so a dropped byte can
  be replaced by the next and the count returned is honest. Count every byte seen,
  then decide whether to drop it, then whether to flip a bit of it. Never return
  more than `read_chunk` bytes.
- `write` offers the inner port at most `write_chunk` bytes. The return value is
  how many it took.
- A generator of your own, `xorshift32`, with a seed of zero turned into something
  else, because a state of zero never leaves zero.

```
rcpp verify 08-06
```

The suite checks the clean path first, then that the drop rate and the flip rate
are about what was asked and that a flip changes exactly one bit, then that a seed
repeats. The last two tests are the measurements above, run through the real
`Link`.

## Use It

`FaultPort` joins `rc::io`, and every later lesson in this phase puts it under
something. Wrapping a port costs one line:

```cpp
rc::io::MemoryPort device;
rc::io::FaultOptions bad;
bad.seed = 7;
bad.drop_per_million = 10000;
rc::io::FaultPort noisy(device, bad);
rc::io::Link link(noisy, storage, timeout);
```

On real hardware the same wrapper goes around a `SerialPort`, which makes a
perfect bench cable misbehave on request. A fault you can switch on is a fault you
can test for.

The trade off is that these faults are *independent*, each byte unlucky on its
own. Real noise comes in bursts: a motor starting takes out thirty bytes in a
row, then nothing for a second. Bursts are kinder to a checksum, which is
designed for them, and harder on framing. Independent faults are the right place
to start because they are the ones you can count, and they are not the whole
picture.

## What Breaks First

- **A lost byte priced at one message.** It costs nearly two, because the parser
  takes what it was promised from the next frame. See `E-IO-0019`.
- **Faults that differ from run to run.** A seed from the clock, or a distribution
  the standard does not pin down, makes a failure impossible to repeat. See
  `E-IO-0020`.
- **A checksum trusted because it passed.** One in 256 misaligned frames passes an
  eight bit check, and a test that only flips bits never sees it. See `E-IO-0021`.

## Ship It

`rc/io/fault_port.hpp` holds `MemoryPort`, `FaultOptions` and `FaultPort`.

It also leaves two questions for the next lesson, both measured here. A parser
that loses nearly two frames for one lost byte can do better by rescanning what it
threw away. And a check that lets one misaligned frame in 256 through can be
widened. Lesson 08-07 does both and measures what each buys.
