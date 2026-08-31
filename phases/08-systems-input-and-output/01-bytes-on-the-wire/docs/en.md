# Bytes on the Wire: What a Struct Is Not

> The microcontroller is not running your compiler, on your processor, with your
> flags. Everything the two of you agree on has to be written down in bytes, and
> a struct is not written down in bytes.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 05-02

## The Problem

You have a reading to send to a microcontroller:

```cpp
struct Reading {
  std::uint8_t id;
  double x;
  std::uint16_t sequence;
};
```

Eleven bytes of data. The obvious thing works on the first try, on your machine,
against a program you also compiled:

```cpp
Reading reading{1, 3.5, 42};
port.write(reinterpret_cast<const char*>(&reading), sizeof(reading));
```

Then you point it at the actual device and it reads garbage. Not slightly wrong
values, which you could debug. Garbage, and a different kind of garbage on the
colleague's machine.

There are four separate bugs in that one line. Each of them is invisible in a
test where both ends are the same binary, which is exactly the test you wrote.

## The Concept

### A struct is bigger than its fields

Run this and read the numbers, because the first one is the point:

```cpp
sizeof(Reading)                                    // 24
sizeof(std::uint8_t) + sizeof(double) + sizeof(std::uint16_t)   // 11
```

Twenty four, for eleven bytes of data. The compiler inserted thirteen bytes of
padding, because a `double` must sit at an address that is a multiple of eight
and `id` used only the first byte. Reorder the fields and the same data becomes
sixteen bytes.

Padding is not garbage in the sense of being random; it is uninitialised, so
what is in it depends on whatever the memory held before. `sizeof` is a decision
your compiler made, with your flags, for your processor, and the device at the
other end made a different one.

**Nothing you send may be a struct.** Send fields.

### Endianness: the order the bytes come out

A `std::uint32_t` holding `0x01020304` is four bytes in memory. Which four, in
what order, is not fixed by the value:

```text
little endian (x86, most ARM)   04 03 02 01
big endian    (network order)   01 02 03 04
```

Both are correct. They disagree, which is the entire problem. If your code
writes the memory bytes and the device reads them the other way round,
`0x01020304` arrives as `0x04030201`, which is 67305985 instead of 16909060.

The fix is not to detect the endianness of your machine. It is to **choose one
order for the wire and write each byte deliberately**, so the code produces the
same bytes wherever it is compiled:

```cpp
buffer[0] = static_cast<std::uint8_t>((value >> 24) & 0xFFu);
buffer[1] = static_cast<std::uint8_t>((value >> 16) & 0xFFu);
buffer[2] = static_cast<std::uint8_t>((value >>  8) & 0xFFu);
buffer[3] = static_cast<std::uint8_t>((value      ) & 0xFFu);
```

That code has no endianness. Shifting is arithmetic on a value, not a view of
memory, so it means the same thing on every machine. This lesson uses big endian
because most sensor and network protocols do, and being able to read a packet
capture left to right is worth something.

### char is not a byte

`char` is signed on x86 Linux and unsigned on ARM, and the standard permits
both. The same code reading the same byte:

```cpp
char c = 0xFF;          //  -1 on this machine, 255 on a Raspberry Pi
std::uint8_t b = 0xFF;  // 255 everywhere
```

A checksum accumulated in `char` on one platform and `unsigned char` on another
gives two different answers from the same bytes, and the disagreement is in the
type rather than in the logic, so reading the logic will not find it.

Buffers are `std::uint8_t`. Not `char`, which has a signedness the standard
leaves open. Not `int`, which is four bytes of a one byte thing.

### Fixed width, because the widths are not fixed

`int` is at least sixteen bits. `long` is sixty four bits on Linux and thirty
two on Windows, which is one of the few places where the same C++ compiled on
two supported platforms of this curriculum disagrees about a number's size.

Wire formats use `std::uint8_t`, `std::uint16_t`, `std::uint32_t` and their
signed forms, from `<cstdint>`, whose names say what they are.

### memcpy, not a cast

To get at the bytes of a `float`:

```cpp
std::uint32_t bits = *reinterpret_cast<const std::uint32_t*>(&value);   // wrong
std::memcpy(&bits, &value, sizeof(bits));                              // right
```

The first is undefined behaviour. Reading an object through a pointer to an
unrelated type breaks the aliasing rules the optimiser relies on, so the
compiler is entitled to assume it cannot happen and to reorder around it. It
usually works, until a release build with optimisation on, which is the worst
possible time to find out.

There is a second reason, less discussed and more brutal. `reinterpret_cast` on
a pointer into the middle of a buffer produces a misaligned pointer. On x86 that
is slow. On some ARM cores it is a fault, and the program stops.

`memcpy` has neither problem, and every compiler recognises the pattern and
emits the same single instruction the cast would have. It costs nothing except
having to know that it is what you want.

## Build It

Implement `ByteWriter` and `ByteReader` in `exercise/solution.hpp`. Both work
over a caller supplied buffer and never allocate, because this code ends up in
places where allocation is not allowed.

- `ByteWriter(rc::span<std::uint8_t> destination)`, and `put_u8`, `put_u16`,
  `put_u32`, `put_i16`, `put_i32`, `put_f32`, each big endian, each returning
  false when there is no room.
- `ByteReader(rc::span<const std::uint8_t> source)`, and the matching `get_`
  functions, each returning false when there are not enough bytes left.
- `ok()` on both: sticky, so a whole message can be written or parsed and
  checked once at the end rather than after every field.

```
rcpp verify 08-01
```

The tests check the actual bytes, not just the round trip. A round trip alone
passes happily with the endianness backwards at both ends, which is precisely
the bug that survives all the way to the device.

## Use It

This is what every protocol in the rest of the curriculum is built on: the
serial framing in the next lesson, the checksum after it, and eventually the
messages that go to and from a real board.

It is also what to write when a datasheet says "register 0x40, sixteen bit,
MSB first". That sentence is a specification of bytes, and `put_u16` is what
turns it into code you can point at.

## What Breaks First

- **The bytes come out backwards.** Writing the memory of an integer rather
  than shifting its value, which works until the other end disagrees. See
  `E-IO-0001`.
- **A struct sent whole.** `sizeof` includes padding your compiler chose, and
  the device chose differently. See `E-IO-0002`.
- **One byte past the end.** The bounds check must come before the write, not
  after. See `E-CPP-0007`.

## Ship It

`ByteWriter` and `ByteReader` join `rc::io`, next to the handle and the replay
sensor. From here the curriculum can describe a wire format precisely, which is
the thing standing between this code and a real board.
