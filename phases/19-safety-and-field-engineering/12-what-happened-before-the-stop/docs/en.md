# What Happened Before the Stop: A Fixed-Size Black Box With a Hash Chain, Wrap-Around That Can Still Be Verified, and What Tampering Leaves Behind

> After an incident, the first question is what the robot was doing. If the answer is that it was fine, until it was not, there is nothing to learn.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 19-11, 19-10, 18-10, 03-03

## The Problem

The supervisor of lesson 19-10 stopped the robot, and recorded a reason. That is the **result**. It is not what led to it: what the speed was, how the range had been closing, whether a heartbeat had been late for a while first. An investigator, a customer, or you at 3 am a month later needs the **last few seconds before the stop**, in order, in the robot's own words, and needs to know that nobody has altered them.

A flight recorder has two requirements that a normal log does not. It must **never get in the way**: recording must not block, allocate or fail in the control loop, or the recorder becomes a hazard. And it must be **trustworthy**: when the memory is full it keeps the newest, and what it keeps can be checked.

## The Concept

### A ring that never allocates

`BlackBox` is a ring of `Event`s of a fixed capacity, allocated once when it is made. `record` writes one event and **cannot throw** (the test says so with a `static_assert`) and cannot fail: when the ring is full the **oldest** event is overwritten, and a counter says how many have gone. A capacity of 0 is harmless: nothing is kept and nothing breaks. An event has a **sequence number** (0, 1, 2 and so on since the box began), the robot's clock, a code, a value and a chain value. The sequence number, and not the clock, gives the order: times 5, 4, 6, from a clock that was set back, were kept in order as sequence 0, 1, 2 and counted as **1 time going back**.

### A chain, so that a change shows

Each event carries a hash of itself and of the event before it (FNV-1a, 64 bits). So a change to one event changes its chain value and, because the next event includes it, **every one after it**: five events with one value different in the middle gave a different chain at the end. It is not secret and it is not a signature. It is a seal that shows **accidental** damage and **careless** tampering, and it costs a few multiplications per event.

### The ring wraps, and the chain must still check

When the ring is full, the oldest event is overwritten, and the first event that is left no longer has its predecessor. A check that starts the chain from zero calls the first event **altered**, though nothing has been touched. So the box keeps the chain value of the **last event it overwrote** (`base_chain`) and the number of events dropped, and `verify` starts from those. Twenty events into a ring of 8: 12 dropped, sequence 12 to 19, **verified from the kept link: yes, from zero: no**, with the first event called altered. After 100 events, 92 dropped, it still verified. See `E-SAFE-0034`.

### What the check finds, and what it does not

`verify` looks for a **gap** first (the sequence must run on from the number dropped) and then for an **altered** event (the chain must follow). Twenty events, then:

| what was done | result |
|---|---|
| the value of event 7 changed | altered at 7 |
| event 7 deleted | gap at 7 |
| events 7 and 8 swapped | gap at 7 |
| the last five cut off | not noticed |

A deleted event breaks both the sequence and the chain, and the more useful answer is that something is **missing**. A swap shows as a gap at the first position out of place. And the last one is the limit of the method: the end of a chain has nothing after it to disagree, so **cutting off the newest events is not detected**. To catch that, the latest chain value has to be kept somewhere else (sent over the radio, written to a second device, shown on a display), and then it is the real protection against someone who means it. See `E-SAFE-0035`.

### The window before the stop

`window_before` gives the events immediately before the first one with a given code. A robot driving at 1.5 m/s at a wall 18 m away recorded two events a tick (speed and range) and a stop event when the range fell below 0.6 m: **235 events**, of which **171** were overwritten, the ring holding the last 64, and the log still verified. The six events before the stop:

| time | event | value |
|---|---|---|
| 11.4 s | speed | 1.50 |
| 11.4 s | range | 0.90 |
| 11.5 s | speed | 1.50 |
| 11.5 s | range | 0.75 |
| 11.6 s | speed | 1.50 |
| 11.6 s | range | 0.60 |

The range was closing at 1.5 m/s with the speed **unchanged**: the robot did not slow for the wall at all, a finding that the stop event alone ("fence, bit 4") would never have given. If fewer events than asked for are held, it gives all there are; if there is no such event, none. See `E-SAFE-0036`.

## Build It

`chain_next`, `BlackBox::record` and `dump`, `verify` and `window_before` in `exercise/solution.hpp`.

```
rcpp verify 19-12
```

## Use It

`rc/safety/blackbox.hpp` joins `rc::safety`.

```cpp
static rc::safety::BlackBox box(4096);                       // made once, at start up
box.record(robot_clock(), kSpeedCode, speed);                // in the control loop, every tick
// after an incident, in a tool, from the saved dump:
const auto dump = box.dump();
if (!rc::safety::verify(dump).ok) warn("the record was damaged");
const auto last = rc::safety::window_before(dump, kStopCode, 50);
```

**What this does not do.** The box is **in memory**. It does not survive a power cut, and a robot whose battery fails is the case where it is most needed: writing the dump to flash on every stop, and on a falling supply voltage, is a hardware question that this class does not answer. It is **not signed**: anyone who can recompute the chain can forge a log, and for a log that has to stand up to a dispute it needs a keyed hash and an external anchor. It has no memory of what the codes **mean**, which is the table next to it. It is not thread safe as written: a recorder shared between threads needs a lock-free design (the threading phases) and care that recording still cannot block. And recording **less** is a decision: what is not recorded is not available, and the rate and the choice of events are yours to make before the incident, not after.

## What Breaks First

- **A check that starts the chain from zero after the ring has wrapped.** See `E-SAFE-0034`.
- **A log that cannot tell missing from altered, or truncation from a clean end.** See `E-SAFE-0035`.
- **A stop that was logged with a reason and nothing before it.** See `E-SAFE-0036`.

## Ship It

`rc/safety/blackbox.hpp` holds `Event`, `chain_next`, `Dump`, `BlackBox`, `Fault`, `Verified`, `verify` and `window_before`.
