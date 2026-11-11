# The Read That Fails: Checksums, Retries and What a Bus Costs

> A bus read that fails is not an exception. It is Tuesday.

**Type:** Build
**Time:** about 110 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 15-12, 15-04

## The Problem

A decoder is only as good as the bytes it is given, and bytes cross a wire. Wires pick up noise, a device is busy when asked, a connector is a little loose. Some reads give nothing and some give something wrong.

A driver that assumes every read works hands the controller a wrong number about once in however many reads the wire loses, and the controller has no way to know. The answer is in two halves: a check that says whether the bytes can be trusted, and a bounded retry that
spends a few more tries on it and then gives up and says so (lesson 15-04 showed what a channel that says what it knows looks like).

## The Concept

### A check the frame carries

The sender puts the **CRC-8** of the payload after it, and the receiver works it out again. A bit flipped on the way changes it, and the frame fails.

```cpp
crc = crc ^ byte;
for (int bit = 0; bit < 8; ++bit) crc = (crc & 0x80) ? (crc << 1) ^ 0x07 : crc << 1;
```

The CRC of the ASCII string `123456789` is 0xF4, which is the figure to test an implementation against. See `E-SENSE-0029`.

### Why not a sum

A simple sum of the bytes is cheaper and looks as good. On a five byte frame:

| error | CRC-8 missed | plain sum missed |
|---|---|---|
| a single flipped bit, 40 possible | 0 | 0 |
| two bytes swapped, 6 possible | 0 | 6 |

A sum does not care about order, so it cannot see two bytes change places. A CRC does. See `E-SENSE-0029`.

### A bounded retry

```cpp
for (int i = 0; i < max_attempts; ++i) {
  ++attempts;
  if (!read(frame, size)) { ++timeouts; continue; }
  if (frame_ok(frame, size)) return ok;
  ++corrupt;
}
```

It stops at the first good frame and gives up after `max_attempts`, and it keeps a count of what went wrong each time. Those counts are the driver's health: a bus that is timing out is a different fault from one that is corrupting frames. See `E-SENSE-0030`.

### How many tries

If each attempt fails independently with probability p, the chance that all N fail is p to the power N. Each attempt in the test failed with probability 0.28, from 20 percent timeouts and 10 percent flipped bits:

| attempts | gave up | predicted | mean attempts used |
|---|---|---|---|
| 1 | 0.2757 | 0.2800 | 1.000 |
| 2 | 0.0771 | 0.0784 | 1.276 |
| 3 | 0.0223 | 0.0220 | 1.351 |
| 4 | 0.0062 | 0.0061 | 1.374 |
| 5 | 0.0015 | 0.0017 | 1.380 |

Twenty thousand reads each, the same random numbers. The extra tries cost little: the average read used 1.38 attempts even with five allowed, because most reads succeed early. See `E-SENSE-0031`.

### Enough

`attempts_needed` turns the arithmetic round:

| failure rate | for 99 percent | for 99.99 percent |
|---|---|---|
| 0.05 | 2 | 4 |
| 0.10 | 2 | 4 |
| 0.28 | 4 | 8 |
| 0.50 | 7 | 14 |
| 0.90 | 44 | 88 |

A bus failing nine times in ten needs 44 attempts for 99 percent, which is a bus that needs fixing and not retrying. A failure rate of one cannot be met by any number of attempts, and the function says so with zero. See `E-SENSE-0031`.

## Build It

`crc8`, `frame_ok`, `read_reliably`, `success_after` and `attempts_needed` in `exercise/solution.hpp`. The outcome type is given.

```
rcpp verify 15-13
```

The bus in the tests works out its own CRC bit by bit and fails according to a seeded generator, so the figures above are the same on every run.

## Use It

`rc/sensor/reliable_read.hpp` joins `rc::sensor`.

```cpp
std::uint8_t frame[7];
const ReadOutcome out = read_reliably([&](std::uint8_t* p, std::size_t n) { return bus.read(p, n); }, 4, frame, 7);
if (out.ok) use(frame); else report(out.timeouts, out.corrupt);
```

**What this does not do.** It retries at once, with no wait, which suits a corrupted frame and not a device that is busy and needs a moment. It assumes the failures are independent, and a loose connector is not: they come in runs, so
the real give up rate is higher than p to the power N. It adds latency in the worst case of N attempts, which has to fit in the control period (lesson 14-12), and a CRC says nothing of a frame that is intact and stale (lesson 15-03).

## What Breaks First

- **No check on the frame.** A flipped bit becomes a reading. See `E-SENSE-0029`.
- **A retry nobody counts.** A failing bus looks healthy. See `E-SENSE-0030`.
- **A number of tries chosen by feel.** Too few lose readings, too many hide a fault. See `E-SENSE-0031`.

## Ship It

`rc/sensor/reliable_read.hpp` holds `crc8`, `frame_ok`, `ReadOutcome`, `read_reliably`, `success_after` and `attempts_needed`.

This is where lesson 15-12's decoder gets its bytes from: decode only a frame that passed.
