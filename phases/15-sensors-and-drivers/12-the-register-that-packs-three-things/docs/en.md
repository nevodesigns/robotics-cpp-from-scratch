# The Register That Packs Three Things: Reading a Sensor Chip's Bytes Without Guessing

> The datasheet says the value is a twelve bit two's complement number, left justified, high byte first. Every word in that sentence is a way to be wrong.

**Type:** Build
**Time:** about 110 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 15-08, 08-01

## The Problem

Everything so far took a sensor's output as a number. A chip does not give a number. It gives bytes, and the bytes pack more than one thing: a status byte with flags and a setting folded into it,
then readings that are narrower than the bytes that hold them, signed, and arranged in an order the datasheet states once.

The first time a driver is written it reads plausible numbers, and they are wrong in ways that are hard to see: right when the joint is held one way and wildly wrong the other, or too big by sixteen.

## The Concept

### The frame

The test chip sends seven bytes.

```
byte 0     status: bit 0 data ready, bit 1 overrun, bits 4 and 5 the range code
bytes 1-6  x, y, z: two bytes each, high byte first, a 12 bit two's complement number in the top twelve bits
```

The range code 0, 1, 2 and 3 means plus or minus 2, 4, 8 and 16 g over 2048 counts.

### Bits from the middle of a register

Shift the field down to bit zero, then mask off everything above it.

```cpp
(reg >> shift) & ((1u << width) - 1u)
```

The range code is `extract_bits(status, 4, 2)`. Mind a width of 32, where shifting 1 left by 32 is undefined, and a shift or a width that makes no sense, which get zero here. See `E-SENSE-0027`.

### Negative numbers

A twelve bit number is held in at least sixteen bits, and the top four of those are not its sign. Twelve bits with the top one set is negative, and the way to say so is to subtract 2 to the twelfth:

| raw 12 bit | unsigned | signed |
|---|---|---|
| 0x000 | 0 | 0 |
| 0x001 | 1 | 1 |
| 0x7FF | 2047 | 2047 |
| 0x800 | 2048 | -2048 |
| 0xFFF | 4095 | -1 |

Read as unsigned, a small negative reading becomes a number near 4095, and a joint at rest on one side of level looks as if it is at the far end of the range. See `E-SENSE-0026`.

### One frame, decoded

A frame of `01 40 00 E0 00 80 00` at plus or minus 2 g: status says ready and range 0; x is 0x4000 shifted down four, 1024 counts; y is 0xE000, which is 3584 as a raw 12 bit and -512 signed; z is 0x8000, which is -2048. At 1 g per 1024 counts:

```
x 1.000, y -0.500, z -2.000 g
```

Every one of the 4096 counts on all four ranges, 16384 frames, decoded to the count times the size of a count, with no axis wrong. One count is 0.000977 g at 2 g and 0.007812 g at 16 g, so the same raw number means eight times as much on the widest range. See `E-SENSE-0027`.

### A frame that is not data

Three things come back that are not an ordinary reading, and each needs its own answer:

- **Too short**, fewer than seven bytes: nothing was read, the output is untouched.
- **Not ready**, bit 0 clear: the chip has no new sample, and the bytes are the last ones.
- **Overrun**, bit 1 set: the frame is good, and an earlier one was lost because nobody read it in time.

The overrun frame still carried 0.1953 g in the test, and a driver that throws it away loses a good reading as well as the lost one. See `E-SENSE-0028`.

## Build It

`extract_bits`, `sign_extend`, `range_in_g` and `decode_frame` in `exercise/solution.hpp`. The sample type and the status enum are given.

```
rcpp verify 15-12
```

The tests pack frames by hand on the chip's side of the wire, so the check does not use the code it is checking.

## Use It

`rc/sensor/registers.hpp` joins `rc::sensor`.

```cpp
AccelSample sample;
switch (decode_frame(bytes, count, sample)) {
  case DecodeStatus::Ok: use(sample); break;
  case DecodeStatus::Overrun: use(sample); note_lost_frame(); break;
  default: break;
}
```

**What this does not do.** It decodes a frame that is already in memory. Getting it there, over a bus that can fail, is the next lesson in this phase. It knows one chip: another has its own byte order, width and flags, and the same three steps apply. It says nothing about units beyond g,
or about calibration (lesson 15-02), and a byte stream with no frame boundaries needs framing first (lesson 08-01).

## What Breaks First

- **Reading a signed number as unsigned.** A reading just below zero becomes one near the top of the range. See `E-SENSE-0026`.
- **Forgetting the register is narrower than its bytes.** The reading is sixteen times too big. See `E-SENSE-0027`.
- **Ignoring the status byte.** A stale or lossy frame is used as if it were new. See `E-SENSE-0028`.

## Ship It

`rc/sensor/registers.hpp` holds `extract_bits`, `sign_extend`, `range_in_g`, `AccelSample`, `DecodeStatus` and `decode_frame`.

The next lesson is the read that fails.
