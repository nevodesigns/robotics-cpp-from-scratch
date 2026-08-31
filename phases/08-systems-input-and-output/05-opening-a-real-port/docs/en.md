# Opening a Real Port: The Twenty Lines That Decide Everything

> The port opened. The device is sending. Your parser has never seen a valid
> frame, and the reason is a line of configuration you did not write.

**Type:** Build
**Time:** about 150 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** none required, see below
**Prerequisites:** 08-04, 02-03

## The Problem

Four lessons have built everything above the wire: fields into bytes, frames
found in a stream, a checksum, and a link that survives partial reads, partial
writes and silence. All of it sits on `BytePort`, which has two functions.

What is left is opening the port. It is about twenty lines, and they are the
twenty lines where this goes wrong, because the obvious version works
perfectly for text and silently destroys binary data.

```cpp
const int fd = ::open("/dev/ttyUSB0", O_RDWR);   // opened, and unusable
```

That port is now in the state a terminal is in when a person is typing at it.
It waits for a whole line before giving you anything. It removes some bytes and
rewrites others. It treats two byte values as flow control and two more as
signals, and one of them erases the byte before it.

Your frames contain all of those values, because your frames contain numbers.

## The Concept

### What the default settings actually do

Not a warning: a measurement. Each of these was sent through a terminal left at
its defaults, and this is what came out the other side.

| sent | received | what happened |
|---|---|---|
| `68 69 0A` | `68 69 0A` | text with a newline, which is what this mode is for |
| `68 69 0D` | `68 69 **0A**` | the carriage return was rewritten |
| `68 **11** 69 0A` | `68 69 0A` | 0x11 is XON, taken as flow control |
| `68 **13** 69 0A` | `68 69 0A` | 0x13 is XOFF, taken as flow control |
| `68 **03** 69 0A` | `69 0A` | 0x03 is interrupt, and it discarded what came before |
| `68 **1A** 69 0A` | `69 0A` | 0x1A is suspend, likewise |
| `68 **04** 69 0A` | `68 69 0A` | 0x04 is end of file |
| `68 58 **7F** 69 0A` | `68 69 0A` | 0x7F is erase, and it deleted the 0x58 |
| `68 69` | *nothing* | no newline, so nothing is delivered at all |

Six of the nine lose or change data. Now put a frame through it:

```text
sent      AA 04 20 21 22 23 5C          a four byte payload
received  AA
```

Read that twice. The payload contained nothing unusual: four ordinary bytes and
a checksum. What destroyed the frame was **the length field**, because the
payload was four bytes long and four is the end of file character.

A payload of three bytes is Ctrl-C. Seventeen is XON. Nineteen is XOFF.
Twenty six is Ctrl-Z. The message that fails is decided by how long it is, and
nothing in your code mentions its length, which is why this presents as a device
that works except sometimes.

One line prevents all of it:

```cpp
cfmakeraw(&settings);
```

Raw means: deliver bytes as they arrive, change nothing, interpret nothing. With
it, every row of that table arrives exactly as it was sent. It is the difference
between a terminal and a wire.

Windows has the same trap under different names. `fInX` and `fOutX` are XON and
XOFF. `fBinary` must be `TRUE`, and `SetCommState` fails if it is not, which is
the tidiest way a platform has ever said that text mode on a serial port was a
mistake.

### Opening is not configuring

Two flags on the open itself, both of which fix a hang rather than a corruption.

`O_NOCTTY` says this port is not the process's controlling terminal. Without it,
a Ctrl-C byte arriving from the device can deliver a signal to **your** program.
A robot that stops because a sensor sent the number three is a memorable
afternoon.

`O_NONBLOCK` says do not wait for carrier detect. Without it, `open` waits for a
modem signal on a port where nothing will ever assert one, and the program hangs
before running a line of its own code.

### VMIN and VTIME decide what a read means

Two numbers control when a read returns:

| VMIN | VTIME | a read returns |
|---|---|---|
| 0 | 0 | immediately, with whatever is there, possibly nothing |
| n | 0 | when n bytes have arrived, however long that takes |
| 0 | t | when a byte arrives, or after t tenths of a second |
| n | t | when n bytes arrive, or t tenths after the first one |

A polled link wants the first row. Any other row means a control loop can be
made to wait by a device that has stopped talking, which is the failure mode
lesson 08-04 spent its time avoiding.

Windows spells this `COMMTIMEOUTS` with `ReadIntervalTimeout` set to `MAXDWORD`
and both totals zero, which is its documented way of asking the same question.

### tcsetattr succeeds when it did not do what you asked

```cpp
tcsetattr(fd, TCSANOW, &settings);   // returned 0
```

POSIX says `tcsetattr` returns success if it applied **any** of the settings it
was given. A baud rate the hardware cannot do is not an error; it is a different
baud rate, applied silently, and the symptom is a link where every frame fails
its checksum.

Read the settings back and check the ones you care about. It is four lines and
it converts a mystery into an error code.

### The port must be released on every path

A descriptor left open stays claimed until the process dies, and the next run
reports the port busy. That is `RAII` from lesson 02-03, and this is the case it
was for: acquire in the constructor, release in the destructor, and delete the
copy so that two objects cannot each close one port.

Exclusive access matters too. Two programs holding the same port each receive a
random half of the bytes. It does not look like a mistake anybody made; it looks
like a device that has become unreliable.

## Build It

Implement `SerialPort` in `exercise/solution.hpp`. It derives from
`rc::io::BytePort`, so a working one drops straight into the `Link` from the
last lesson.

- `open(device, baud)`, returning a `SerialError` that says which of the
  half dozen things went wrong.
- `read` and `write`, where **nothing to read is zero, not an error**.
- `close`, and a destructor that calls it, and a deleted copy.

```
rcpp verify 08-05
```

## Hardware, and what is actually tested

No hardware is needed to do this lesson.

On Linux and macOS the tests open a **pseudo terminal**, which is a real
terminal that `termios` configures exactly as it configures a serial port. Every
line of the implementation runs: the open flags, `cfmakeraw`, VMIN and VTIME,
the read back check, the error paths, and a complete `Link` carrying framed
messages over it. The measurements in the table above were made this way.

On Windows the code is compiled on every build and its error paths are run, but
**no port is opened**, because a virtual COM port needs a driver that continuous
integration cannot install. That is a real asymmetry and it is written down
rather than glossed: the Windows implementation is reviewed and compiled, not
exercised.

With about ten dollars of hardware you can close that gap yourself. Take a USB
serial adapter, connect its TX pin to its RX pin with a single wire, and run the
same tests against `/dev/ttyUSB0` or `COM3`. Everything the port sends comes
straight back, which is enough to exercise the whole implementation on the real
driver on either platform.

## Use It

This is the last piece. `SerialPort` plus `Link` plus the encoder is a complete
conversation with a microcontroller, and everything except this file is
platform independent and tested without hardware.

The same shape works for a socket, a USB device or a radio: put the platform in
one small class behind `BytePort` and keep everything else above it.

## What Breaks First

- **A port left in the terminal's default mode.** It works for text and
  destroys binary, and which messages it destroys depends on their length. See
  `E-IO-0011`.
- **A read that blocks.** VMIN above zero, or Windows timeouts left at their
  defaults, and the control loop waits on a device that has stopped talking.
  See `E-IO-0012`.
- **A configuration that reported success without applying.** `tcsetattr`
  returns zero if it applied any of it. See `E-IO-0013`.

## Ship It

`SerialPort` joins `rc::io`, and phase 08 has what it set out to build: a robot
that can talk to a board, over a format that is specified, framed, checked, and
tested end to end without owning any of the hardware it will eventually run on.
