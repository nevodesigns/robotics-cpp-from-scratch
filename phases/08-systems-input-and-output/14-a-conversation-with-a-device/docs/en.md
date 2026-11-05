# A Conversation With a Device: Everything in Phase 08, Together

> A device streams, answers, goes quiet, is unplugged and comes back reset. Put
> every piece of the phase in one object and measure what it does to that.

**Type:** Build
**Time:** about 240 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none required, see below
**Prerequisites:** 08-13, 08-11, 08-10

## The Problem

Thirteen lessons each fixed one thing: a struct that is not its bytes, a stream with no
messages in it, a checksum that lets one in 256 through, a read that returns nothing, a
port that is not set up for binary, damage and what it costs, a parser that throws away
the next frame, a buffer that fills, a loop that acts on the past, a reader that cannot
be held up, an answer to the wrong question, a clock that wraps and drifts, a cable that
comes out.

Each was tested alone. A robot uses them together, and the interesting failures are in
how they meet: a request that eats the telemetry, a reconnect that leaves the old clock
in place, a loss counter that does not see the outage.

This lesson puts them in one object, `Session`, and runs a conversation against a
simulated device that misbehaves.

## The Concept

### The device you do not own

Most of this curriculum's hardware is not on the learner's desk, and the code that talks
to it still has to be tested. `SimDevice` is the other end of a link. It streams
numbered, time stamped telemetry at 100 Hz, answers requests, replays a repeat instead
of repeating it, and can be unplugged, power cycled, made to answer late or not at all,
and starved of buffer. Its own clock runs 50 ppm fast and wraps every 65.5 seconds.

It runs on the host's clock, passed in as a number, so a test can make a minute pass in
a moment and get the same answer every time. Three messages, all framed as the `Link`
frames them:

```text
telemetry  02  sequence(u16)  ticks(u16)  position(i32)
request    01  id  operation  arguments
response   81  id  status  position(i32)
```

### One place reads the link

The simple way to ask a device something is to send the request and poll the link until
the answer turns up, discarding what is not it. Telemetry arrives all the time, so the
wait eats it.

Fifty requests made while a 100 Hz stream runs on the same link:

| | telemetry lost |
|---|---|
| waiting by polling the link and discarding the rest | **38** |
| a session, which routes each message to who wants it | **0** |

`Session::tick` polls the link in exactly one place and offers each message to the
requester first. The requester takes it if it is a response, whether or not it is the
one being waited for, and says so. Anything else of nine bytes that starts with the
telemetry type is telemetry. The requester never polls, so it cannot read what is not
its own. See `E-IO-0043`.

### A clean conversation

Two minutes, 100 Hz, a jittery link, no faults:

```text
delivered 12000, lost 0, checksum failures 0, clock drift found 53.8 ppm
the newest frame is placed on our clock to within 0.09 ms
```

Every frame in order, and the device's time on our clock to a tenth of a millisecond
after two minutes. The drift is found at 53.8 for a device that was 50.0, from ten
second buckets that have only just begun to fill.

### A hostile minute

Sixty seconds with 0.04 percent of bytes dropped and 0.04 percent flipped, one answer in
ten late and one in fifteen lost, the cable out for three seconds at twenty, and the
device power cycled while it was out.

| | |
|---|---|
| telemetry sent | 6000 |
| delivered | 5609 |
| counted lost by the sequence numbers | 90 |
| checksum failures | 61 |
| connections made again | 1 |
| requests sent / resent / gave up on | 277 / 40 / 0 |
| answers recognised as belonging to something else | 24 |
| moves answered after the power cycle / done by the device | 177 / 177 |
| repeats replayed instead of repeated | 40 |
| worst error placing a frame on our clock once settled | 4.0 ms |

Four things in that table are the phase doing its work. The device did exactly the 177
moves that were answered, because 40 resends were replayed and not repeated. The 24
late answers were recognised and counted instead of being taken for the next question's
reply. The clock was within 4 ms after a power cycle, because the map started over on
the new connection. And the loss counter says 90, which is true and not the whole truth.

### The loss that no counter sees

6000 sent and 5609 delivered is 391 missing. The sequence numbers counted 90.

The other 301 are in no counter. A tracker is started over when the connection is made
again, because a device that restarted counts from zero and a tracker kept across that
reports phantom losses, which lesson 08-13 measured. So whatever was lost while the
cable was out is outside every count. The 301 are the three seconds of outage at a
hundred frames a second.

`Health::lost` is loss **while connected**, and `Health::reconnects` is how many times
the connection was made again. Report both, and do not present the first as the whole
story. See `E-IO-0044`.

### What starts over

On a new connection the session starts the parser, the sequence tracker, the tick
unwrapper, the clock map and the requester again, and counts a request that was in
flight as **abandoned**. A device off for five seconds and rebooted at thirty was within
12.0 ms for the next twenty, where lesson 08-12 measured a map kept from before at
5011 ms. See `E-IO-0045`.

### Over a real terminal

On Linux the last test puts a pseudo terminal between the session and the simulated
device. The session opens it with the real `SerialPort` from lesson 08-05, with its real
`termios` settings, through a `PortOpener`, and the bytes cross a real kernel terminal
layer.

```text
1.6 seconds: delivered 160 telemetry frames, lost 0, checksum failures 0,
5 requests asked, the device at 5
```

That is the whole stack, from the opener to the clock map, with nothing faked but the
far end.

## Build It

`Session` in `exercise/solution.hpp`. Every part is something you built, in `rc::io`.
The result types and the members are given. You write `tick`, `ask`, `health`,
`accept_telemetry` and `start_over`, and the comments say in what order.

```
rcpp verify 08-14
```

Eight tests run on a virtual clock: nothing connected, then the device appearing; a clean
conversation; requests sharing the link; the answer carrying the position; the request
abandoned at a reconnect; the clock map starting over; the shared link measured against
a blocking wait; and the hostile minute. The ninth is the pseudo terminal, and it runs
for about two seconds of real time.

## Hardware, and what is actually tested

No hardware is needed. With a USB serial adapter and a microcontroller that speaks the
three messages above, the same `Session` talks to hardware by changing the one line
that says which path the opener opens.

What the tests do not do is as important as what they do. The faults are **independent**:
each byte is unlucky on its own, and a real cable gives bursts. The link's delay is a
number from a generator and not a measurement of a particular adapter. The pseudo
terminal has no baud rate and no real timing. The simulated device is one that never
makes a mistake in its own firmware. A robot's first day on real hardware will find
something these did not, and what these buy is that it will not be one of the thirteen
things above.

## Use It

`rc/io/session.hpp` joins `rc::io`, and `rc/io/sim_device.hpp` is the device the rest of
the curriculum tests its drivers against.

```cpp
rc::io::Session session(opener);

// every tick
session.tick(now);
rc::io::Telemetry t;
if (session.latest(t)) act(t.position, t.sent_at);

const rc::io::Health h = session.health(now);
if (h.stale) stop_the_robot();
```

The honest limit is that a `Session` polls on the thread that calls `tick`. A caller
whose loop can run longer than the buffer lasts has lesson 08-08's problem again, and the
answer is the one from lesson 08-10: run the session on a thread that does nothing else.

## What Breaks First

- **Two loops reading one link.** A request that waits by polling eats telemetry. See
  `E-IO-0043`.
- **A loss counter read as the whole story.** The outage is in no counter. See
  `E-IO-0044`.
- **State from the connection before.** An abandoned request, a clock map, a counter
  that restarted. See `E-IO-0045`.

## Ship It

`rc/io/session.hpp` holds `Telemetry`, `SessionOptions`, `Health` and `Session`, and
`rc/io/sim_device.hpp` holds `SimDevice` and its port.

Phase 08 has what it set out to build: a robot that can talk to a board over a format
that is specified, framed, checked, read without being held up, asked and answered,
timed, and reopened when the cable comes out, tested end to end without owning any of the
hardware it will run on. What it does not cover is a different class of bus, CAN, SPI,
I2C, RS-485 with its direction control, and flow control, which is off throughout. They
share the shape of everything here and differ in the details that matter.
