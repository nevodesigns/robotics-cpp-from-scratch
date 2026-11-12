# The Target Is Not the Host: Reading a Target Triple, and the Same Struct in Four Sizes

> The first thing wrong with a program that works on your laptop and not on the robot is that the laptop is not the robot.

**Type:** Build
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 19-14, 08-01, 04-10, 04-01

## The Problem

Phase 19 ended with a robot that can be shown to be safe. It is safe on the machine it was built and tested on, a laptop with an x86-64 processor. The robot itself has a different processor, perhaps 32 bit ARM, or 64 bit ARM, and a different operating system image. **Cross compilation** is building on one machine for another, and the first thing it needs is a way to say which other: the **target triple**.

The second thing it needs is the knowledge of how the target differs. A program that compiles for the robot and runs wrongly is the usual outcome, and it is wrong in ways that are the same few: a struct that is a different size, an integer of a different width, a number laid out in the other byte order. This lesson makes those differences into code that can be run on any machine.

## The Concept

### A triple

A target is named by a **triple**: `x86_64-linux-gnu` is architecture, operating system and C library convention. A fourth part, the vendor, is often put in: `x86_64-pc-linux-gnu`. A third part may also be the vendor, with the system after: `x86_64-w64-mingw32`. `parse_triple` accepts all three shapes: with three parts, if the second is an operating system it is architecture, system and convention, otherwise architecture, vendor and system. What it reads, for the targets of this curriculum:

| triple | pointer | long | byte order | os |
|---|---|---|---|---|
| x86_64-linux-gnu | 64 | 64 | little | linux |
| aarch64-linux-gnu | 64 | 64 | little | linux |
| arm-linux-gnueabihf | 32 | 32 | little | linux |
| i686-pc-linux-gnu | 32 | 32 | little | linux |
| x86_64-w64-mingw32 | 64 | **32** | little | mingw32 |
| x86_64-pc-windows-msvc | 64 | **32** | little | windows |
| mips-linux-gnu | 32 | 32 | **big** | linux |
| riscv64-unknown-linux-gnu | 64 | 64 | little | linux |

The two Windows rows are the trap: a 64 bit machine on which `long` is **32 bits**. Code that uses `long` for something that must be 64 bits works on every Linux target and breaks only on Windows. Nine malformed triples (empty, two parts, an empty part, an unknown architecture, an unknown system, five parts) were all **invalid**, with a pointer width of 0. A reader that guesses is a reader that builds for the wrong machine without a word. See `E-DEPLOY-0002`.

### The same struct, five sizes

`struct { char flag; int64 stamp; void* next; long count; double x; }`, laid out with each field at the next multiple of its alignment:

| target | size |
|---|---|
| x86_64-linux-gnu | 40 |
| aarch64-linux-gnu | 40 |
| x86_64-w64-mingw32 | 40 |
| arm-linux-gnueabihf | 32 |
| i686-pc-linux-gnu | 28 |

Three different answers from the same source. The 32 bit targets have smaller pointers and longs; and **32 bit x86 aligns an eight byte field to 4**, so it packs tighter than 32 bit ARM, which aligns it to 8. The mingw row is 40, the same as Linux, with a `long` of 4 bytes, because the padding fills up the difference: two layouts that happen to agree are not the same layout. A struct that is written to a file, sent over a link or shared with a driver **as it lies in memory** is read wrongly by any machine on which its layout differs, and nothing at the receiving end can tell. See `E-DEPLOY-0001`.

### When it is safe to send a struct as it is

`layout_equal` says yes only if the struct has the **same size on every target** and has **no** pointer, `long` or `size_t`, whose width depends on the target. `{ int32, int32, float }` was the same everywhere (12). `{ char, int64 }` differed: **16** bytes on the 64 bit targets and 12 bytes on 32 bit x86, though it contains nothing but fixed width types, because of the alignment of the `int64`. `{ int32, long }` and `{ pointer }` were refused outright. Between two 64 bit Linux machines the `{ char, int64 }` struct is fine. The answer for data that leaves the machine is the one of lesson 08-01: **write each field out in a defined order and width**, and do not send memory. See `E-DEPLOY-0001`.

### Byte order

`0x01020304` is in memory as **4 3 2 1** on a little endian machine and **1 2 3 4** on a big endian one. The bytes that the big endian machine wrote, read by a little endian machine as a number, gave **67305985**, which is `0x04030201`. Nearly everything a robot uses today is little endian, and **nearly** is the word: a PLC, a network protocol and some older microcontrollers are not. See `E-DEPLOY-0003`.

## Build It

`parse_triple`, `size_of`, `align_of`, `struct_size`, `encode_u32` and `layout_equal` in `exercise/solution.hpp`.

```
rcpp verify 20-01
```

## Use It

`rc/deploy/target.hpp` opens the new module `rc::deploy`.

```cpp
const rc::deploy::Target robot = rc::deploy::parse_triple("aarch64-linux-gnu");
if (!robot.valid) return 2;
const bool safe = rc::deploy::layout_equal({rc::deploy::Field::i32, rc::deploy::Field::f32}, {host, robot});
```

**What this does not do.** It is a **model of the rules**, written from the documented ABIs, and the only machine it was run on is an x86-64 laptop: the numbers for ARM, i686 and Windows are the rules applied, **not measured on those machines**. The way to be sure on the real target is a `static_assert(sizeof(Message) == 12)` in the header, compiled by the cross compiler, which fails the build on the day the size changes. It does not know about packed structs, bit fields (whose layout is implementation defined), vectors, or the many other triples there are. It covers a handful of architectures and operating systems. And it does not build anything: this repository's continuous integration has **no cross compiler**, so no lesson in this phase cross compiles. They teach what the work is made of, in code that runs on all four lanes, as phase 17 did for ROS 2.

## What Breaks First

- **A struct sent as it lies in memory.** See `E-DEPLOY-0001`.
- **A triple read by guessing.** See `E-DEPLOY-0002`.
- **Bytes in the other machine's order.** See `E-DEPLOY-0003`.

## Ship It

`rc/deploy/target.hpp` holds `Endian`, `Target`, `parse_triple`, `Field`, `size_of`, `align_of`, `struct_size`, `encode_u32` and `layout_equal`.
