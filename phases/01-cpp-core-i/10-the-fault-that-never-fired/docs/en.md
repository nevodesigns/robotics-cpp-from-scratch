# The Fault That Never Fired: Bits, Masks, and the Precedence Nobody Remembers

> The over temperature bit was set. The check said no fault. Nothing was broken
> except one pair of brackets that was not there.

**Type:** Build
**Time:** about 105 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 01-06, 01-09

## The Problem

Every motor driver, IMU and encoder chip on a robot has a status register: one
word where each bit means a different thing has gone wrong. Over current. Over
temperature. Stalled. Encoder signal lost.

Reading that register is the first thing a driver does. Acting on it correctly
is the difference between a machine that stops when something is wrong and a
machine that keeps pushing a stalled motor until something melts.

The arithmetic for it is four operators and it has three traps in it, and all
three produce a program that works, runs, and reports the wrong faults.

## The Concept

### A register is a fixed number of bits, so say which

Before anything else, the type. `int` is a number. A status register is a
specific width of a specific signedness, because the data sheet says so, and
`<cstdint>` has names that say it:

```cpp
using Bits = std::uint32_t;    // exactly 32 bits, unsigned, everywhere
```

`std::uint8_t`, `std::uint16_t`, `std::uint32_t`, `std::uint64_t`. Exactly that
many bits on every platform, which `int` does not promise. For a register, a
buffer, or anything a data sheet describes, these are the types to reach for,
and the rest of this lesson is partly about what happens when you pick a narrow
one.

### Trap one: precedence, and the fault that never fires

Here is the line, and it is in a great deal of shipped firmware:

```cpp
if (status & kOverTemp != 0) { shut_down(); }
```

`==` and `!=` bind **tighter** than `&`. So the compiler reads it as:

```cpp
if (status & (kOverTemp != 0))
```

`kOverTemp != 0` is true. True converts to 1. So the whole expression is
`status & 1`, which asks whether **bit zero** is set, whatever flag you named.

Measured with over temperature and stalled both set, so the fault is certainly
there:

| form | result |
|---|---|
| `has_any(status, kOverTemp)` | true |
| `status & kOverTemp != 0`, which is `status & 1` | 0, so **false** |

Now the part that makes this genuinely dangerous rather than merely wrong.

**It is not always wrong.** When the flag being tested happens to be bit zero,
`status & 1` is exactly the right question and the two forms agree. The first
flag in a data sheet is almost always bit zero. So the first test anybody writes
passes, the check looks proven, and the bug is in every other flag.

gcc and clang both refuse the line under `-Werror`, through `-Wparentheses`,
which is part of `-Wall`. Switch it on. And remember from lesson 01-07 how
uneven that kind of coverage is between compilers: a diagnostic is a bonus, not
a floor.

The fix that does not depend on anybody remembering a precedence table is a
name:

```cpp
has_any(status, kOverTemp)
```

A named function has no precedence to get wrong. That is the whole argument for
this header.

### Trap two: width, and the bit that went nowhere

Narrow types promote to `int` before anything happens to them. So the
expression is a different type, and a different width, from the variable it came
from.

Measured on a `std::uint8_t` holding `0x80`:

| | value | sizeof |
|---|---|---|
| `narrow` | 0x80 | 1 |
| `narrow << 1` as an expression | 256 | 4 |
| stored back into the `std::uint8_t` | **0** | 1 |

The shift did not lose the bit. The **assignment** did, and nothing said so.
`if (narrow << 1)` is true while the stored value is zero.

The complement is the same problem with a nastier shape. For a mask of `0x02` in
a `std::uint8_t`:

| | value | sizeof |
|---|---|---|
| `~mask` | **-3** | 4 |

`~mask` is a *signed int* with the entire top of the word set. Used immediately
in an and it gives the right answer, because the and throws the top away again.
Stored, compared, or passed to something expecting a mask, it does not.

Neither of these warns under `-Wall -Wextra` on either compiler.

The repair is not to remember the promotion rules at each use. It is to pick one
register width and stay at it. `Bits` is a fixed `std::uint32_t`, so every
expression in the header is already the width it looks like, and a narrower
device register widens into it losing nothing.

### Trap three: the shift count

Lesson 01-09 established that shifting by the width of the type or further is
undefined behaviour, and that nothing diagnoses it once the count arrives at
runtime. A loop building masks from a bit index walks straight into it.

So `bit(index)` checks first:

```cpp
inline Bits bit(int index) {
  if (!is_valid_bit_index(index)) return 0u;
  return Bits{1u} << index;
}
```

Two details in two lines. The shift is of `Bits{1u}`, not `1`, so it happens at
the register width: `1 << 31` on a signed int is undefined, which is trap three
and trap two in the same character.

And the return for a bad index is zero, which is a choice worth defending. Zero
is the identity for or, so `value | bit(99)` leaves `value` alone rather than
corrupting it, and it is absorbing for and, so `has_any(value, bit(99))` is
false rather than true. A wrong index therefore produces a flag that is never
set and never clears anything: a visible failure instead of a silent
corruption. Wrapping the index with `% 32` would instead address a different
real bit, which is exactly the corruption being avoided.

### Any is not all

One more distinction, because it costs nothing to get right and is invisible
until it bites.

```cpp
(value & mask) != 0      // is ANY bit of mask set
(value & mask) == mask   // are ALL bits of mask set
```

With a single bit mask these are the same answer, always. They diverge the
moment the mask covers two bits. Measured with only over temperature set,
against a mask of all four known faults:

| question | answer |
|---|---|
| `has_any(status, kAllFaults)` | true |
| `has_all(status, kAllFaults)` | false |

Both are right answers to their own question. The bug is asking the other one,
and a test suite written against single flags cannot tell them apart.

### One free check worth running

A flag constant transcribed from a data sheet is supposed to be a single bit,
and the common typo turns it into two. `0x03` where `0x02` was meant reports two
faults for one, and clears a neighbour when acknowledged.

```cpp
for (Bits flag : flags) RC_CHECK(is_single_bit(flag));
```

Run once at startup over every constant. It catches a whole class of
transcription error for one line.

### What the type system can do here

Flags are integers, and two different registers on the same device are the same
integer type, so nothing stops a mask for one being tested against the other.

A scoped enum per register does stop that. It will not implicitly convert to its
underlying type, so it cannot be dropped into a bitwise expression at all, and
that is a hard type error on every compiler rather than a warning. This lesson's
`checks/` directory holds one, so you meet the error deliberately.

The price is that the conversion has to be written where it is meant, and that
is the trade: an explicit `static_cast` at each use, in exchange for the
compiler refusing to mix up two fault sets.

## Build It

Implement `bit`, `has_any`, `has_all`, `with_cleared`, `count_set` and
`is_single_bit` in `exercise/solution.hpp`.

```
rcpp verify 01-10
```

The suite sets a real fault and shows the mistaken check reporting nothing,
walks every one of the thirty two bit indices and four that do not exist,
measures what a narrow type does to a shift and a complement, and decodes a
driver status register the way a driver has to. One snippet in `checks/` is
compiled and expected to fail.

## Use It

**Never write a bitwise test inline.** `has_any` and `has_all` say which
question they are asking and cannot be misparsed.

**Pick one register width and stay at it.** `Bits` is `std::uint32_t`. Widen
narrow device registers into it on the way in.

**Build masks with `bit(index)`,** which checks the index and shifts at the
right width.

**Check your flag constants are single bits, at startup.**

**Test a flag that is not bit zero, and a mask with more than one bit in it.**
Those two tests are what separate correct code from the three bugs above.

**Switch on `-Wparentheses`.** It catches trap one and nothing else here.

## What Breaks First

- **A fault check that reports no fault.** See `E-CPP-0034`.
- **A mask on a narrow type that loses the top of the word.** See `E-CPP-0035`.
- **Any and all confused.** See `E-CPP-0036`.

## Ship It

`Bits`, `bit`, `has_any`, `has_all`, `with_set`, `with_cleared`, `count_set` and
`is_single_bit` join `rc::core`. This lesson also introduces the fixed width
integer types, which phase 08 needs to put bytes on a wire and phase 15 needs
to read a real sensor, and both now list it as a prerequisite rather than
teaching the types again.
