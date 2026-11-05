# A Device Faster Than You Read: Losses You Can Count and Budget For

> A serial buffer fills while nobody is looking and drops what arrives next
> without saying a word. Work out how long you may stop reading, and count what
> you lost.

**Type:** Build
**Time:** about 150 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 08-07

## The Problem

Everything so far was about damage: bytes that arrived wrong, or went missing on
the wire. This lesson is about bytes that arrived perfectly and were lost anyway,
because nobody was there to take them.

A robot's reading loop is not the only thing the program does. It saves a file,
redraws a window, writes a log, and for a while it is not reading. The device does
not know and does not stop. It sends into a buffer of fixed size in the driver,
and when that is full the **new** bytes are dropped, without an exception, without
a return code, and usually without a counter anyone looks at.

The symptom is a burst of missing messages that lines up with the program being
busy, on a cable with no checksum failures at all. This lesson puts a number on
how long "busy" is allowed to last, and builds the counter that tells you when it
was longer.

## The Concept

### How fast does a line fill a buffer?

A serial line carries bits, and a byte is not eight of them. It is a start bit,
eight data bits and a stop bit:

```text
115200 baud / 10 bits a byte = 11520 bytes a second     (not 14400)
```

Using eight overstates the rate by a quarter, which is the wrong direction to be
wrong in: it makes the budget look smaller than it is, and you add a margin you
did not need and miss the one you did. See `E-IO-0027`.

A buffer of 4096 bytes, a common size for a serial driver and the one these tests use, takes
`4096 / 11520 = 0.356` seconds to fill from empty at full speed. A third of a
second is not long. Most programs pause longer than that sometimes.

A device rarely sends at full line speed. Five hundred frames a second of eleven
bytes is 5500 bytes a second, and the same buffer then lasts 0.745 seconds. That
figure is the **stall budget**: the longest the reader may go without reading
before the device starts losing bytes.

```cpp
const double budget = stall_budget(4096, 500.0 * 11.0);   // 0.745 s
```

### The budget, measured

The tests run this on a virtual clock, so the answer is the same on every run:
a device sending 500 frames a second into a 4096 byte buffer, read by a loop that
comes round every so many milliseconds, for twenty seconds.

| read every | sent | delivered | lost | predicted | overflow bytes |
|---|---|---|---|---|---|
| 100 ms | 10100 | 10100 | 0 | 0 | 0 |
| 300 ms | 10100 | 10100 | 0 | 0 | 0 |
| 700 ms | 10100 | 10100 | 0 | 0 | 0 |
| 800 ms | 10100 | 9374 | 726 | 691 | 7600 |
| 1000 ms | 10100 | 7520 | 2580 | 2553 | 28080 |
| 2000 ms | 10100 | 3810 | 6290 | 6276 | 69040 |

Inside the 0.745 second budget, nothing is lost. Past it, the loss is what the
arithmetic said: the part of each stall beyond the budget, times the rate the
device sends, times the number of stalls. The prediction is within a few percent.

It is a cliff and not a slope. At 700 ms the loss is zero, and at 800 ms, 100 ms
later, it is 726. A program that is fine for a month and then loses data when a
disk gets slow is not mysterious, it has just crossed a line it could have
computed.

### Counting what you lost

The overflow is invisible to the driver and invisible to the checksum, because
the frames that did arrive are intact. The only way to know a frame is missing is
for the frames to say which they are. A message that carries a **sequence number**
lets the receiver say how many did not arrive.

`SequenceTracker` takes each number and says what it learned:

- in order: nothing lost
- ahead of the expected one by *k*: *k* messages are missing
- the same as the last one: a duplicate
- behind it: a late arrival, and neither of those moves the expectation, or the
  next message in order would be counted as a loss

**The arithmetic has to be done in the counter's own width.** A `uint8_t`
subtraction is done in `int` after promotion, so `2 - 254` is `-252`, not `4`. In
order the two are equal and the bug cannot show, which is why a test on a clean
stream passes. It appears when a loss straddles the wrap: measured, with messages
253 to 256 lost, the `int` version counts **0** lost and calls 252 following
messages late, until the stream has gone all the way round to catch up. Taking the
difference in the counter's type counts 4. See `E-IO-0025`.

### How wide must the counter be?

A counter of N bits can see a gap of at most `2^(N-1) - 1`. Past that, a gap looks
like a message going backwards. And a gap of exactly one turn of the counter looks
like nothing at all.

| counter | widest gap it can see |
|---|---|
| 8 bits | 127 |
| 16 bits | 32767 |

The same two second stall, counted both ways:

| | frames lost |
|---|---|
| the truth, and the sixteen bit counter | 6290 |
| the eight bit counter | **1170** |

Each stall lost about 629 frames. An eight bit counter can only say 629 modulo
256, which is 117, and for a stall whose remainder is 128 or more it would say
nothing. The number it gives is small and plausible, and it comes from the device
whose stalls are the long ones. See `E-IO-0026`.

Choose the width from the longest stall you must account for: frames a second
times seconds. At 500 frames a second a one second stall is 500 frames, which
needs sixteen bits.

## Build It

`BoundedPort` is given: a buffer with a fixed size that drops what does not fit.
Implement three things in `exercise/solution.hpp`:

- `wire_bytes_per_second(baud, bits_per_byte)`, with ten bits to a byte by default.
- `stall_budget(buffer_bytes, bytes_per_second)`, which is infinity for a device
  that sends nothing.
- `SequenceTracker<T>`, whose `accept` returns a `Verdict`, and `widest_gap()`.

The comments in `accept` carry the two traps: the width of the subtraction, and
that a duplicate and a late arrival must not move the expectation.

```
rcpp verify 08-08
```

The tests cover the arithmetic, the tracker on hand built streams including the
wrap and the limits of the counter, and then the twenty second runs in the tables
above.

## Use It

`rc/io/sequence.hpp` holds all of it. For a real device the steps are:

1. Put a sequence number in every message. If the device's firmware does not, add
   one in the firmware, because it is the only place it can come from.
2. Work out the budget for the buffer you have, and the stall the program can
   cause. If the second is longer than the first, move the reading somewhere else.
3. Count loss with a `SequenceTracker`, and report it, because a loss you do not
   report is a loss you will discover from the robot's behaviour.

The honest limit is that a tracker counts what it can see. A stall that loses a
whole counter's worth of messages is invisible to it, and a device that reboots
and restarts its counter at zero looks like a late arrival, or a loss, depending
on where it was. The tracker cannot tell a reboot from a gap, and a link that cares
needs the device to say it has restarted.

## What Breaks First

- **A sequence number subtracted in `int`.** A loss across the wrap is not counted
  and the stream is called late for a whole turn. See `E-IO-0025`.
- **A counter too narrow for the stall.** It reports a small plausible number, or
  nothing. See `E-IO-0026`.
- **A buffer that filled while the program was busy.** A byte is ten bits, the
  budget is the buffer over the rate, and crossing it is a cliff. See `E-IO-0027`.

## Ship It

`rc/io/sequence.hpp` holds `BoundedPort`, `wire_bytes_per_second`, `stall_budget`,
`Verdict` and `SequenceTracker`.

The budget says how long the reader may stop. The next lesson asks a question the
budget does not: when the reader does come back and finds many messages waiting,
which should it act on? A robot that handles every one is handling the past.
