# Rescan Instead of Giving Up: Getting the Next Frame Back

> A parser that discards a frame when its checksum fails also discards the next
> one. Keep the bytes, give up one, and look again.

**Type:** Build
**Time:** about 150 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 08-06

## The Problem

Lesson 08-06 priced a lost byte, and the price was wrong in the direction nobody
guesses. A flipped bit cost one frame. A dropped byte cost **nearly two**, 1.89
frames for each one in the light case, and the checksum had done nothing wrong.

Where does the second frame go? Read what the parser does:

```text
AA 0A  01 02 03 04 05 06 07 08  C7     a frame, ten bytes long on the wire
```

It reads the start byte, reads the length, and then takes that many bytes. If one
byte went missing from the middle, the parser is still short when the frame
should have ended, so it takes the shortfall from what comes next, which is
`AA 0A ...`, the start of the next real frame. The checksum fails and the
parser throws all of it away.

The next frame lost its first byte to the **repair attempt**.

## The Concept

### Keep what you took

The fix is to stop treating a failed frame as used up. A failed checksum means the
bytes do not form a frame **starting at this start byte**. It says nothing about
whether they contain one starting somewhere later.

So the parser holds every byte it has taken until a frame is proven good. When the
checksum fails it gives up one byte, the start byte, and runs the same search
again over what is left. That search skips to the next start byte, and if one is
sitting inside the bytes just read, the next frame is recovered.

Measured, over fifty thousand frames:

| parser | dropped | lost frames | lost per drop |
|---|---|---|---|
| discard the candidate, one byte in 500 | 1144 | 2158 | 1.89 |
| **rescan**, one byte in 500 | 1144 | 1146 | **1.00** |
| discard the candidate, one byte in 100 | 5531 | 9253 | 1.67 |
| **rescan**, one byte in 100 | 5531 | 5263 | **0.95** |

A lost byte now costs the frame it was in and nothing else. That is the least it
can cost.

The price is memory and a copy, since the parser holds a window of bytes and
`memmove` shifts it. It is about 260 bytes of window and nothing is allocated.
When the byte after a failed start is not a start byte, which is almost always,
the extra work is one comparison.

### Two frames inside one false claim

A rescan can find more than one frame in what it already holds. Take a false
start byte followed by a length of 36, then two real frames that together fit
inside those 36 bytes. The parser takes 38 bytes for a frame that does not exist,
fails the checksum, and finds the first real frame by rescanning. The second is
sitting complete in the window.

If the only way to look at the window again is to push another byte, the second
frame waits for one. On a device that sends a burst and goes quiet, the last
frame of every burst is delivered when the next burst begins. So there is a
second entry point:

```cpp
bool ready = parser.push(byte);
while (ready) {
  deliver(parser.payload());
  ready = parser.next();   // look again at what is held, with no new byte
}
```

See `E-IO-0024`.

### Claims from outside are checked before they are used

The length byte comes off a wire. In a parser that holds bytes it also decides how
many to wait for, so a length of 200 in a window of 64 would wait for bytes it has
no room to keep. The parser checks the claim against the window and against the
size of the check before it uses it, counts what fails as `malformed`, and gives up
the start byte.

The noise that tests this is a stream of nothing but start bytes, where every
position looks like the beginning of a frame. The test sends ten thousand bytes of
it and requires that no frame comes out and the window is never overrun. See
`E-IO-0023`.

### A wider check, measured

Rescanning gets the frames back. It does not make a wrong frame impossible, and
it can add a few, because it examines more candidates and each is another chance
for a checksum to pass by luck.

The same stream, one byte in 100 dropped, fifty thousand frames:

| check | misaligned frames rejected | passed with the wrong content |
|---|---|---|
| CRC-8 | 4913 | 13 |
| CRC-16 | 5431 | 0 |

Eight bits let one in 256 through. Sixteen let one in 65536, which is 256 times
fewer, for one more byte on every frame: ten percent more on a ten byte frame.

Whether that is worth it is a decision for the link, not a rule. The measured
CRC-8 figure is 13 wrong frames in 50000, one in 3846, which at a thousand frames
a second is a wrong frame about every four seconds on a link dropping one byte in
a hundred. On a cable that drops far less the figure falls with it. The point is
that you can now compute it instead of hoping.

## Build It

Implement `step()` in `exercise/solution.hpp`. The class around it is given:
`push`, `next`, the window, and the counters.

The comments in `step()` list the six moves. The two that matter:

- Check the length **before** using it, against `check_bytes_` and `longest_`.
- On a failed checksum, give up **the start byte alone**, count it in
  `checksum_failures_` and `rescans_`, and go round again.

```
rcpp verify 08-07
```

The first tests are the parser's basic behaviour, including that a frame sent by
the `Link` from lesson 08-04 is read by this parser. Then the hand built cases:
a frame short one byte followed by two whole ones, and two frames hidden in a
false claim. The last two are the measurements above.

## Use It

`RescanParser` joins `rc::io`. It does not replace `FrameParser`: that one is one
byte of state and no memory, which is right for a microcontroller with nothing to
spare. This one is for the host, which has a window to spare and frames to save.

To put it under a link, call `push` for each byte a read returns and drain with
`next`. The `Link` from lesson 08-04 still does the reading, counting and timing;
this replaces only the thing that finds frames in the bytes.

## What Breaks First

- **A parser that discards what it checked.** One lost byte costs two frames. See
  `E-IO-0022`.
- **A parser that holds bytes without a limit.** An unchecked length waits for
  bytes the window cannot keep. See `E-IO-0023`.
- **A second frame that waits for a byte.** A rescan can find two frames and only
  one is returned until something else arrives. See `E-IO-0024`.

## Ship It

`rc/io/rescan.hpp` holds `Check` and `RescanParser`.

Lesson 08-08 turns to a different problem. Nothing has been lost so far, only
damaged, but a device can also send faster than the host reads, and then bytes are
lost without anything having gone wrong with the wire at all.
