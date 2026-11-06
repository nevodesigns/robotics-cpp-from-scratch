# What a Struct Looks Like in Memory: Where Each Member Goes, and the Order That Wastes Least

> A struct is not its members laid end to end. Each goes where its alignment allows, and the
> order they are written in decides how much is wasted.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 02-05, 02-08

## The Problem

A buffer of a million records takes 48 MB, and the data in the records is 29. The program does not run out of
memory, and it does run slower than the arithmetic in it says it should, because its working set does not fit
in the cache it was sized for.

Nothing in the code is wrong. The struct is just bigger than its contents, by an amount that depends on the
order somebody happened to write the members in.

Lesson 08-01 found that `sizeof` of a struct includes padding and that a struct cannot be sent down a wire for
that reason. This lesson is about the padding itself: where it comes from and how to have less of it.

## The Concept

### The rule

Every member sits at an address that is a multiple of its own **alignment**, which is usually its size: a `double`
at a multiple of eight, a `uint16_t` at a multiple of two, a `uint8_t` anywhere. Members are placed one after
another in the order they are written, each at the next offset its alignment allows. Whatever lies in the gap is
**padding**, and stays unused.

The next offset is a rounding up:

```cpp
align_up(offset, alignment) = (offset + alignment - 1) / alignment * alignment
```

The integer division throws away the remainder, which is what turns adding one less than the alignment into a
rounding up. `align_up(9, 4)` is 12, `align_up(17, 16)` is 32, and an alignment of one or nothing leaves it
alone.

### The end is padded too

The size of the struct is its last offset rounded up to a multiple of **its own** alignment, which is the largest of
its members'. The reason is arrays: the second element starts where the first ends, and its members have to be
aligned as well.

A `uint64_t` and a `uint8_t` are nine bytes of members and a struct of **16**. The test puts three in an array and
checks that the second starts exactly `sizeof` after the first and that its `uint64_t` is on a multiple of eight. The
size of a struct is never the sum of the sizes of its members. See `E-MEM-0021`.

### The arithmetic against the compiler

`layout_of` takes each member's size and alignment and returns where it goes, the size, the padding and the struct's
alignment. The test checks every offset against the compiler's `offsetof` and every size against `sizeof`, for a byte,
a double and a byte, for a struct of mixed members, and for one with an `alignas(16)` member.

### The order is the waste

The same three members, a byte, a double and a byte:

| order | size | padding | offsets |
|---|---|---|---|
| as written | 24 | 14 | 0, 8, 16 |
| largest first | 16 | 6 | 0, 8, 9 |

Put the double first and the bytes follow it at 8 and 9, and the struct is 16. Written the other way, the double has to
skip to 8, the second byte to 16 and the end is padded to 24.

For a record of eight members holding 29 bytes of data:

| order | size |
|---|---|
| in the order they occurred to somebody | 48 bytes, 19 of them padding |
| the best order | 32 bytes |

A million of them is 48 MB against 32, and a cache line of 64 bytes holds 1.3 records against 2.0. See `E-MEM-0020`.

### The best order is the biggest alignment first

`smallest_size` puts the most strictly aligned members first. It leaves no gap before any of them, and what is left are members
of smaller alignment, which fit wherever the last one ended. For alignments that are powers of two, which they always are, that
is the best order there is.

The test does not take that on trust. It generates 400 random structs of three to six members, tries **every order** of each,
and keeps the smallest. Largest alignment first gave the smallest size in all 400, and reordering saved up to 32 bytes.

### Over-aligned members

A member with `alignas(16)`, or a SIMD vector, sits at a multiple of sixteen and gives the whole struct that alignment. A
byte, a 16 byte aligned vector of four floats and a byte are eighteen bytes of data and **48 bytes** of struct: the vector at 16,
the last byte at 32, and the end padded to 48. See `E-MEM-0022`.

## Build It

`align_up`, `layout_of` and `smallest_size` in `exercise/solution.hpp`. The types are given, and the comments say what each does,
including keeping members of equal alignment in their given order.

```
rcpp verify 02-13
```

The structs in the tests are real ones, and the compiler running the test is the judge of your arithmetic.

## Use It

`rc/core/layout.hpp` joins `rc::core`.

```cpp
static_assert(sizeof(TelemetryRecord) == 32, "the record grew: look at the order of its members");
```

A `static_assert` on the size is the habit worth taking away. It makes a field added later, or one whose alignment changed, a compile error and
not a surprise in a log file. Sort the members of a struct that is held in the millions, written to disk or shared with another processor.

**What this does not do.** It reorders by alignment only. It does not keep related fields together, which a person may prefer for reading, or put the
fields that are used together next to each other, which matters more for speed: a struct whose hot fields are in two cache lines is slower than a bigger
one whose hot fields are in one. It knows nothing about bit fields, packing pragmas, which remove padding and make unaligned access, or virtual
functions, which add a hidden pointer at the front. And on a different compiler or architecture the alignment of `long double`, or of a 64 bit
integer on a 32 bit target, is different, which is why the test uses types of a fixed size.

## What Breaks First

- **A struct in the order it occurred to somebody.** A third of it is padding. See `E-MEM-0020`.
- **A size computed as a sum.** The struct is padded at the end as well. See `E-MEM-0021`.
- **One over-aligned member.** The struct triples. See `E-MEM-0022`.

## Ship It

`rc/core/layout.hpp` holds `align_up`, `FieldSpec`, `Layout`, `layout_of` and `smallest_size`.

Phase 02 started with a pointer being an address. This is what the addresses inside a struct are, and it is a good place to end on.
