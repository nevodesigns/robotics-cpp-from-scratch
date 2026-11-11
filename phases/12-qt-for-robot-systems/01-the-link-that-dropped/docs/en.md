# The Link That Dropped: Reconnecting to a Robot With a Delay That Grows, and Only Once for Each Drop

> A link drops. Wait before trying again, wait longer after each failure, start short again after a success, and schedule one retry for one drop however many times Qt says it.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 09-12, 09-11, 07-06

## The Problem

A control station is useful while it is connected to a robot, and a robot reboots, a cable is moved and a wireless link goes behind a wall. A station that gives up has to be restarted by the person who is trying to stop the robot. A station that retries in a loop fills the log and the robot with attempts and does nothing else.

## The Concept

### The wait

`next_delay_ms(attempt, base, cap)` is the base doubled for each failure and never more than the cap: with a base of 100 and a cap of 5000 the waits are 100, 200, 400, ... 3200, then **5000** (6400 would be more) and 5000 after that. **100** failures do not overflow, a base that is not positive is no wait, and a cap below the base is the base.

### Refused, and then a success

No server. With a base of 20 ms the waits scheduled were **20**, **40** and **80**. Then a server appeared on the same port: the client connected, the count of failures went back to **0** and the count of good connections was **1**. The next failure after a success is a first failure again, and waits the base. See `E-QT-0096`.

### One drop

A server that closed a link that was up: the socket reported an error (the remote host closed it) **and** that it had disconnected. A client that acts on both has scheduled two attempts for one drop, and two timers become four. This one scheduled **1** wait, of **20** ms, and then reconnected: good connections **2**. The failed attempts of the refused case report only the error. See `E-QT-0097`.

### Lines

A line written in pieces, `temp 2` and `1.5\nvolt 12` and `.1\n`, was **2** lines, `temp 21.5` and `volt 12.1`, and half a line was not a line (lesson 09-12). Each complete line is also announced by `line_received` as it arrives, and `send_line` sends a line to the robot when the link is up and refuses, sending nothing, when it is not: it was refused after `stop()`.

### An attempt that takes too long

How long a failed attempt takes to be reported is the operating system's business. Linux reported a refused connection to this machine at once. On the Windows runner of this repository the first version of this lesson waited a second and a half for a refusal to a port with nothing listening and none had been reported. A host that is switched off, or behind a firewall that drops packets, is never reported at all. So the client keeps a limit of its own, `connect_timeout_ms`, and an attempt that is still running when it ends is aborted and counted as a failure. The waits in this lesson used a limit of **100** ms and were the same on both systems. See `E-QT-0096`.

### Stopping

After `stop()`, with the server coming back and **200** ms passing, there were no more waits, the state was stopped and there were **0** good connections: a retry that is waiting is ended, and a client that is stopped is not started by a link that is good. It could be started again. See `E-QT-0098`.

## Build It

`next_delay_ms`, `LinkClient::on_connected` and `LinkClient::schedule_retry` in `exercise/solution.hpp`. The limit on an attempt is supplied.

```
rcpp verify 12-01
```

## Use It

`rc/qt/link_client.hpp` joins its module.

```cpp
rc::qt::LinkClient link(QHostAddress("192.168.1.20"), 5000, 100, 5000, 2000);
QObject::connect(&link, &rc::qt::LinkClient::state_changed, this, [this](int s) { show_link_state(s); });
link.start();
// on exit: link.stop();
```

**What this does not do.** It retries a connection that is refused, timed out or closed, and it knows nothing of one that is up and silent: a link that stops sending but is not closed is not a failure to this client (a later lesson of this phase is that). The waits have no random part, so a hundred stations that lose the same robot at once retry together, and a real fleet adds jitter. It is plain TCP, with no encryption and no authentication, and a robot on a network that others share needs both. And the lines are text: a binary protocol needs the framing of lesson 09-07.

## What Breaks First

- **A retry loop with no wait.** See `E-QT-0096`.
- **Two retries for one drop.** See `E-QT-0097`.
- **A retry that outlives the stop.** See `E-QT-0098`.

## Ship It

`rc/qt/link_client.hpp` holds `next_delay_ms` and `LinkClient`.

The capstone of this phase keeps its link to the robot with it.
