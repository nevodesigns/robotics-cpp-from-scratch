# Record and Replay: A File of Messages That Reproduces the Day It Went Wrong

> A robot did something strange at 3:14 yesterday. You were not there. What you have is whatever was recorded.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 17-03, 17-01, 17-13

## The Problem

Robots fail on the day nobody is watching, in the corridor with the bad light, and not on the bench. The way to understand it later is to have written down every message that crossed the bus, with its time, and to be able to put it back. The record is a **bag**; putting it back is **replay**; and a node that gets the same messages at the same pace does the same thing, so the bad day can be
run as often as it takes.

Two things make that harder than it sounds. The file is read from disk, perhaps damaged by a power cut at the end of a recording, and has to be taken apart without trusting it. And replay is about time: what is played too fast arrives in a different order relative to the node's own timers.

## The Concept

### A bag as bytes

A bag is its messages in order, each a topic, a stamp and a payload, written with the writer of lesson 17-03: the six characters `RCBAG1`, a count, then each message's topic (a length and characters), stamp (a double) and payload (a length and bytes). Four messages, one with a zero byte in its payload, were **92 bytes**, and read back identical: every stamp equal to the last bit, every payload the same
size, the topic counts and the duration right. A payload is bytes and not a string: one with `\0` in it round trips because it is written with its length. See `E-BUS-0046`.

### A file that stops

A recording ends with a power cut, in the middle of a message. The reader must say so, not guess. The test took the 92 byte file and cut it at every length from 0 to 91:

```
every prefix of the file, 92 of them: 92 refused, 0 accepted or left something behind
```

Each was refused, and none left half a bag in the output: a failed read leaves the bag **empty**, so a caller that ignores the status still does not replay a partial file as if it were whole. A file that does not start with `RCBAG1`, here with its first letter changed, is `bad_magic`. See `E-BUS-0047`.

### A count that lies

The count says how many messages follow, and a reader that trusts it will reserve space for that many. A header that claims four billion messages in a file of 27 bytes would ask for gigabytes before finding out. Every message takes at least 12 bytes (two lengths and a double), so a count above what the rest of the file could hold is refused at once as `too_many`:

```
a header claiming 4000000000 messages in a file of 27 bytes: too_many
```

See `E-BUS-0048`.

### Replay, at a pace

A player counts time from the **first** message, whatever its stamp was: the bag started at 100.0 s, and replay starts at 0. Messages at 0, 0.1, 0.3 and 1.0 s after the first, stepped in 0.05 s:

```
published per step 1 1 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 0 0 0
```

Each message appeared at the step where it came due, in order, none early and none twice, and a further step after the end published nothing. A **rate** scales the pace:

| rate | seconds until the last message |
|---|---|
| 0.5 | 2.00 |
| 1.0 | 1.00 |
| 2.0 | 0.50 |
| 4.0 | 0.25 |

At 4 times the rate the whole second is over in a quarter of a second, and a node with its own timers, which follow the clock of lesson 17-13, needs that clock sped up with it. See `E-BUS-0048`.

### The same output

The test that matters. A node keeps a running average of what is sent on `scan` and publishes it. Run live, with a recording of the scans made as they went by, it gave:

```
live output:     2.000 3.000 2.333 3.250 3.000
replayed output: 2.000 3.000 2.333 3.250 3.000
```

The recording was written to bytes, read back, and played into a brand new bus with a brand new node, and the outputs were **equal**. That is the property that makes a bag worth having: the node's behaviour depends only on the messages, so the file is the bug. See `E-BUS-0046`.

## Build It

`serialize`, `parse` and `step` in `exercise/solution.hpp`. `add`, the accessors and the types are given.

```
rcpp verify 17-16
```

Everything is exact and untimed, so the figures above are the same on every run.

## Use It

`rc/bus/bag.hpp` joins `rc::bus`.

```cpp
Bag bag;
bus.subscribe("scan", [&](const std::string& t, const std::string& p) { bag.add(t, clock.now(), p); });
write_file(bag.serialize());
// later
Bag again;  Bag::parse(read_file(), again);   Player player(again, 1.0);   player.step(dt, bus);
```

**What this does not do.** It holds the whole bag in memory, where a real recorder writes to disk as it goes and indexes the file so a part can be played. It has no compression and no per message checksum (lesson 15-13's CRC would catch a flipped bit that this format would replay). It replays what it recorded and a node that also reads the wall clock, a random number or a file will not give the same output, which is the usual reason a replay does
not reproduce a bug. And it records the messages a subscription saw, which is not those that would have arrived without it if the queues of lesson 17-02 were dropping.

## What Breaks First

- **A payload treated as a string.** A zero byte ends it early. See `E-BUS-0046`.
- **A damaged file read as if it were whole.** A replay of half a recording with no warning. See `E-BUS-0047`.
- **A count believed.** A gigabyte allocated for a file of 27 bytes. See `E-BUS-0048`.

## Ship It

`rc/bus/bag.hpp` holds `BagEntry`, `BagStatus`, `Bag` and `Player`.

The bus can now remember a day, and give it back exactly.
