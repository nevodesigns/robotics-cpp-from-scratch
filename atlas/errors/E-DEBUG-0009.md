id: E-DEBUG-0009
title: The print that never appeared, so you blamed the wrong line
match: the last thing it printed is not where it stopped
platforms: linux, windows
teaches: 00-09-what-the-program-tells-the-outside-world
---

## Symptom

A program crashes. You add prints to find out where, the way lesson 00-06
taught, and the output stops in a place that makes no sense: before a line you
can prove ran, or in the middle of a function that has no way to fail.

So you spend an afternoon on the wrong function.

It is worse when it runs under something else. The same program prints
everything when you run it by hand and loses the end of its output in
continuous integration, or into a log file, or through a pipe.

## Cause

`stdout` is **block buffered** when it is not a terminal. Your text goes into
a buffer in your process and is written out when the buffer fills or when the
program ends tidily. A program that dies does not end tidily, so whatever is
still in that buffer is gone.

This is why it behaves differently by hand: to a terminal, `stdout` is line
buffered, so each line leaves as you print it. Under a script, a log file or
a build server, it is not, and those are the runs you cannot watch.

`stderr` is not buffered. It leaves immediately, and it survives.

Measured in lesson 00-09. Four lines printed, then a deliberate segmentation
fault, output going into a pipe:

```
A printf, buffered        arrived
B cout, buffered          arrived
C cerr, not buffered      arrived
D cout again              LOST
```

The interesting part is **why A and B arrived**. The write to `std::cerr`
flushed them, because `std::cerr` is tied to `std::cout`, so the unbuffered
line dragged the waiting ones out with it. The line printed after it had
nothing to drag it out and was lost.

So which of your diagnostics you see depends on where the unbuffered one
happened to be, and **the line you never saw is not the line it stopped at.**

With no `std::cerr` at all, both buffered lines were lost and the output
stopped two statements early.

One more measurement, which is the reverse of the usual advice. With `printf`
and `fprintf(stderr)` the ordering genuinely does break when `stdout` is not a
terminal: stderr arrived first and stdout afterwards in a block. With
`std::cout` and `std::cerr` the order was kept, because of that tie. Audit
rule L018 forbids `printf` in this curriculum, and this is the measured reason
rather than a style preference.

## Fix

Put diagnostics on `stderr`. Not for redirection hygiene, which is the reason
usually given and the less important one, but because it is the only output
you will still have when the program dies.

```cpp
std::cerr << "about to normalise, length is " << length << "\n";
```

Three further things, in the order they are worth doing.

**Prefer a check to a print.** `rc/core/expect.hpp` from lesson 04-06 reports
and keeps going in the build you ship, which is more than a print does and it
is still there in release.

**Use the debugger.** Lesson 04-02 covers reading a crash, and a backtrace
tells you where it stopped without any of this. Prints are for when you cannot
attach one.

**If you must print to stdout, flush it.** `std::cout << std::flush` or
`std::endl`, which flushes as well as writing a newline. This costs a system
call per line, which is exactly why it is not the default, so flush where you
need the guarantee rather than everywhere.

And when the output simply stops: do not believe the last line. Check whether
the next thing you expected writes to a different stream.
