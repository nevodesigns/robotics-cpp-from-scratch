id: E-DEBUG-0013
title: A crash with no message and no last line
match: (core dumped|status 139|128 plus the signal)
platforms: linux, windows
teaches: 02-10-where-the-locals-live
---

## Symptom

The program stops. There is no exception, no error text, and the output ends
somewhere that makes no sense: before a line you can prove executed, or in the
middle of a function with no way to fail.

```
Segmentation fault (core dumped)
$ echo $?
139
```

Under a build server there is often nothing at all, because the log ends
mid-sentence.

## Cause

Three things conspire, and each is covered by a lesson this one depends on.

**The status is not an exit code.** 139 is 128 plus 11, and 11 is `SIGSEGV`.
Lesson 00-09 measured the arithmetic. A program that returns a status above
128 has almost certainly not chosen to stop, so that range is not yours to
use: a tool returning 139 on purpose cannot be told apart from one that
crashed.

**The last line of output is missing, not the last line executed.** `stdout` is
block buffered whenever it is not a terminal, so whatever was waiting in that
buffer went with the process. Lesson 00-09's measurement: of four lines
printed before a deliberate crash, the three before the `std::cerr` write
arrived and the one after it was lost, because writing to `cerr` flushed what
was waiting. **The line you never saw is not the line it stopped at.**

**A stack overflow looks exactly like a null dereference.** Both are
`SIGSEGV`. There is no separate signal for running out of stack, and no
message distinguishing them.

## Fix

Find out which kind of `SIGSEGV` it was, because the two have nothing in
common.

**Look at the backtrace depth.** Lesson 04-02 covers reading a crash. A
backtrace thousands of frames deep, repeating the same function, is a stack
overflow; one a handful of frames deep is a bad pointer. In gdb, `bt -20` and
`bt 20` show both ends, which is the quickest discrimination there is.

**Ask the sanitizer.** The address sanitizer names it:
`stack-overflow` for one, `SEGV on unknown address` for the other, and lesson
04-07 covers what it does and does not see.

**Put the diagnostics on `stderr`**, which is not buffered, so the output you
have when it dies is the output you actually wrote.

Then fix the right thing. For a stack overflow, `rc/core/recursion.hpp` and
`E-MEM-0018`: give the recursion a limit it can report against, so deep input
ends in a value rather than a signal. For a bad pointer, `E-MEM-0001` and
lesson 02-07.

One measurement worth carrying: a recursion with 64 byte frames reached depth
**130860** on an 8 MiB stack before dying. If a backtrace is anywhere near
that deep, stop reading it and go to the depth limit.
