# The Value That Was Never Written: Uninitialised Memory, and Proving a Function Filled Its Output

> This lesson's own suite caught the author quoting a number from a scratch
> experiment instead of from the suite. The scratch program saw 32 of 32 bytes
> read as zero; inside the suite it was 27, and the difference turned out to be
> the better half of the lesson.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 01-01, 02-01, 02-02

## The Problem

```cpp
Pose p;
if (read_pose(&p)) {
  drive_towards(p);
}
```

The call succeeded. The error was checked. `p.heading` was never written, and
the robot turns.

The symptom arrives in whichever disguise your project happens to supply. The
number is wrong differently each run, or the same way every run, which is
worse because it looks deliberate. It works in the debug build and fails in
release, or works in release and fails in debug: both directions happen. It
breaks the day somebody adds an unrelated local variable to a function higher
up the stack.

And frequently it does not look broken at all, because the leftover value was
`0`, and `0` is a perfectly ordinary heading.

## The Concept

### Declaring is not filling

`double x;` reserves eight bytes. It puts nothing in them. What is in them is
what was there before: the previous call's locals if this is the stack, the
last owner's data plus the allocator's bookkeeping if this is the heap.

Reading one is **undefined behaviour**, and that phrase is doing real work. It
does not mean "you get a random number". It means the compiler is entitled to
assume the read never happens, so an optimiser may propagate a constant
through it, delete the branch that depended on it, or keep the variable in a
register that was holding something else.

Measured, one function returning an unwritten `int`, one machine, one
compiler:

| build | what it returned |
| --- | --- |
| `-O0` | `1405118043` |
| `-O2` | `0` |

Neither is a compiler bug. There was no correct answer to give. This is the
whole mechanism behind "it works in release", and it is why reasoning about
what the leftover bytes probably contain is the wrong instinct.

### The heap's more convincing lie

Ask for 32 bytes, fill them with `0x7e`, free them, ask for 32 bytes again.
The lesson's suite prints what came back:

```
    bytes that were zero on arrival:     27 of 32
    bytes still holding 0x7e afterwards: 16 of 32
```

Sixteen bytes came back **exactly as they were left**. The other sixteen were
overwritten by the allocator's own free list bookkeeping, which differed
between runs because it stores a mangled pointer there. Which half you get
depends on the size you asked for.

The first number is the more instructive one, and it is the reason the epigraph
above is at the top of this lesson. A program whose very first act is that
allocation sees all 32 bytes read as zero, because the kernel must not hand one
process another process's data. That is a property of the **page**, not of
`new`. The lesson's test suite has already allocated by the time it gets there,
so it sees 27.

Nothing is the first allocation for long. A buffer that read as zeros while you
were testing tells you nothing whatsoever about the next one.

### What writes, and what does not

| written | not written |
| --- | --- |
| `Pose p = {};` | `Pose p;` |
| `Pose p = {1.5};`, rest set to zero | |
| `new int[4]()` | `new int[4]` |
| `std::vector<int> v(4)` | |

The parentheses in `new int[4]()` are the entire difference between four
defined ints and four indeterminate ones. `std::vector` is the right answer in
this curriculum anyway, for the reason lesson 02-02 gave: it also cannot leak.

Braces check the count **in one direction only**. Too many initialisers is an
error on every compiler, and `checks/one_value_too_many.cpp` proves it. Too few
is accepted, and the members you left out are value initialised to zero. So a
short brace is the fix, not a smaller version of the problem.

### The one case the language itself refuses

```cpp
const Pose p;        // error on every compiler
```

`checks/a_const_with_nothing_in_it.cpp` ships this. A `const` object can never
be assigned to later, so an uninitialised one is not a value that might be
written soon: it is one that can never be written at all. That is unambiguous,
local and decidable, so it is an error.

Drop the `const` and the identical declaration is accepted everywhere, because
now the compiler would have to prove that no path writes it before the first
read, and across a function boundary it cannot. That boundary is exactly where
the diagnostics stop.

### Where the diagnostics stop, measured

gcc 11 and clang 14, both at `-O0` and `-O2`:

| the read | gcc | clang |
| --- | --- | --- |
| a local `int` read in the same function | warns | warns |
| a struct member read in the same function | warns | **silent** |
| a struct member read inside a function it was passed to | **silent** | **silent** |

The third row is the one that reaches a robot, and nothing warns about it.

A related measurement that cuts the other way, because it is the compiler
warning about the **safe** thing: `Pose p = {1.5};` on a struct whose members
have no defaults warns on both compilers as `-Wmissing-field-initializers`,
even though the behaviour is completely defined. Give the members default
initialisers, as lesson 01-01 asked, and the identical brace is silent on both.
The warning is the compiler asking for the defaults, not a claim that anything
was left unwritten.

### The sanitizer lane does not cover this

Worth stating plainly, because this repository runs a sanitizer job on every
push and it would be reasonable to assume it has this covered.

The address sanitizer finds buffer overflows, leaks and use after free. It is
blind to uninitialised reads. The lesson's example ran clean under
`-fsanitize=address,undefined`, exit code 0, and printed the garbage.

The tool that does find them is the **memory** sanitizer. It is clang only, it
cannot be combined with the address sanitizer, and it needs the whole program
including the standard library instrumented to be reliable. Tried on this
curriculum's machine it needed `-fPIE -pie` to start at all, then caught a
trivial unwritten local and **missed** the struct member case.

So the gap is real, no tool in these lanes closes it, and that is why the fix
has to be in how the type and the test are written.

### Making it loud

```cpp
inline unsigned char poison_pattern() { return 0xff; }

inline void poison_bytes(void* storage, std::size_t bytes) {
  if (storage == nullptr || bytes == 0) return;
  std::memset(storage, poison_pattern(), bytes);
}
```

Every byte `0xff`, chosen for three reasons that are all about being noticed.

A **double** of all ones is a NaN. Arithmetic on an unwritten heading then
produces something visibly not a number, NaN propagates through every
operation after it, and every comparison with it is false, including
`heading == heading`. The suite prints it:

```
    heading            -nan
    distance * cos     -nan
    heading == heading false
```

A **pointer** of all ones is not a mappable address on any platform this
curriculum claims, so a dereference faults at once instead of reading whatever
is there.

An **integer** of all ones is `-1`, which is not loud, and that is the honest
limit of the idea.

Now the part that matters more than the bytes. Before the call, reading that
storage is undefined behaviour and the compiler may assume you will not. After
it, every byte has been written, so reading it is ordinary defined behaviour
that yields a NaN.

**Poisoning does not make the bug legal.** It converts an undefined read into a
defined and loud one, which is the difference between a value you can see in a
log and a value the optimiser was entitled to invent.

### Proving the function filled its output

The C shape every driver, vendor SDK and OS API uses has a hole in it: the
return value says whether the call succeeded, and nothing says whether the
output was completely written. A function that writes two of three members and
returns `true` is reporting honestly by its own lights. It did not fail. It did
not finish, and there is no channel for that.

Poison the output, call, and count. The suite's measurement, on a 24 byte
`Pose` of three doubles, with both functions returning `true`:

| filler | bytes left unwritten |
| --- | --- |
| writes all three members | 0 |
| writes two and returns true | 8 |

Eight is `sizeof(double)`, and it names the field: the one the return value
said nothing about.

### The limit, measured rather than mentioned

A byte a function wrote to `0xff` on purpose cannot be told from one it never
touched. The last test in the suite measures exactly that, on an eight byte
frame where **every** byte was written and two of them happened to be `0xff`:

```
  every byte of this frame was written, and the counter
  reports 2 unwritten, because two of them were
  written to the pattern itself.
```

So read the count as a **lower bound** on what was written, not an upper bound.
For a struct of doubles it is exact, because the pattern repeated is a NaN and
no correct code writes that. Padding counts as unwritten, because it is, which
is the same fact `E-IO-0002` cares about from the other side.

## Build It

Four pieces, and the first is a decision rather than code.

Choose the pattern. Write down why, against the three criteria above, before
writing the byte. Returning `0` satisfies none of them, which is why the suite
fails until it is right.

Write `poison_bytes`, guarding the null pointer and the empty range so callers
need not.

Write `bytes_still_poisoned`. The cast to `unsigned char` is the part to
understand rather than copy: it is the only type you may inspect an arbitrary
object representation through, which is what makes the loop defined where
reading the equivalent `int` would not be. It is the same rule that lets the
suite's own byte counting measure the heap at all.

Write `all_bytes_written` in terms of that, in one line.

## Use It

```cpp
#include <rc/core/init.hpp>

Pose p;
rc::core::poison_bytes(&p, sizeof(p));
const bool ok = read_pose(&p);
RC_CHECK(ok);
RC_CHECK(rc::core::all_bytes_written(&p, sizeof(p)));
```

This belongs in a test, not in a shipping code path. The point is to prove once
that the function fills what it claims to, so every call site can go on
trusting the return value.

Where the function is yours to change, the deeper fix is to return the value
instead of filling a pointer. A `Pose` returned by value cannot be partly
written, and lesson 03-02's error type carries the failure case without needing
a second channel.

And for the types themselves: give the members defaults. It is lesson 01-01's
advice, and this lesson is the second reason for it.

## What Breaks First

`E-MEM-0014` is the read itself, with the full diagnostic table and the two
different answers from the two optimisation levels.

`E-MEM-0015` is the heap that looked zeroed, with the 16 of 32 and the 27 of 32.

`E-MEM-0016` is the function that filled part of its output and returned
`true`, which is the one you will meet through somebody else's driver.

## Ship It

`rc/core/init.hpp`, in `namespace rc::core`.

From here on, any lesson in this curriculum that tests a function filling an
output parameter poisons it first.
