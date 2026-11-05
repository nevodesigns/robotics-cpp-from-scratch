# Where the Locals Live: The Stack, Its Size, and the Recursion That Ends the Program

> A recursion with 64 byte frames reached depth 130860 and then died with no
> exception, no error code and no last line of output. The program simply
> stopped.

**Type:** Build
**Time:** about 115 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 00-09, 02-01, 02-02, 02-03

## The Problem

Lesson 02-01 taught that a pointer is where something is. Lesson 02-02 taught
the heap, and that every manual `delete` is eventually wrong.

Nobody has yet said where an ordinary local variable lives, how much room
there is for them, or what happens when you run out.

There is a lot of room. It is not unlimited. And running out does not throw,
does not return an error, and does not print anything.

## The Concept

### Where things are

One program, four addresses:

```
  a local in main        0x7ffd1795103f
  a heap allocation      0x5baa7c65deb0
  a global               0x5baa48796010
  the code of main       0x5baa48793259
```

The local is somewhere else entirely. Globals, statics and code sit near each
other low down, the heap above them, the stack far above both.

The exact values change every run, because the operating system moves things
about on purpose. The pattern does not, which is why the suite asserts only
that the regions differ and prints the rest.

### Which way it grows

A local one call deeper sat at a **lower** address than its caller's, by 48
bytes.

The stack grows **downwards**, so "deeper" means numerically smaller, which is
the opposite of what the word suggests. That is a property of the processor,
the same on every toolchain this curriculum claims, so the suite does assert
it.

### How much room

The limit is a **setting**, not a property of C++.

| | |
| --- | --- |
| this machine | 8192 KiB, which `ulimit -s` prints and `setrlimit` changes |
| Windows | fixed at link time, default **1 MiB** |

So a recursion comfortable on Linux can end the same program on Windows: at 64
byte frames that is 10922 safe levels against 87381.

### What running out looks like

A recursion with a small frame reached depth **130860** and died with signal
11, which the shell reported as status **139**.

Lesson 00-09 explains that number, 128 plus the signal, and also why there was
no final line of output: what was still in `stdout`'s buffer went with the
process.

8 MiB over 130860 frames is **64 bytes a frame**, so the arithmetic holds, and
it is the arithmetic worth remembering:

> the depth you can afford is the limit divided by the frame.

A frame holding a 1 KiB buffer buys 5461 levels rather than 87381. The suite
prints the table:

```
  stack      frame   depth that fits
  8 MiB      64 B    87381
  8 MiB    1024 B    5461
  1 MiB      64 B    10922   (the Windows default)
```

Two thirds of the quotient rather than all of it, because the frame you
measured is one function and the real call chain includes everything it calls,
library code included. The measured crash was at 130860 and the figure above
is 87381, which is the margin doing its job.

### A large local costs its space on a path it is not on

This one is worth meeting deliberately, because it contradicts how the code
reads.

A `double local[2000000]`, 15625 KiB against an 8192 KiB limit, declared in
`main` beside a branch that used a `std::vector` instead:

| where the array was declared | the vector branch |
| --- | --- |
| in `main`, beside the branch | **crashed, status 139** |
| in its own function | survived, status 0 |

The array was never written on that path and never read. **The frame is
allocated when the function is entered**, not when the declaration is reached.

So several large locals in mutually exclusive branches can fail when no single
branch would, and the fix is to move each into the function that uses it, or
onto the heap.

### A variable length array is not C++

```cpp
double samples[n];      // n is a runtime value
```

C++ has no variable length arrays. C99 does, and both compilers offer it as an
extension, which is why it is so widely believed to be portable. Measured:

| flags | gcc 11 | clang 14 |
| --- | --- | --- |
| `-Wall -Wextra` | **silent** | **silent** |
| `-Wall -Wextra -Wvla` | error | error |
| `-pedantic-errors` | error | error |

Silent on both by default. It is not in `checks/` for that reason: a `checks/`
file has to be refused by every compiler, and this one is not.

The danger is not portability. It is that the size comes from the input, so a
function that puts `n` doubles on the stack is one input away from the crash
above. Switch `-Wvla` on, which is lesson 04-08's subject.

### Making the failure a value

The point of the artifact is not to make recursion safe. It is to turn a
signal that ends the process into a value you can return, so a planner handed
a map deeper than it expected **says so**.

```cpp
bool visit(const Node& node, rc::core::DepthLimit& limit) {
  const rc::core::DepthGuard guard(limit);
  if (!guard.ok()) return false;        // too deep: report, do not recurse
  for (const Node& child : node.children)
    if (!visit(child, limit)) return false;
  return true;
}
```

The suite measures both outcomes on the same limit of 8:

```
  a tree of depth 3 under a limit of 8: 15 nodes, limit never reached
  a tree of depth 20 under the same limit: 255 nodes visited, cut off at depth 8
```

The second answer is partial and says so. Without the guard it is not a
partial answer; it is signal 11 and no answer at all.

The decrement is a **destructor**, not a line at the bottom of the function,
for lesson 02-03's reason: an early return skips a line and skips no
destructor, and a depth counter that leaks reports a limit that was never
reached. The guard also deletes its copy operations, and
`checks/a_guard_that_was_copied.cpp` is that refusal: copying one would count
a level once and give it back twice.

## Build It

Three pieces.

The guard's constructor: refuse if the depth has already reached the limit,
recording that the limit was exceeded; otherwise take the level, update the
high water mark, and remember that you took it.

The guard's destructor: give the level back, **only if this guard took one**,
or the counter drifts below zero.

`depth_that_fits`: the limit divided by the frame, two thirds of it, with the
nonsense guarded so a zero frame does not divide by zero.

A note on the stub you are given: it refuses everything. That is the safe
direction to be wrong in, because a stub that allowed everything would recurse
until the stack ran out, and an exercise that ends in signal 11 tells you
nothing about which function to write.

## Use It

```cpp
#include <rc/core/recursion.hpp>

rc::core::DepthLimit limit(rc::core::depth_that_fits(8L * 1024 * 1024, 64));
if (!visit(root, limit))
  std::cerr << "map deeper than " << limit.max_depth() << ", answer is partial\n";
```

And print `high_water()` when it worked, because it says whether the limit was
close or nowhere near.

Where the recursion is yours to restructure, the better answer is a loop with
an explicit `std::vector` of pending work: that grows on the heap, which is
large, and the depth becomes a number you can print rather than a frame count
you cannot.

## What Breaks First

`E-MEM-0018` is the recursion that ended the program, with the depth, the
frame size and the Windows difference.

`E-CPP-0062` is the local that cost its space on a path it was not on, and the
variable length array both compilers accept in silence.

`E-DEBUG-0013` is a crash with no message, and how to tell a stack overflow
from a bad pointer, since both are `SIGSEGV`.

## Ship It

`rc/core/recursion.hpp`, in `namespace rc::core`.

From here on, any recursive function in this curriculum that walks input it
did not choose takes a `DepthLimit`.
