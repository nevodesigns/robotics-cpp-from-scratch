# The Command That Arrived Twice: Retrying What Was Not Acknowledged, and Applying It Once

> Retry what was not acknowledged. Number every command, and apply a number once, however often it arrives.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 12-02, 03-03

## The Problem

The station tells the robot to move forward one metre. The link drops the message, so the station, which has not heard back, sends it again. That is right. Then the link drops the acknowledgement of a command that did arrive, the station sends it again, and the robot has two copies of an order that should have been one.

## The Concept

### The sender

`CommandSender` numbers each command (a 16 bit number that wraps), keeps it until it is acknowledged, and says which are due to be sent again: after the retry time, each counts as another attempt. A command sent its limit of times and due again **fails**, is removed, and is listed. `ack` is true for a command that was waiting and false for one that was not.

### The receiver

`CommandReceiver` applies a number it has not seen and only recognises one it has, within a window of the numbers it remembers. The caller acknowledges both: the acknowledgement of a duplicate is how the sender stops.

### What the channel did

A simulated channel in steps of 10 ms, a retry time of 50 ms and 5 attempts, with a chosen message lost:

| lost | transmissions | arrivals | applied by the receiver |
|---|---|---|---|
| nothing (20 commands) | **20** | 20 | **20** |
| the first request of each of 10 commands | **20** | 10 | **10** |
| the first acknowledgement of each of 10 commands | 20 | **20** | **10** |

The second row: nothing arrived until the retry, and then each was applied once. The third: every command arrived twice, the receiver applied each **once** and refused **10** duplicates, and a robot that applied every arrival would have done **20**, one metre each, twice. See `E-IO-0046`.

### Giving up

A link that lost everything: the command was transmitted **5** times, the first and four retries, then **failed**, and was not waiting any more. A failed command is an event the station has to show. See `E-IO-0047`.

### Wrapping

**70000** commands used **65536** different numbers, no number came twice in a row, and the last was 4463. An answer that is not wanted is refused: a second `ack` of the same number, and one for a number never sent. And a receiver with a window of two forgot a number after two others: a window is a limit, and a command that is retried after it has gone by is applied again. See `E-IO-0048`.

## Build It

`CommandSender::send`, `due` and `ack`, and `CommandReceiver::accept`, in `exercise/solution.hpp`.

```
rcpp verify 12-03
```

## Use It

`rc/core/command_channel.hpp` joins its module.

```cpp
rc::core::CommandSender sender(200, 5);
const std::uint16_t seq = sender.send(now_ms);  transmit(seq);
for (std::uint16_t again : sender.due(now_ms)) transmit(again);
// on an acknowledgement:  sender.ack(seq);
// on the robot:  if (receiver.accept(seq)) apply(command);  acknowledge(seq);
```

**What this does not do.** It makes a command safe to repeat and does not make it ordered: two commands that arrive in the wrong order are applied in that order, and a protocol that needs order needs it said. It remembers a window of numbers, so the window has to be larger than the number of commands that can be outstanding, and a retry that comes after a long outage can be applied again. It cannot make a command that does something by its nature, such as a relative move, into one that is safe to apply once and cannot be undone if a command that failed had in fact arrived. And the robot has to keep its window across a restart, or a restart forgets what it has done.

## What Breaks First

- **A duplicate applied twice.** See `E-IO-0046`.
- **A command that is retried for ever or never.** See `E-IO-0047`.
- **A sequence number that wraps into trouble.** See `E-IO-0048`.

## Ship It

`rc/core/command_channel.hpp` holds `CommandSender` and `CommandReceiver`.

The capstone of this phase sends the commands of its operator this way.
