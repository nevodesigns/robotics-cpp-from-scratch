# A Queue for Each Listener: Depth, Overflow and What a Slow Subscriber Loses

> A subscriber that cannot keep up does not slow the publisher down. It loses messages. The only choice is which.

**Type:** Build
**Time:** about 110 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 17-01, 03-03

## The Problem

Lesson 17-01 delivered every message the moment it was published, by calling the subscriber. That works while the subscriber is quick. A subscriber that draws a picture, or writes to disk, or runs a planner for 80 ms is called by whatever thread published, and holds it up. The publisher is a sensor driver
on a timer, and for 80 ms it is not reading its sensor.

So each subscriber gets its own **queue**. The bus puts the message in and returns; the subscriber takes it out when it is ready. The publisher is no longer slowed by the slowest subscriber, and a new problem arrives in exchange: the queue is finite, and a subscriber that is slower than the publisher fills it.

## The Concept

### A queue per subscriber

```cpp
Handle subscribe(const std::string& topic, std::size_t depth, Overflow overflow);
void publish(const std::string& topic, const std::string& payload);
bool take(Handle handle, std::string& out);
```

The bus puts a copy in the queue of every subscriber of the topic, and the subscriber takes the oldest. Order within a queue is order of publication. A depth of zero would be a queue that holds nothing, so it is treated as one.

### Which message to lose

When the queue is full and another message arrives, one is lost, and it matters which:

| rule | depth 3, five messages | took | lost |
|---|---|---|---|
| drop the oldest, keep the newest | 1 2 3 4 5 | 3 4 5 | 2 |
| drop the newest, keep what is waiting | 1 2 3 4 5 | 1 2 3 | 2 |

A controller wants the newest position: the old ones are history. A log that is being copied to a file wants the oldest first, and a hole at the start is worse than missing the tail. The bus counts every loss, and the count is how a slow subscriber finds out it has been slow. See `E-BUS-0004`.

### Separate queues

Each subscriber's queue is its own. Ten messages, a subscriber that took each one as it came and one that never took any:

```
fast lost 0 and took 10, slow lost 8 and has 2 waiting
```

The slow one lost 8 and the fast one beside it was not touched. With a shared queue, the slow subscriber would have held back every other. See `E-BUS-0004`.

### What depth buys

Depth is a buffer, and a buffer rides out a burst. It does not cure a rate. A burst of 8 messages, then the subscriber takes them all:

| depth | lost |
|---|---|
| 1 | 7 |
| 4 | 4 |
| 8 | 0 |
| 10 | 0 |

A queue as deep as the burst loses nothing. Now a publisher at 100 Hz for 10 seconds and a subscriber that takes one message every 100 ms, 10 Hz:

| depth | lost of 1000 |
|---|---|
| 1 | 900 |
| 10 | 891 |
| 100 | 801 |

A hundred times the depth saved 99 messages, which is what it holds, and nothing else. A subscriber that is slower than the publisher all the time loses the difference, 900 a second here, whatever the depth, and the deeper queue only delays the loss and adds the age of what it delivers. The cure is a faster subscriber, a slower publisher, or a decision
that the subscriber only wants the latest, which is depth 1 by choice. See `E-BUS-0005`.

## Build It

`subscribe`, `publish` and `take` in `exercise/solution.hpp`. `pending`, `lost`, `taken` and the `Overflow` choice are given.

```
rcpp verify 17-02
```

Nothing is timed: the rates are simulated by counting messages, so the figures above are the same on every run.

## Use It

`rc/bus/queued_bus.hpp` joins `rc::bus`.

```cpp
QueuedBus bus;
const auto pose = bus.subscribe("pose", 1, Overflow::drop_oldest);       // the controller wants the latest
const auto log = bus.subscribe("pose", 10000, Overflow::drop_newest);   // the logger wants the beginning
```

**What this does not do.** It is still one thread: `take` and `publish` are not safe to call from two threads at once (lesson 7-02 and 7-06 built the queues that are). It has no notion of time, so it cannot say how old a waiting message is, and a deep queue delivers old data that looks like new (a later lesson in this phase covers lifespan).
It does not tell the subscriber how many it lost until asked. And its depth is chosen by the subscriber alone, where a real middleware lets the publisher offer a history too.

## What Breaks First

- **A queue that is too shallow for a burst.** Losses at every spike. See `E-BUS-0004`.
- **A queue that is deep and the subscriber slower all the time.** Growing delay and loss anyway. See `E-BUS-0005`.
- **A queue that keeps the wrong end.** A controller acting on the past, or a log with a hole. See `E-BUS-0006`.

## Ship It

`rc/bus/queued_bus.hpp` holds `Overflow` and `QueuedBus`.

Every subscriber now sets its own pace, and finds out what the pace cost it.
