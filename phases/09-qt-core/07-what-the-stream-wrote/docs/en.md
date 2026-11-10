# What the Stream Wrote: Byte Order, a Float That Took Eight Bytes, and a String That Is Not the Bytes You Meant

> A QDataStream is the easy way to turn values into bytes in Qt, and its defaults suit Qt talking to Qt.
> A device with a data sheet is a different listener.

**Type:** Build
**Time:** about 150 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 09-06, 08-01

## The Problem

Lesson 08-01 wrote a frame by hand, one byte at a time, because a struct cannot be sent down a wire as it is in memory. Qt offers a shortcut, `QDataStream`: write values into it with `<<`, read them back with `>>`, and the bytes take care of themselves.

They do, for another Qt program. For a device that follows a data sheet the bytes have to be exactly the ones the sheet says, and the stream's defaults are not.

## The Concept

### What it writes by default

The suite wrote four small things and printed the bytes:

| written | bytes |
|---|---|
| the 16 bit number 0x0102, default | `01 02` |
| the same, little endian | `02 01` |
| the float 1.5, default | `3f f8 00 00 00 00 00 00` (**8** bytes) |
| the float 1.5, single precision | `3f c0 00 00` (**4** bytes) |
| the string "ab" | `00 00 00 04 00 61 00 62` (**8** bytes) |

Big endian by default; a `float` as a double by default; a string as a four byte length in bytes followed by UTF-16. Two Qt programs agree on all of it and a device does not. See `E-QT-0030`.

### A format written down once

```
bytes 0 to 1    the id, a 16 bit number, least significant byte first
bytes 2 to 9    the value, an IEEE 754 double, least significant byte first
byte  10        the length of the name in bytes, 0 to 255
bytes 11 on     the name in UTF-8, that many bytes
```

`encode_reading` says each choice on the stream, writes the length byte and the name's bytes itself, and refuses a name over 255 bytes. An id of 258, a value of 1.5 and the name micro-s came out as **14** bytes:

```
02 01 00 00 00 00 00 00 f8 3f 03 c2 b5 73
```

An empty name made 11 bytes, and a name of 255 bytes made 266. See `E-QT-0031`.

### Reading is the hard half

A stream that runs out of bytes does not throw. It sets a status, and the values read after that are set to zero. Two bytes read as two 16 bit numbers gave 0x0102 with status Ok, then **0** with status `ReadPastEnd`: not the 88 the variable held before, zero. A decoder that does not look at the status accepts a truncated frame as a reading of zeros.

`decode_reading` asks three questions: did the fixed part read without error, were all the name bytes really read, and is the stream at its end. Every one of the **14** prefixes of the frame, from 0 to 13 bytes, was refused. One byte too many was refused. A length byte of 200 on a short frame was refused. The whole frame was accepted, and a value that is not exact in decimal, 0.1 + 0.2, came back exactly. See `E-QT-0032`.

## Build It

`encode_reading` and `decode_reading` in `exercise/solution.hpp`. The `Reading` and `DecodedReading` types are given.

```
rcpp verify 09-07
```

## Use It

`rc/qt/frame.hpp` joins `rc::qt`. Its reading type there is `FrameReading`.

```cpp
const QByteArray bytes = encode_reading(reading);
const DecodedReading back = decode_reading(received);
if (!back.ok) { /* count it, drop it, do not act on it */ }
```

**What this does not do.** There is no checksum, which lesson 08-03 covers, so a frame with a flipped bit in the value is accepted. There is no framing, so it does not say where one frame ends and the next begins on a stream (lesson 08-02). The version of the stream is not set. The types used here are integers, a double and raw bytes; if you ever stream another kind of type, set the version on both ends
and test it. And the text is whatever UTF-8 the name's bytes happen to be: invalid bytes become the replacement character, as lesson 09-06 found.

## What Breaks First

- **The stream's defaults on a data sheet.** The wrong byte order, and four bytes too many. See `E-QT-0030`.
- **A streamed `QString`.** A length of four bytes and UTF-16. See `E-QT-0031`.
- **A read that never asks the status.** A truncated frame becomes zeros. See `E-QT-0032`.

## Ship It

`rc/qt/frame.hpp` holds `FrameReading`, `encode_reading`, `DecodedFrameReading` and `decode_reading`.

Lesson 08-01 said a struct cannot be sent as it is. This is the shortcut that can be, once it has been told every choice.
