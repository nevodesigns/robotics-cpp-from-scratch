# The Newest Reading or All of Them: Acting on the Past

> A loop that reads one message a tick looks finished and is not. Find out how old
> the data it acts on really is, and choose what to do with the ones you skip.

**Type:** Build
**Time:** about 150 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 08-08

## The Problem

Lesson 08-08 worked out how long a reader may stop reading before the buffer fills.
This one is about a reader that never stops and still falls behind.

```cpp
if (link.poll(now)) act(link.message());     // once a tick, forever
```

`poll` returns one message. If the device sends faster than the loop runs, each
call takes one and leaves the rest. No call is ever slow and nothing is ever
reported, so the loop looks healthy while the queue behind it grows to the size of
the driver's buffer.

Then comes the part that makes this expensive. A full buffer keeps what it already
holds and drops what arrives. The data at the front of the queue is the oldest
there is, and the loop acts on it while the device's current readings fall on the
floor.

## The Concept

### How old is the data, measured

A 500 Hz device, a 100 Hz control loop, and a 4096 byte buffer, for twenty seconds
on a virtual clock. Each message carries the time it was sent, so the loop can say
how old the message it just acted on was.

| policy | acted on | mean age | worst age | bytes lost |
|---|---|---|---|---|
| one message per tick | 2000 | 2472 ms | 3741 ms | 76058 |
| every message, 3 ms each, 3 a tick | 5999 | 962 ms | 1245 ms | 27692 |
| **drain, act on the newest** | 2000 | **1 ms** | **1 ms** | **0** |

The first row acts on data **two and a half seconds old, on average**, and a
robot driven by it is steering by the past.

The worst age is not an accident of the run. It is arithmetic: 4096 bytes is 372
frames of eleven, and 372 frames drained at 100 a second takes 3.72 seconds. The
measured worst was 3741 ms. At 300 a second it is 1.24 seconds, and the measured
worst was 1245 ms.

The second row is what most people reach for: handle every message that is waiting.
It is better and still wrong. A tick has ten milliseconds in it, and if acting on a
message takes three, the loop can handle three a tick, 300 a second, against 500
sent. It falls behind more slowly. See `E-IO-0028`.

The third row does the one thing the first two do not: it drains everything that is
waiting and acts on the newest. The queue is empty after every tick, so the data is
never older than one frame, and nothing overflows because nothing waits.

### The newest is a policy, and it has a domain

Skipping is only safe when a skipped message was made redundant by a later one.

A **state** message says what is true now: a position, a temperature, a joint angle.
A newer one replaces an older one, and skipping loses nothing.

A **change** message says how much happened since the last: an encoder delta, a
gyro increment, a step. Every one adds to a total, and a skipped one is a piece of
the total thrown away.

A thousand messages each saying +1, drained a hundred at a time:

| | total |
|---|---|
| a loop that integrates the newest of each drain | 10 |
| what the skipped messages carried | 990 |
| the truth | 1000 |

The error is 99 percent and it varies with load, which sends the search for a bug
towards the CPU instead of the policy. See `E-IO-0029`.

The better fix is in the message. Make a change message carry a **running total**
and it becomes a state: the newest total is the sum, and a lost message costs
nothing. Where the format cannot change, handle every message and count the skipped
ones.

### The drain needs a limit

`while (link.poll(now))` runs until nothing is waiting. If the device can send as
fast as the loop reads, nothing is ever waiting, and the call never returns. The
loop has a deadline and a call that takes as long as the device likes cannot be in
it.

So `drain_latest` takes a limit, and returns `more` when it stopped because of it.
Measured, with two hundred thousand frames waiting and a limit of 64, the call took
64 and returned, with almost everything else still waiting.

The newest message is **copied out**, because the `Link`'s own view is only valid
until the next poll. A message bigger than the destination is counted in
`oversized` and not copied, so it cannot write past the end. See `E-IO-0030`.

## Build It

Implement `drain_latest` in `exercise/solution.hpp`. The result type is given.

The rules are in the comment: poll until nothing is left or the limit is reached,
copy each message as it arrives, count a replaced message in `skipped`, count a
message that does not fit in `oversized`, and set `more` only when you stopped
because of the limit.

```
rcpp verify 08-09
```

The first six tests are the function: the newest of several, nothing waiting, a
damaged frame in between, the limit and the next call, a device that never stops,
and a buffer that is too small. The last two are the tables above.

## Use It

`rc/io/drain.hpp` joins `rc::io`. A control loop with a link reads:

```cpp
std::uint8_t newest[64];
const rc::io::Drained d = rc::io::drain_latest(link, now, newest, 1000);
if (d.got) act(newest, d.size);
if (d.skipped > 0) skipped_total += d.skipped;    // report it
```

The honest cost is that you decided to throw messages away. For a state that is
right, and `skipped` is a measure of how much faster the device is than the loop,
which is worth knowing: if it is zero the loop is faster than it needs to be, and
if it is large the loop is the thing being sampled.

For a change message, do not use this. Either carry totals, or run a reader that
handles every message on its own thread, which is the next lesson.

## What Breaks First

- **One message a tick.** The queue grows to the buffer and the loop acts on the
  past. See `E-IO-0028`.
- **The newest of a stream of changes.** The skipped messages carried the missing
  sum. See `E-IO-0029`.
- **A drain with no limit, or a copy with no size check.** The first holds the loop
  for as long as the device likes, the second writes past a buffer. See
  `E-IO-0030`.

## Ship It

`rc/io/drain.hpp` holds `Drained` and `drain_latest`.

The loop is now never behind. It is still the loop that reads, though, so whatever
else it does is time the buffer is filling. The next lesson takes the reading off
it.
