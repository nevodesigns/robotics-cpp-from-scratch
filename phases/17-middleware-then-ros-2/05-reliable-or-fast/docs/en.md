# Reliable or Fast: Acknowledgements, Retransmission and a Window

> A message that may be lost has a cost to make it not lost, and the cost is time and traffic. Paying it for every topic is how a robot's network falls over.

**Type:** Build
**Time:** about 130 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 17-03, 17-04, 03-03

## The Problem

Up to here a message that was published arrived. A real link, a wireless one to a robot or a busy network, loses some of what it carries. A lost pose is replaced a moment later by the next one and nobody minds. A lost command to open a gripper is not, and nor is a lost map or a lost parameter change.

ROS 2 calls this a **quality of service** choice for a topic: best effort, which sends once and moves on, or reliable, which makes sure. Reliability is not magic: it is acknowledgements and resending, with a cost that this lesson measures.

## The Concept

### Number, acknowledge, resend

The sender gives each message a number. The receiver tells it, with an **acknowledgement**, the next number it expects, which says that everything below that has arrived. A number that is not acknowledged within a **timeout** is sent again.

```cpp
std::vector<DataPacket> tick(int now);   // what to put on the wire now: new packets, and old ones that timed out
void on_ack(int next_expected);          // everything below it is done
```

A **window** limits how many unacknowledged packets are on the wire at once. With a window of 4 and ten messages, the first tick sent 4, the next tick sent nothing, and after acknowledging 2 the following tick sent 2: numbers 4 and 5. A packet not acknowledged is sent again at exactly the timeout and not a tick before, and every resend is counted. See `E-BUS-0014`.

### The receiver

Packets arrive late, early and twice, and the receiver hands them up in order, once each. The next one it needs is delivered at once; one that comes early is kept until the gap fills; one it has already delivered is a **duplicate** and is ignored but still acknowledged, because the sender may have lost the acknowledgement. Arrivals 2, 0, 1, 1, 0:

| arrival | acknowledges | delivered so far |
|---|---|---|
| 2 | 0 | |
| 0 | 1 | a |
| 1 | 3 | a b c |
| 1 | 3 | |
| 0 | 3 | |

Number 2 came early and waited, 0 was delivered, 1 filled the gap and released 2 as well, and the two repeats were acknowledged and counted: 2 duplicates. The acknowledgement is cumulative, so a lost acknowledgement is covered by the next. See `E-BUS-0013`.

### What it costs

500 messages over a link that loses 20 percent of what it carries in each direction:

| | delivered | in order | packets sent per message |
|---|---|---|---|
| send once | 398 | yes | 1.00 |
| acknowledge and resend | 500 | yes | 1.79 |

Sending once lost 102 of 500, as a fifth would. Reliable lost none and put 1.79 packets on the wire for every message: some data packets were lost and resent, and some arrived while their acknowledgements were lost, which looks the same to the sender. A link that is already struggling gets almost twice the traffic from a reliable topic. That is
right for a command and a mistake for a stream of scans. See `E-BUS-0015`.

### What the window buys

Reliable delivery has a second price, which is latency. A sender waiting for each acknowledgement before sending the next does one message per round trip. 100 messages, a one way delay of 50 ticks and no loss:

| window | ticks to deliver all |
|---|---|
| 1 | 10000 |
| 2 | 5000 |
| 5 | 2000 |
| 10 | 1000 |
| 20 | 500 |

A window of 1 is 100 messages at 100 ticks, a round trip, each. The time falls in proportion to the window until the link is full. See `E-BUS-0014`.

## Build It

`send`, `tick`, `on_ack` and `on_data` in `exercise/solution.hpp`. The counters and `idle` are given.

```
rcpp verify 17-05
```

The link in the tests loses packets by a generator seeded in the test, so the figures above are the same on every run.

## Use It

`rc/bus/reliable.hpp` joins `rc::bus`.

```cpp
ReliableSender sender(10, 120);
ReliableReceiver receiver;
sender.send(command);
for (const DataPacket& p : sender.tick(now)) link.transmit(p);
// when a packet arrives: link.transmit_ack(receiver.on_data(packet));
```

**What this does not do.** It resends on a fixed timeout, where a real protocol measures the round trip and adapts it, and backs off when the link is bad, which is what keeps a retransmitting sender from making a congested network worse. The receiver's buffer is not bounded here. There is no flow control from a slow receiver, and no way to give up on a message: a link that is gone makes the sender
resend for ever, which a real middleware limits with a history depth (lesson 17-02) or a deadline (a later lesson). It also resends everything in the window, not only what is missing.

## What Breaks First

- **A receiver that trusts arrival order.** Duplicates delivered twice and gaps delivered out of order. See `E-BUS-0013`.
- **A window of one, or a timeout shorter than the round trip.** Slow, or a storm of needless resends. See `E-BUS-0014`.
- **Reliable by default.** Almost twice the traffic on a link that is already in trouble. See `E-BUS-0015`.

## Ship It

`rc/bus/reliable.hpp` holds `DataPacket`, `ReliableSender` and `ReliableReceiver`.

A topic can now be made to arrive, and the cost of that is a number.
