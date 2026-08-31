# Checksums: What They Catch, Measured

> Everybody knows a CRC is better than a sum. Almost nobody can say at what, or
> by how much, and the two questions have surprising answers.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 08-02

## The Problem

The last lesson ended on an admission. A start byte can appear inside a payload,
so after resynchronising, the parser can lock onto a false start and hand you a
frame that was never sent. The framing narrowed the problem and did not close
it.

There is a second, more ordinary reason to care. Serial lines are analogue.
A motor switching, a badly grounded connector, a cable run alongside a power
lead: any of these flips bits, and none of them announces itself. The message
arrives, it parses, the numbers are wrong, and the robot acts on them.

A checksum is the answer, and the interesting question is not whether to have
one. It is which one, and the usual reasoning is folklore. This lesson replaces
it with measurements.

## The Concept

### Four candidates

**A sum.** Add every byte, keep the low eight bits. One line, and the most
common thing in hobby protocols.

**An exclusive or.** The same shape, with xor instead of addition. Slightly
cheaper, and popular in datasheets.

**CRC-8.** Treat the message as a polynomial over the field with two elements
and take the remainder after dividing by a chosen generator. That sentence
sounds like it costs something; it is a shift and a conditional xor, eight times
per byte, and a table makes it one lookup.

**CRC-16.** The same, sixteen bits wide.

### What each one actually catches

The following is measured, not quoted. The programs are the tests in this
lesson, and the numbers come from running them.

**One bit flipped**, over 2000 random eight byte messages, every bit position:

| scheme | caught |
|---|---|
| sum8 | 100% |
| xor8 | 100% |
| crc8 | 100% |
| crc16 | 100% |

All four. If a single flipped bit is your whole threat model, this decision does
not matter and you should stop reading and use the sum.

**Two bits flipped**, every pair of positions:

| scheme | caught | missed |
|---|---|---|
| sum8 | 93.75% | one in sixteen |
| xor8 | 88.89% | one in nine |
| crc8 | 100% | none |
| crc16 | 100% | none |

The two cheap schemes have a hole, and it is not small. Two changes that cancel
each other are invisible to a sum, and two flips in the same bit position of any
two bytes are invisible to an exclusive or.

**Two bytes swapped**, no bits changed at all:

| scheme | caught |
|---|---|
| sum8 | **0%** |
| xor8 | **0%** |
| crc8 | 99.65% |
| crc16 | 100% |

Zero. Not a small number: a certainty. Addition and exclusive or are both
commutative, so the order of the bytes cannot affect the result, and no amount
of making the checksum wider will change that. It is a property of the
arithmetic, not of the implementation.

This is the row that matters for the previous lesson, because a parser that has
locked onto a false start byte does not deliver corrupted bytes. It delivers
**correct bytes in the wrong places**, which is the one thing an order
independent checksum cannot see.

**A burst**, a run of whole bytes replaced by noise, which is what a motor
switching for a few microseconds does. Three million sixteen byte messages, and
here the raw miss counts say more than percentages:

| burst | sum8 | xor8 | crc8 | crc16 |
|---|---|---|---|---|
| 1 byte | 0 | 0 | 0 | 0 |
| 2 bytes | 11669 | 11681 | 11753 | **0** |
| 3 bytes | 11643 | 11821 | 11801 | 40 |
| 4 bytes | 11705 | 11727 | 11722 | 48 |
| 8 bytes | 11817 | 11607 | 11648 | 51 |

Read the columns rather than the rows and two rules fall out.

**Below the width of the checksum, a CRC is a guarantee.** CRC-16 missed exactly
zero two byte bursts out of three million. That is not luck; a CRC of width n
detects *every* burst of n bits or fewer, by construction. CRC-8 does the same
for one byte.

**Above that width, everything is chance, and only the width matters.** Every
eight bit scheme, clever or not, missed about 11700 in three million, which is
one in 256. Sum, exclusive or and CRC-8 are indistinguishable there. CRC-16
missed about 45, which is one in 65536. The polynomial stopped mattering; the
number of bits took over.

### So which one

- If reordering or a framing desync is possible, and after the last lesson you
  know it is, **do not use a sum or an exclusive or**. That failure has
  probability one.
- Choose the **width** for the residual risk you will accept: one in 256 is a
  wrong message every few minutes on a busy link, one in 65536 is a wrong
  message every few hours.
- Given a width, choose a **standard** CRC rather than inventing one, so the
  device at the other end can be told which by name.

### Getting a CRC right is a matter of agreeing, not of being clever

Every CRC has a polynomial, a starting value, whether the bits are reflected on
the way in and out, and a value xored at the end. Change any one and you get a
checksum that is perfectly good and that nobody else computes.

The way to know you got it right is not to round trip it against yourself, which
succeeds for every wrong variant. It is the **check value**: every published CRC
comes with the result of running it over the nine bytes `123456789`.

```text
CRC-8   polynomial 0x07,   init 0x00     check value 0xF4
CRC-16  polynomial 0x1021, init 0xFFFF   check value 0x29B1
```

If your code does not produce those, it is talking to itself.

## Build It

Implement in `exercise/solution.hpp`:

- `sum8` and `xor8`, so the measurements have something to be measured against.
- `crc8`, polynomial 0x07, starting value 0x00.
- `crc16_ccitt`, polynomial 0x1021, starting value 0xFFFF.
- `verify_and_strip`, which takes a payload whose last byte is its CRC-8 and
  reports whether it survived, handing back a view of the body.

```
rcpp verify 08-03
```

The tests include the published check values, the order independence of the two
cheap schemes as an exact fact rather than a statistic, and a whole message
carried end to end through the encoder, the framing and the checksum from the
last two lessons.

## Use It

Any datasheet that specifies a CRC gives you a polynomial and, if the author was
kind, a check value. This is where those turn into code, and the check value is
how you find out you agree before you have hardware to disagree with.

The other use is diagnostic. Count the frames that fail their checksum and
report the count. A link with a handful of failures an hour is healthy. One with
a steady few percent is telling you about a cable, a ground or a baud rate, and
it is telling you before anything else in the system notices.

## What Breaks First

- **An order independent checksum used where order can change.** A sum cannot
  see two swapped bytes, ever. See `E-IO-0005`.
- **A CRC that matches nothing but itself.** The polynomial, the starting value
  and the reflection all have to agree with the other end. See `E-IO-0006`.
- **A checksum computed over different bytes at each end.** Include the length
  or do not, but both ends must make the same choice. See `E-IO-0007`.

## Ship It

The four functions join `rc::io`. With the encoder and the parser they make a
complete wire format: fields written into bytes, a frame found in a stream, and
a check that the frame is the one that was sent.

What is left before a real board is the port itself, which is the next lesson.
