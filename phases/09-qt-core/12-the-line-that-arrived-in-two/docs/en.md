# The Line That Arrived in Two Pieces: Reading Lines From a Device That Gives You Whatever It Has

> A device does not hand over lines. It hands over what has arrived, and the cut falls anywhere.
> A reader that does not know that works on the bench and fails in the field.

**Type:** Build
**Time:** about 150 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 09-07, 01-12

## The Problem

Lesson 09-07 framed bytes with a length. Many devices frame with a newline instead: a serial port, a child process's output, a log. What `read` returns is a chunk, and a chunk is not a line.

## The Concept

### Chunks

33 bytes of two lines, read 7 at a time from a `QBuffer`, were 3 chunks. Splitting each chunk at the newline gave **4** pieces for 2 lines, and the first line was never whole. See `E-QT-0045`.

### Every cut

`LineAssembler` keeps the part of a line that has not ended. The same 33 bytes cut every way into two chunks and into three, **629** ways, gave the same lines in all of them, and so did one byte at a time.

### The carriage return

A line written `\r\n` and split at `\n` ends in `\r`: the first line of `a\r\nb` is **2** bytes. The assembler removes one `\r` before the `\n` and keeps a `\r` inside a line. `a\r\nb\n\r\nc\rd\n\n` gave **5** lines, among them an empty one and `c\rd`. See `E-QT-0047`.

### A line with no end

With a limit of 8 bytes, 100000 bytes with no newline left **0** held and no lines returned. When its newline arrived the line was counted as dropped, and `next` after it was read normally. A line of exactly 8 was kept. See `E-QT-0046`.

## Build It

`LineAssembler::feed` in `exercise/solution.hpp`.

```
rcpp verify 09-12
```

## Use It

`rc/qt/line_assembler.hpp` joins `rc::qt`.

```cpp
rc::qt::LineAssembler lines(256);
connect(port, &QIODevice::readyRead, this, [&] {
  for (const QByteArray& line : lines.feed(port->readAll())) handle(line);
});
if (lines.dropped() > 0) { /* say that noise was seen */ }
```

**What this does not do.** It splits on `\n` and knows nothing of encodings: the bytes of a line are not text yet (lesson 09-06). It does not time out a line that has begun and stalled, so a device that sends half a line and stops leaves it held. A dropped line is counted, and which bytes it held is not kept. And a binary protocol needs a length and not a newline (lesson 09-07).

## What Breaks First

- **A line cut by the read.** See `E-QT-0045`.
- **Noise with no newline.** See `E-QT-0046`.
- **A `\r` that stayed.** See `E-QT-0047`.

## Ship It

`rc/qt/line_assembler.hpp` holds `LineAssembler`.

Lesson 09-07 asked for a length so that a reader knows where a message ends. This is the reader for the device that uses a newline instead, and that must never be fooled by where the read happened to stop.
