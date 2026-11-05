# A Reader Thread of Its Own: Nothing Else Can Hold the Reading

> The stall budget is spent by whatever shares the reading thread. Give the reading
> a thread that does nothing else, and only its own code can spend it.

**Type:** Build
**Time:** about 180 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 08-09, 07-02, 07-07, 07-12, 07-14

## The Problem

Lesson 08-08 found a cliff: stop reading for longer than the buffer takes to fill
and the device starts losing bytes. Lesson 08-09 kept the loop from falling
behind. Neither helps with the ordinary thing a program does, which is something
slow.

A save, a redraw, a log flush, a call to a network service. Any of them on the
thread that reads can run longer than the budget, and the device does not wait.
The program that did the slow thing has no idea anything was lost.

The fix is not to make everything fast. It is to move the one job that has a hard
deadline onto a thread where nothing slow can be.

## The Concept

### Measured

A device sending 1000 frames a second into a 4096 byte buffer, which lasts 372 ms.
The main thread does nothing for 900 ms, which is longer than that. Then it takes
what is there.

| reading | frames lost | bytes dropped |
|---|---|---|
| on the main thread | 628 | 6904 |
| **on a reader thread** | **0** | **0** |

The 628 is arithmetic, not luck. The buffer holds 372 frames, the stall lasted
longer than it takes to fill, and every frame after that was dropped.

The reader thread's own longest gap between looks at the port was **0.8 ms**,
against a budget of 372 ms. That number is the point: `longest_gap_us()` is how long
the reader went without looking, and the budget can only be spent by code inside the
reader's loop, which has no work in it. It stayed that way when the test was run
with sixteen busy processes fighting for the machine, which is the kind of load a
shared build runner has. See `E-IO-0031`.

### Two ways to take what it read

The reader hands over what it reads in two ways, because lesson 08-09 showed there
are two kinds of message.

`latest()` is the newest message, through a **Seqlock** (lesson 07-12). It is for a
state: position, temperature, battery. It never blocks the reader, however many
threads look, and it never returns half of a message. Measured, with the device
sending 1200 frames while the main thread looked in a tight loop, **1881086** looks
found no torn message and none that went backwards.

`pop()` is every message, in order, through a **queue** (lesson 07-02). It is for a
change: an encoder delta, a step. When nobody is taking them and the queue fills,
the new ones are dropped and **counted**, so a loop that is too slow shows up as a
number: `queue_dropped()`. The oldest are kept, so what is in the queue is still an
unbroken run.

### The thread must be the last member

```cpp
  // ... queue_, storage_, link_, newest_, stopper_, counters ...
  rc::rt::ScopedThread thread_;     // last
```

Members are built in the order they are declared and destroyed in the reverse. A
thread declared first starts before the queue and the link it uses exist. A thread
declared last starts when everything it touches exists, and is joined, by the
compiler, before any of it is destroyed. The destructor asks it to stop first,
because joining a thread nobody told to stop waits for ever. See `E-IO-0033`.

### Rest after a failure

A reader that rests only when nothing was waiting has a hole in the loop. A port
that is failing does not say "nothing waiting", it says "error", and the code for an
error returns at once. The loop asks again, and again, at the speed of the processor.

Measured, 200 ms with every read failing: **355 attempts**, one every 0.56 ms, which
is the rest of 500 microseconds. The reader recovers when the cable is put back.
See `E-IO-0032`.

### Silence is about messages

`seconds_since_message()` counts from the last **valid message**, not from the last
byte. The test sends 200 bytes of noise that never becomes a frame and the answer
stays infinity, which is lesson 08-04's point made again for a thread: a crashed
board holding its line produces bytes all day.

## Build It

`SharedPort` is given: a port two threads can use, with a buffer that drops when
full and a way to make reads fail. Implement `LinkReader` in `exercise/solution.hpp`.
The class around the parts is given, with the member order already right. You write:

- `stop()`: ask, then wait, in that order, safe to call twice.
- `latest()` and `pop()`, which are a line each.
- `seconds_since_message()`.
- `run()`, the loop. The comment lists it, and the two things that are easy to
  leave out are the rest after a failure and the bound of 256 messages per look, so
  that a device that never stops cannot hide the stop request.

```
rcpp verify 08-10
```

The tests use real threads and a real clock, so the stall test takes about two
seconds. They are written so that a loaded machine does not fail them: the stall is
2.4 times the budget and the reader's budget is 372 ms against a typical gap of
under a millisecond.

## Use It

`rc/io/reader.hpp` joins `rc::io`.

```cpp
rc::io::LinkReader reader(port);

// the loop, as often as it likes:
rc::io::Reading newest;
if (reader.latest(newest)) act(newest);
if (reader.seconds_since_message() > 0.5) stop_the_robot();
```

What this costs is a thread and a copy. The thread wakes every 500 microseconds when
idle, which is a small constant load, and on Windows a sleep of 500 microseconds can last
about 15 milliseconds unless the timer resolution is raised, so the reader may poll
less often there. The budget survives it, since it is hundreds of milliseconds, and
the latency does not, which is a reason to wait for the port rather than sleep when
latency matters, and that is a later lesson.

A `Reading` holds 48 bytes of payload. A longer message is counted in `oversized()`
and skipped, because a fixed size is what lets it pass through a `Seqlock` and a
queue without allocating.

## What Breaks First

- **Reading on the thread that does the work.** The budget is spent by everything on
  it. See `E-IO-0031`.
- **A reader that spins when the port fails.** One core at full speed for a cable
  that is out. See `E-IO-0032`.
- **A thread that outlives what it uses, or a newest value that is torn.** Declare
  the thread last, stop before joining, and publish through a `Seqlock`. See
  `E-IO-0033`.

## Ship It

`rc/io/reader.hpp` holds `SharedPort`, `Reading`, `ReaderOptions` and `LinkReader`.

The reader can now deliver what a device says. A robot also has to ask the device
things and be sure the answer is to the question it asked, which is the next lesson.
