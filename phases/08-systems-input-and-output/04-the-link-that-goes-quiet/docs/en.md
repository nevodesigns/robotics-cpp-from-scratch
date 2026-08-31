# The Link That Goes Quiet: Reads That Return Nothing, Writes That Do Not Finish

> The robot stopped. The log says the link was fine right up to the moment it
> was not, and it says that because the code was watching the wrong thing.

**Type:** Build
**Time:** about 150 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 08-03, 03-05

## The Problem

Three lessons have built a wire format: fields into bytes, a frame found in a
stream, a check that the frame is the one that was sent. Every one of them is a
pure function of the bytes, and every one is tested by handing it bytes.

A real link is not a pure function. It is a device that answers when it feels
like it, and the code around it has to survive four things that no format
lesson can prepare you for:

- **A read returns zero.** Not an error. Nothing had arrived yet.
- **A write accepts fewer bytes than you gave it.** The buffer was full.
- **The device stops talking.** Everything still returns successfully.
- **The device talks nonsense.** Everything parses, nothing is valid.

The last two are the dangerous ones, because a link that has died and a link
that is merely quiet look identical from inside a single call, and a link
delivering garbage looks *busier* than a healthy one.

## The Concept

### Zero is not an error, and not an end

```cpp
const int count = port.read(buffer, sizeof(buffer));
if (count <= 0) return Error::Disconnected;      // wrong
```

A non blocking read returns zero when nothing has arrived. On a link at 100 Hz
polled at 1 kHz, nine out of ten reads return zero, and every one of them is the
link working correctly.

Three outcomes, three meanings:

| return | meaning | what to do |
|---|---|---|
| positive | that many bytes arrived | parse them |
| zero | nothing has arrived yet | nothing, and it is not news |
| negative | the port itself failed | this one is an error |

Collapsing the first two is the fastest way to a link that reports failure on an
idle bus. Collapsing the last two is the slower and worse way to a link that
never reports failure at all.

### A write is a request, not an instruction

```cpp
port.write(frame.data(), frame.size());          // wrong
```

`write` returns how many bytes it took, and it is allowed to take fewer. The
kernel's buffer was nearly full, the device is not draining it, the frame was
larger than a single transfer. What goes out is a **truncated frame**, which is
exactly what the previous two lessons were about: the receiver either fails a
checksum or, worse, resynchronises onto the wrong byte and stays out of step.

Write in a loop until it is all gone, and treat running out of attempts as a
failure worth reporting rather than a partial success worth ignoring.

### Staleness is measured from the last good message, not the last read

This is the one that costs a robot.

```cpp
if (port.read(buffer, size) > 0) last_seen_ = now;    // wrong
```

That clock is fed by **bytes**, and bytes are not information. A link picking up
interference from a motor produces bytes all day. A device that has crashed and
is holding its line at a level that reads as a stream of zero bytes produces
bytes all day. Both feed the watchdog, and the watchdog says the link is
healthy while the controller acts on the last pose it ever got.

Feed the clock from the last thing that was actually a message: framed
correctly, checksum intact, decoded. Nothing else counts, because nothing else
is evidence that anybody is at the other end.

The same distinction turns the counters into a diagnosis. Bytes discarded,
frames received and checksums failed are three different numbers, and their
ratios say different things:

| what you see | what it means |
|---|---|
| discards high, frames zero | the baud rate is wrong, or nothing is connected |
| frames arriving, checksums failing | a physical problem: cable, ground, a motor |
| frames and checksums fine, nothing recent | the device is alive and has stopped sending |
| everything zero | you are not polling, or the port never opened |

A link that only reports "ok" or "failed" cannot tell you any of those, and the
difference is an afternoon.

### Polling takes a timestamp

`poll(now)` takes the time rather than reading a clock, for the same reason
`LoopMonitor` did in lesson 07-03 and `rate_limit` did in 01-02: a test can then
make an hour pass instantly, and the staleness policy becomes something you can
check rather than something you hope about.

## Build It

Implement in `exercise/solution.hpp`:

- `BytePort`, the interface: `read` and `write`, both returning a count, zero
  meaning nothing happened and negative meaning the port failed.
- `Link(port, storage, timeout)`, holding a frame parser and the counters.
- `poll(now)`, draining whatever has arrived and returning true when a valid
  message is ready. It must survive a read of zero, a read that splits a frame,
  several frames in one read, and a read that fails.
- `send(body, now)`, which frames, checksums and writes the whole thing however
  many attempts that takes.
- `stale(now)` and the four counters.

The fake port in the tests can be told how many bytes to return per read, how
many to accept per write, and when to fail, so every one of those cases is a
test rather than a hope.

```
rcpp verify 08-04
```

## Use It

This is the shape to write for anything that speaks bytes: a serial port, a
socket, a USB device, a radio. The platform specific part is small and lives
behind `BytePort`; everything above it is the same code, and everything above it
is testable without hardware.

That is also how to keep a driver honest when the hardware is on somebody else's
desk. Record a real session's bytes once, replay them into the fake for ever,
and the failure that happened at three in the morning becomes a test.

## What Breaks First

- **A read of zero treated as failure or as disconnection.** Nine reads in ten
  are zero on an idle link. See `E-IO-0008`.
- **A partial write left unfinished.** A truncated frame is worse than no
  frame, because it desynchronises the far end. See `E-IO-0009`.
- **A watchdog fed by bytes rather than by messages.** A dead link that is
  producing noise looks healthier than a healthy one. See `E-IO-0010`.

## Ship It

`Link` joins `rc::io`, and with it the phase has what it set out to build:
everything needed to talk to a device except the device. The next lesson opens a
real port, and the only new code is the twenty lines behind `BytePort`.
