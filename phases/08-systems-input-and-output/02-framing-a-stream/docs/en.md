# Framing: A Stream Has No Messages In It

> `read` returned 7. Seven is not a number of messages. It is a number of bytes,
> and which bytes they are depends on timing you do not control.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 08-01

## The Problem

The last lesson got the bytes of a message right. This one is about the fact
that the message does not arrive.

Here is the code almost everybody writes first, and it passes every test:

```cpp
std::uint8_t buffer[64];
const int count = port.read(buffer, sizeof(buffer));
handle_message(buffer, count);
```

On a desk, at a low rate, with short messages, that works for days. Then one of
these happens, and all of them will:

- `read` returns **half a message**, because the rest had not arrived yet.
- `read` returns **two messages**, because both arrived while you were busy.
- `read` returns **one byte**, because that is genuinely all there was.
- `read` returns a message with **noise in front of it**, because the cable was
  plugged in halfway through a transmission.

None of these is an error. Every one of them is the serial port working
correctly. A stream delivers bytes in order, and that is the entire promise: no
boundaries, no grouping, no relationship between one `read` and one message.

The boundaries have to be put there by you, at the sending end, and found again
by you, at the receiving end. That is framing, and it is the difference between
code that works on a desk and code that works on a robot.

## The Concept

### Three ways to mark where a message ends

**A delimiter.** End every message with a byte that means "that was the end",
usually a newline. Simple, readable in a terminal, and wrong the moment the
payload can contain that byte. Binary data contains every byte, so the delimiter
has to be escaped, and now you have written half of COBS and have a payload
whose length changes depending on its contents.

**A length prefix.** Say up front how many bytes follow. No escaping, and the
receiver knows exactly how much to wait for. The cost is that the length itself
must arrive intact, and a corrupted length is worse than corrupted data because
it puts the parser out of step with the stream rather than producing one bad
message.

**A start byte, then a length.** What most real protocols do, and what this
lesson builds. The start byte gives you somewhere to resynchronise to when
things go wrong, and the length keeps the payload unescaped.

```text
  AA  05  48 65 6C 6C 6F
  ^   ^   ^
  |   |   five bytes of payload
  |   how many follow
  start of frame
```

### Resynchronisation is the feature

A parser that gives up when it sees something unexpected is useless in the
field, because the first plugged cable ends the session. The parser's job is to
get back in step, and the only tool it has is the start byte: throw bytes away
until one of them is the start byte, then try again.

That has an honest cost worth knowing. **A start byte can appear inside a
payload**, so after a resynchronisation the parser may lock onto a false start
and produce one garbage frame before finding the real rhythm. A checksum is what
catches that, which is the next lesson. Framing narrows the problem; it does not
close it.

The number of bytes discarded is worth counting and reporting. A link that
discards a few bytes at startup is normal. A link discarding thousands is
telling you the baud rate is wrong, and no amount of staring at the payloads
will say so.

### The length field is not your data

This is the one that gets written up as a security advisory.

```cpp
const std::uint8_t length = buffer[1];
read_exactly(payload, length);          // payload is 64 bytes; length can be 255
```

The length arrives from outside. It comes from a device that might be faulty, a
cable picking up interference from a motor, or nothing at all. It is not a fact
about your message, it is a claim, and it is a claim that indexes your memory.

Check it against the space you have **before** using it, and treat a length that
does not fit as a framing failure rather than a truncation. A frame that does
not fit is not a frame you understood.

### One byte at a time

The parser in this lesson takes one byte and returns whether that byte completed
a frame. That shape looks less convenient than one that takes a buffer, and it
is the one to write, for two reasons.

It cannot be given a whole message by accident, so the awkward cases are not
special cases: a byte at a time and a thousand at a time go down the same path,
and the tests can split the input anywhere.

And it is the shape an interrupt handler needs, because that is what a
microcontroller hands you: one byte, when it arrives.

## Build It

Implement `FrameParser` in `exercise/solution.hpp`.

- `FrameParser(start_byte, storage)`, where storage is a caller supplied buffer
  that bounds the largest payload accepted. It never allocates.
- `push(byte)`, returning true when that byte completed a frame.
- `payload()`, the bytes of the frame just completed, valid until the next
  `push`.
- `discarded()`, bytes thrown away while hunting for a start byte.
- `oversized()`, frames refused because the length did not fit.

```
rcpp verify 08-02
```

The tests feed the same frames split in every awkward way: one byte at a time,
two frames in one chunk, a frame split across the length field, garbage in
front, and a truncated frame followed by a good one.

## Use It

This is the receiving half of every serial protocol in the rest of the
curriculum, and the shape you will recognise in other people's: a start byte, a
length, a payload, and in the next lesson a checksum.

When you meet a datasheet that specifies a frame, this is where its description
turns into code, and the part worth copying is not the parser but the tests
around it.

## What Breaks First

- **Assuming one read is one message.** The stream is under no obligation, and
  the failure is a timing problem that a desk cannot reproduce. See `E-IO-0003`.
- **Trusting the length field.** It arrives from outside and it indexes your
  memory. See `E-IO-0004`.
- **Writing the payload before checking it fits.** The bounds check comes
  first, not after. See `E-CPP-0007`.

## Ship It

`FrameParser` joins `rc::io` beside the encoder. Together they are a wire
format: one end writes fields into bytes, the other finds the message in the
stream and reads the fields back out.
