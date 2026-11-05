# The Cable Comes Out: Reconnecting Without a Restart

> A cable comes out, goes back in, and the robot has to carry on without a person.
> Open again patiently, stop for mistakes, and start everything over.

**Type:** Build
**Time:** about 180 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 08-12, 08-05, 02-03

## The Problem

Everything in this phase has assumed the port stays open. A robot on a bench stays
plugged in. A robot in a field gets a connector knocked loose, a USB hub that resets, a
board that browns out and comes back as a new device.

If the program treats that as the end, a person has to restart it. If it treats it as
a reason to open the port in a loop, it has three new problems: how often to try,
what not to try again, and what the old connection left behind.

## The Concept

### How often to try

The cable is out for seconds or minutes and the loop comes round in microseconds, so a
retry with no wait is a try on every tick. A fixed wait is a choice between two bad
answers.

A cable out for about a minute, with the port reopened on four different schedules:

| wait between tries | tries while out | worst wait once back |
|---|---|---|
| every millisecond | 53267 | 0 ms |
| every 10 ms | 5929 | 4 ms |
| every second | 62 | 900 ms |
| **10 ms doubling to 2 s** | **38** | **1952 ms** |

The first two are prompt and hammer the system. The third is patient and slow. The
doubling does both: it is as patient as the third while the cable is out, 38 tries
against 62, and it is quick for a glitch, since the first retries are 10, 20 and 40
ms. The price is that after a long absence the wait is up to the cap, so the cap is the
number to choose. See `E-IO-0040`.

`BackoffPolicy::delay` multiplies one step at a time and stops at the cap. The
tempting version, `initial * pow(factor, attempt)`, overflows to infinity for a long
enough absence. The test calls it with a million failures in a row.

### What not to try again

Not every failure is about the cable.

| status | what it usually is | retry? |
|---|---|---|
| not found | the cable is out | yes, with a backoff |
| busy | something else holds the port | yes |
| permission denied | the user is not in the group that owns it | a few times |
| fatal | a baud rate that does not exist | never |

A baud rate with no constant fails every time. A reconnect loop that retries it looks
like patience and hides a mistake in the program, so a fatal status becomes `Failed`
after one try. Permission has two causes that look the same: a moment after a device
appears the system has not yet set its permissions, and a retry works, while a user
who is not in the group fails for ever. A device refused five times in a row is the
second. See `E-IO-0041`.

`Failed` is information. `last_status()` says why, and the caller can print the
sentence a person needs.

### A port that is a BytePort

`ReconnectingPort` is a `BytePort`, so a `Link` goes over it unchanged. While there is
no port it reads **zero**, which a `Link` takes as an ordinary quiet link, and it
**refuses writes**, which is true. The failure is reported once, as a `-1`, at the
moment the cable comes out, and then the link is quiet.

It also drops a port that has produced no bytes for `silence_limit` seconds, if one is
set, because a cable can come out in a way that never reports an error. The limit is
off by default: a device that only answers when asked is legitimately silent.

### What the old connection left behind

Reconnecting restores the port, and nothing else. Two things hold state from the
connection that is gone.

**A half frame.** The cable was pulled out in the middle of a message. The parser is
holding the front of it and waiting for the rest, and what it gets is the start of the
first frame on the new connection.

**A counter that restarted.** Unplugging power cycles most devices, so the sequence
number starts again. The tracker expected 9 and got 100, which is 91 ahead and so
counted as 91 lost messages that were never sent.

The cable pulled out mid frame and put back with the device restarted:

| on reconnecting | sent after | delivered | lost | phantom losses |
|---|---|---|---|---|
| keep the parser and the counter | 17 | 16 | 1 | **91** |
| start both again | 17 | 17 | 0 | **0** |

So the port says when it happened. `generation()` goes up each time a connection is
made, and whatever owns state compares it with the one it last saw:

```cpp
if (port.generation() != seen) {
  seen = port.generation();
  link.reset(new rc::io::Link(port, storage, timeout));
  tracker.reset(new rc::io::SequenceTracker<std::uint8_t>());
  clock_map.reset();
}
```

That is the parser, the sequence counter, the clock map of lesson 08-12 and a request
in flight from lesson 08-11. Everything that learned something from the old connection
has to forget it. See `E-IO-0042`.

## Build It

`BackoffPolicy::delay` and `ReconnectingPort` in `exercise/solution.hpp`. The opener
interface, the states and the members are given. The comments list what each function
does, including the order in which the failures are classified.

```
rcpp verify 08-13
```

The tests use a simulated cable that can be plugged in and pulled out, and a clock that
is a counter, so there is no sleeping and the numbers are the same every run. Three are
about the backoff, three about what is retried, one about a quiet port, one about a
`Link` over it, and the last two are the tables above.

## Use It

`rc/io/reconnect.hpp` joins `rc::io`. A real serial port goes behind a `PortOpener`:

```cpp
class SerialOpener : public rc::io::PortOpener {
 public:
  std::unique_ptr<rc::io::BytePort> open(rc::io::OpenStatus& status) override {
    auto port = std::unique_ptr<rc::io::SerialPort>(new rc::io::SerialPort());
    switch (port->open("/dev/ttyUSB0", 115200)) {
      case rc::io::SerialError::Ok:               status = rc::io::OpenStatus::Ok; return port;
      case rc::io::SerialError::NotFound:         status = rc::io::OpenStatus::NotFound; break;
      case rc::io::SerialError::Busy:             status = rc::io::OpenStatus::Busy; break;
      case rc::io::SerialError::PermissionDenied: status = rc::io::OpenStatus::PermissionDenied; break;
      default:                                    status = rc::io::OpenStatus::Fatal; break;
    }
    return nullptr;
  }
};
```

What this does not do is find the device when it comes back **under a different name**.
A USB adapter can return as `ttyUSB1`, and an opener that only knows `ttyUSB0` waits
for ever. Matching by a stable identifier, the serial number or the physical port, is
the fix, and is a property of the system rather than of this class.

## What Breaks First

- **A reconnect with no sensible wait.** Tens of thousands of tries, or a minute of
  waiting after the cable is back. See `E-IO-0040`.
- **A mistake retried for ever.** A baud rate or a permission looks like a cable that
  is out. See `E-IO-0041`.
- **State from the old connection kept.** A half frame costs the first frame, and a
  restarted counter costs a number of phantom losses. See `E-IO-0042`.

## Ship It

`rc/io/reconnect.hpp` holds `BackoffPolicy`, `OpenStatus`, `PortOpener`, `LinkState`,
`ReconnectOptions` and `ReconnectingPort`.

One lesson is left. The pieces are all here: a port, a link, a reader, requests, time
and a reconnect. The last lesson puts them together and measures the conversation.
