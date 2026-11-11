# The Conversation as a Test: Golden Transcripts of a Device Session, and Ignoring Exactly What Should Differ

> The simplest test of a device driver is the one nobody writes: say exactly this, and expect exactly that, and fail if the conversation changes.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 18-09, 01-10, 01-05, 03-02, 01-06

## The Problem

Phase 8 talked to devices in bytes, and lesson 5 gave them fakes. A driver is tested one reply at a time, each with a hand written expected value, and those tests are a good start and miss the thing that breaks in practice: the conversation as a whole. A firmware update adds a byte to every reply and every test written around the old length still passes against the fake and fails on the device. A refactoring changes the order of two requests and the device is
fine with it, until it is not.

A **golden transcript** records a whole session, once, against the real device or a trusted emulator, as a file. A test replays the driver and compares what it says and hears with the file. It is the cheapest test there is to write, since it is recorded and not composed, and it has two jobs this lesson is built around: being readable by a person who reviews it, and not failing for reasons that do not matter.

## The Concept

### A transcript as text

Each message is a line: `>` for what was sent to the device, `<` for what came back, and the bytes as two digit hexadecimal numbers.

```
> 01 02
< 81 02 AB 00 FF
>
```

The last is an empty message, just the arrow. The text read back as it was written, message for message, and wrote itself out identically. That makes it a file in the repository, reviewed in a pull request like source. A session of 3 requests to the test device is **6 lines**, and fits on a screen. See `E-SIM-0028`.

### Typed by hand

A transcript is edited by people, to add a case or to remove a reply that was noise, so the reader is careful. Comments (`#`), blank lines and lower case digits are accepted. A bad line is refused with its number, and the read leaves the transcript **empty**, so a half read golden file is never used as if it were whole:

| text | answer |
|---|---|
| `? 02` | line 2: a line starts with > or < |
| `> 02 3` | line 2: bytes are two hexadecimal digits each |
| `< 0G` | line 2: bytes are two hexadecimal digits each |
| `> 0102` | line 1: bytes are two hexadecimal digits each |

The last is the subtle one: two bytes with no space between them look like one number. See `E-SIM-0028`.

### The first difference

`compare` walks the two sessions message by message and reports the **first** difference and where it is, because after the first one everything that follows is out of step and a second report is noise:

| kind | meaning |
|---|---|
| direction | message 2 went the other way |
| length | the messages differ in size, from this offset |
| content | a byte differs, here |
| extra | the new session has more messages, from here |
| missing | the new session stopped early |

A byte changed in message 3 was content at message 3, byte 6; a message sent the other way was direction at 2; one message more was extra from 6, and two fewer missing from 4. See `E-SIM-0029`.

### What is allowed to differ

Replay the same session tomorrow and the device's time of day is different: bytes 2 to 5 of every reply are a clock. Compared exactly, the test fails every day. A **mask** names bytes that may differ, in one direction:

```
the same session at two times of day: without a mask the first difference is message 1 byte 2; with the time masked: none
```

Three things about the mask matter. It covers **only** the direction it was given for: a change to byte 1 of a request (the host's side) was still caught with the time of the replies masked. It covers **only** those bytes: a reading changed at byte 6, one past the masked time, was caught at message 1, byte 6. And it is small: a mask over a whole message would pass anything. A test that masks too much has stopped being a test. See `E-SIM-0030`.

### What it catches

Against a golden session of 5 requests:

| change | difference found |
|---|---|
| new firmware adds a byte to every reply | length, message 1, offset 8 |
| a reading one too high on request 3 | content, message 5, byte 6 |
| the same conversation at another time of day | none |

The first is a protocol change no one announced, caught at the first reply and at the byte where the new one is longer. The second is a one-in-a-hundred bug, caught at the message and the byte. The third is not a change at all, and the mask says so. A transcript of two versions of a session differs in **one line** when one reading is different:

```
golden: < 81 02 0A 04 00 00 6A 00
now:    < 81 02 0A 04 00 00 6B 00
```

which is what a reviewer sees in the diff. See `E-SIM-0029`.

## Build It

`to_text`, `from_text`, `masked` and `compare` in `exercise/solution.hpp`. The message types and `add`, `size` and `at` are given.

```
rcpp verify 18-10
```

The device in the tests is deterministic, so the figures above are the same on every run.

## Use It

`rc/sim/transcript.hpp` joins `rc::sim`.

```cpp
rc::sim::Transcript golden;  std::string error;
rc::sim::Transcript::from_text(read_file("sessions/read_five.txt"), golden, error);
const auto diff = rc::sim::compare(golden, recorded, {{rc::sim::Direction::from_device, 2, 4}});
if (diff.kind != rc::sim::Difference::Kind::none) report(diff.message, diff.offset);
```

**What this does not do.** It compares bytes and knows nothing of what they mean: a reply that is correct but laid out differently fails, and so does a reordering that the device does not care about. It has no notion of time, so a session that was right but took ten times as long passes. It compares the whole conversation or none: one run cannot say "the first three are as before and the rest are new". And a golden file is only as good as the
session it was recorded from: a transcript recorded against a buggy device keeps the bug.

## What Breaks First

- **A transcript nobody can read.** A binary file that no one reviews. See `E-SIM-0028`.
- **A test that compares everything exactly.** Failing every day on the clock. See `E-SIM-0030`.
- **A mask over too much.** A test that passes whatever the device says. See `E-SIM-0030`.
- **A comparison that reports all the differences.** Noise after the first. See `E-SIM-0029`.

## Ship It

`rc/sim/transcript.hpp` holds `Direction`, `Message`, `Mask`, `Transcript`, `Difference`, `masked` and `compare`.

A driver can now be tested against a conversation recorded once, with a mask over the bytes that are supposed to change.
