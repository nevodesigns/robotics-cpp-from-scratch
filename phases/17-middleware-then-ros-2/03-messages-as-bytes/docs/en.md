# Messages as Bytes: A Layout Both Ends Agree On, and a Check That They Do

> A message is not a struct. It is bytes that two programs, written at different times, perhaps by different people on different machines, have to read the same way.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 17-01, 01-10, 08-01

## The Problem

So far the payload was a string and nobody asked what was in it. A pose is not a string: it is a few doubles. The easy way to send one is to copy the struct's memory into the message and copy it back out at the other end. It works on the machine it was written on, and then it stops working in three ways: the other compiler lays the struct out differently,
the other machine puts the bytes of a number in the other order, and the other program was built last month from a struct that has since gained a field.

Lesson 8-01 framed bytes on a wire. This is the same discipline for the messages of the bus, and the second half of it, checking that what arrived is what was expected.

## The Concept

### The layout is written down

The writer puts each field down in a fixed order with a fixed width, **little endian**, with nothing between the fields:

```cpp
w.put_u8(0xAB);  w.put_u16(0x1234);  w.put_u32(0xDEADBEEFu);
// AB 34 12 EF BE AD DE
```

The size is the fields added up: 1 + 2 + 4 = 7. A struct of a byte, a double and a 16 bit count is 1 + 8 + 2 = 11 on the wire, and **24** in memory on the compiler that built this lesson, because the compiler pads after the byte and at the end so that the double sits on an eight byte boundary. That padding is the compiler's business, not the wire's,
and another compiler or an other machine may lay it out differently. See `E-BUS-0007`.

A double is written as its eight bytes, copied through `memcpy` into an integer and out lowest byte first, and it comes back exactly: a double of -0.1234567890123456 and one of 1e300 came back equal, not near. A string is a two byte length and then the characters. A message of a u32, two doubles, a nine character string and an empty one was 33 bytes: 4 + 8 + 8 + (2 + 9) + 2.

### A reader that does not trust the length

Bytes arrive from outside. The reader checks, before every read, that there are enough left, and when there are not it returns zero and sets a flag that stays set. A message of a u16 and a u8 read as a u16, a u32, a u8: the first is fine, the second runs out, and the third does not hand out the byte that is there, because the read has already failed and what
follows would be nonsense that looks like data.

The worst case is a length that lies. A string whose length said 500 with two bytes behind it: the read failed, `ok()` was false, and the string was empty. A reader that trusted the 500 would read 498 bytes of whatever lies after the message. See `E-BUS-0008`.

### What this message is

A receiver should be able to tell whether the bytes it got are the message it expected. The **type id** is a number worked out from the type's name and version by FNV-1a, which is simple and the same on every machine (the published value for the string "a" is 0xE40C292C, which is how to test an implementation). Two names that differ by a letter, `Pose2D` and `Pose2d`, or by a version, `Pose2D/v2`,
give different ids. Changing the layout of a message means changing its name.

A **frame** is the type id, the payload's length, then the payload. Taking one apart gives one of four answers:

| frame | answer |
|---|---|
| as sent, for the type expected | ok |
| for another type | wrong_type |
| cut short by 3 bytes, or with an extra byte | bad_length |
| cut to 5 bytes | too_short |

The payload is only filled in when the answer is ok. A receiver that gets wrong_type has found out that two programs disagree on what a topic carries, which is better learned from a status than from a robot that drives to coordinates that are somebody else's velocity. See `E-BUS-0009`.

## Build It

`put_u16`, `put_u32`, `put_f64`, `put_string`, `get_u16`, `get_u32`, `get_f64`, `get_string`, `type_id`, `frame` and `unframe` in `exercise/solution.hpp`. `put_u8`, `get_u8` and the bounds check are given.

```
rcpp verify 17-03
```

The doubles are checked for equality and not nearness, and the type id against a published value.

## Use It

`rc/bus/message.hpp` joins `rc::bus`.

```cpp
ByteWriter w;
w.put_f64(pose.x);  w.put_f64(pose.y);  w.put_f64(pose.theta);
bus.publish("pose", to_string(frame(type_id("rc/msg/Pose2D"), w.bytes())));
```

**What this does not do.** It is a fixed layout with no way to add a field to a message that is already out there, which is what schema evolution is, and ROS 2 solves it with generated code and the DDS type system and not with a hand written writer. It has no check on the bytes beyond the length: a flipped bit in a double is a different double (lesson 15-13 added a CRC for that). It writes little endian always and so does not suit
a protocol that is big endian by agreement, and it does no compression. The type id is 32 bits, so two different names can collide, which is very unlikely among a robot's messages and not impossible.

## What Breaks First

- **A struct copied as it sits in memory.** The size, the padding and the byte order are all the compiler's and the machine's. See `E-BUS-0007`.
- **A length trusted.** A read that runs on into whatever follows. See `E-BUS-0008`.
- **A message with no name.** Two programs that disagree and neither knows. See `E-BUS-0009`.

## Ship It

`rc/bus/message.hpp` holds `ByteWriter`, `ByteReader`, `type_id`, `FrameStatus`, `frame` and `unframe`.

The bus can now carry a message that means the same thing at both ends, or say that it does not.
