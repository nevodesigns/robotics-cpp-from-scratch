# Too Late to Be True: Deadlines, Lifespans and Data That Is Old by the Time It Arrives

> A message is a statement about a moment. The longer it is in transit, in a queue or in a retained buffer, the less it says about now.

**Type:** Build
**Time:** about 110 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 17-02, 17-04

## The Problem

Discovery (lesson 17-04) answers whether a node is alive, with a lease on its heartbeat. A node can be alive and its data stalled: the lidar driver is running and has stopped producing scans because a cable came loose. A topic needs its own watch on how often it delivers.

And data that does arrive can be old. Lesson 17-02 showed a queue as a buffer, and a deep queue that a slow subscriber is always behind on is an old data store: what it serves is as old as the queue is long. A controller acting on a pose from five seconds ago is acting on a world that is not there.

ROS 2 gives both a name in its quality of service: the **deadline**, how often a message must come, and the **lifespan**, how long a message stays worth delivering.

## The Concept

### A deadline monitor

The monitor is told when messages arrive and asked, from time to time, whether the promise was kept.

```cpp
void on_message(double now);
int check(double now);          // 1 for a new miss, otherwise 0
```

A silence longer than the deadline is a miss, reported **once**, however long it goes on, and not again until a message arrives and a new silence starts. Reporting every check would turn one dead lidar into a thousand log lines. A silence of exactly the deadline is not a miss. Nothing is missed before the first message, since a stream that has not begun has no promise yet to break.
Two silences gave 2 misses, and the longest gap between messages was 8.9 s.

A publisher that died after its last message at 1.0 s, with a deadline of 0.25 s, was noticed at 1.26 s: one deadline after the last message, and not before. See `E-BUS-0022`.

### The deadline and the jitter

Messages do not come exactly on time. A message every 0.10 s, each as much as 0.03 s early or late, 2000 messages:

| deadline (s) | misses reported |
|---|---|
| 0.10 | 992 |
| 0.12 | 441 |
| 0.14 | 124 |
| 0.17 | 0 |

A deadline equal to the period is broken about every other message by jitter alone, which is a monitor that cries wolf until nobody listens. The worst gap here is the period plus both jitters, 0.16 s, and a deadline of 0.17 s never fired. The deadline is the period plus the worst jitter the publisher is allowed, and a bit, which means measuring it: lesson 7-03 is how. See `E-BUS-0023`.

### A lifespan

A **lifespan** says a message is not worth delivering after so long. The queue checks the age of each message as it takes it, throws away the expired and counts them:

Five messages made at 0.0 to 0.4 s, a lifespan of 0.25 s, taken at 0.5 s: the subscriber got `m3`, aged 0.20 s, and 3 had expired, so it did not get a message half a second old.

The harder case is a consumer that is always behind. Published every 10 ms, taken every 20 ms, for ten seconds:

| lifespan | served | expired | oldest age served (s) |
|---|---|---|---|
| none | 500 | 0 | 5.00 |
| 0.5 s | 500 | 450 | 0.50 |
| 0.1 s | 500 | 490 | 0.10 |

With no lifespan the consumer was served 500 messages in order, each older than the last, until it was acting on data five seconds old. A lifespan put a ceiling on it: the consumer is just as slow and no longer sees a world that is far behind. It sees less of it, which is the cost, and it knows how much less from the count. See `E-BUS-0024`.

## Build It

`on_message`, `check` and `take` in `exercise/solution.hpp`. The counters, `size` and `longest_gap` are given.

```
rcpp verify 17-08
```

Time is passed in and the jitter is from a seeded generator, so the figures above are the same on every run.

## Use It

`rc/bus/staleness.hpp` joins `rc::bus`.

```cpp
DeadlineMonitor lidar(0.17);
// on every scan: lidar.on_message(now);   on a timer: if (lidar.check(now)) enter_safe_mode();
ExpiringQueue poses(0.1);
```

**What this does not do.** It uses the time it is given, and that time has to be from one clock: a message stamped by another machine whose clock is five seconds out is expired or fresh by accident (a later lesson in this phase is about time across machines). The monitor has to be asked: nothing here wakes up by itself when a deadline passes, so it needs a timer, and the timer's own jitter adds to the margin.
And it says a stream is late, not why: a lidar that has stopped and a link that has dropped look the same.

## What Breaks First

- **A deadline monitor that reports every check.** One failure floods the log. See `E-BUS-0022`.
- **A deadline set to the period.** A monitor that fires on jitter. See `E-BUS-0023`.
- **A queue that serves what it has, however old.** A controller acting on the past. See `E-BUS-0024`.

## Ship It

`rc/bus/staleness.hpp` holds `DeadlineMonitor` and `ExpiringQueue`.

A subscriber can now tell that a stream has gone quiet, and refuse data that is too old to act on.
