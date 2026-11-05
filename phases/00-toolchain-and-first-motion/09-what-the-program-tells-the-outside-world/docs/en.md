# What the Program Tells the Outside World: Two Streams and One Number

> Four lines were printed before a deliberate crash and three of them arrived.
> Not the first three in the order you would guess, and the one that was lost
> is the reason people spend afternoons on the wrong function.

**Type:** Build
**Time:** about 90 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 00-01, 00-04, 00-06

## The Problem

Your programs have printed things and ended. Lesson 00-06 used printing to
find a wrong answer, and it worked, because you were sitting there watching.

Almost nothing that runs your code will be sitting there watching. A script
runs it. A build server runs it. The thing that starts your robot when power
comes up runs it, at three in the morning, with nobody logged in.

All of those need to know two things: what happened, and whether it worked.
C++ gives you exactly two ways to tell them, and they are smaller than you
would guess.

**Two streams of text. One number.**

## The Concept

### The one number

`main` returns `int`. Whatever it hands back is what the shell, the build
system or the supervisor reads, and the convention is older than C++:

**Zero means it worked. Anything else means it did not.**

That is the whole protocol. A shell asks one question of a finished program,
which is whether to carry on, and zero says yes.

Note how counter-intuitive that is. Zero is the success here, and every other
number is a failure, which reads backwards from a `bool`. That is exactly why
this lesson writes it down once, in a function called `succeeded`, rather than
leaving every call site to remember.

### The number is not an int

Whatever the signature of `main` says, only the **low eight bits** survive.
What a shell sees is your return value modulo 256. The suite prints it:

```
  returned   what a shell sees
  0          0
  1          1
  2          2
  255        255
  256        0
  257        1
  -1         255
```

The 256 row is the defect, and it is the natural mistake:

```cpp
int main() {
  const int problems = check_everything();
  std::cerr << problems << " problem(s) found\n";
  return problems;            // reports success at exactly 256 problems
}
```

This is lesson 00-04's truncation in a place nobody looks for it. No warning is
available, because the truncation happens when the process ends rather than in
any expression the compiler can see.

A negative return is the same thing from the other side. `return -1;` arrives
as 255, which happens to be a failure and so is harmless, and it is not the
number anybody intended.

**And a status above 128 usually means the program did not choose to stop.** A
segmentation fault was measured as 139, which is 128 plus `SIGSEGV`'s 11. So
that range is not yours to use: a tool returning 139 on purpose cannot be told
apart from one that crashed. It is also why this repository's exercise gate
checks for exit 1 specifically rather than merely non-zero.

So collapse the count on purpose:

```cpp
std::cerr << problems << " problem(s) found\n";
return rc::core::status_for(problems);
```

One problem and a thousand problems are the same answer to the only question
being asked. The quantity is interesting to a **person**, so it goes in the
message, where nothing truncates it.

### The two streams

`stdout` is for the answer. `stderr` is for the commentary.

The usual reason given is redirection: a caller can keep one and discard the
other, which is how a tool fits into a pipeline.

```
$ ./plan_route 2>&1 >/dev/null
2 stderr
4 stderr
```

That reason is real and it is the less important one.

### The real reason, measured

`stdout` is **block buffered** when it is not a terminal. Your text sits in a
buffer inside your process and is written out when the buffer fills or when the
program ends tidily. A program that crashes does not end tidily.

`stderr` is not buffered. It leaves immediately.

Four lines, then a deliberate segmentation fault, output into a pipe:

| printed | arrived? |
| --- | --- |
| `printf("A printf, buffered")` | yes |
| `std::cout << "B cout, buffered"` | yes |
| `std::cerr << "C cerr, not buffered"` | yes |
| `std::cout << "D cout again"` | **lost** |

The interesting question is why A and B arrived.

The write to `std::cerr` flushed them. `std::cerr` is **tied** to `std::cout`,
so writing to the unbuffered stream drags whatever is waiting on the buffered
one out with it. The line printed *after* it had nothing to drag it out, and it
was lost.

So which of your diagnostics you see depends on where the unbuffered one
happened to be, and:

**The line you never saw is not the line it stopped at.**

With no `std::cerr` in the program at all, both buffered lines were lost and
the output stopped two statements early. That is the afternoon on the wrong
function.

It is also why the same program behaves differently when you run it by hand.
To a terminal, `stdout` is line buffered, so each line leaves as you print it.
Under a script, a log file or a build server it is not, and those are precisely
the runs you cannot watch.

### One measurement that reverses the usual advice

You will read that `stdout` and `stderr` arrive out of order when redirected.
With C's `printf` and `fprintf(stderr)`, that is exactly what happened: stderr
first, then stdout afterwards in a block.

With `std::cout` and `std::cerr` the order was **kept**, because of that tie.

Audit rule L018 already forbids `printf` in this curriculum. This is the
measured reason, rather than a style preference.

### What a stream will not tell you

```cpp
return std::cerr << "cannot open the route file\n";   // does not compile
```

Each `<<` hands back the stream, so the next one has something to attach to.
That is what makes a chain work. It does not hand back whether anything was
written, and `checks/a_stream_is_not_a_status.cpp` is the refusal.

This matters more than it looks. A stream that has failed, because the disk
filled or the pipe at the other end closed, **does not throw and does not say
so.** It sets a flag on itself and silently discards everything afterwards. A
program whose only report is its printing can print nothing at all and still
end with the number 0.

So: two lines, always. The message for the human, the number for the machine.

```cpp
std::cerr << "cannot open " << path << "\n";
return rc::core::kFailed;
```

## Build It

Three small functions and one decision.

`status_for(int problem_count)`: zero is `kSuccess`, anything else is
`kFailed`, including a negative count, because a count that came out negative
is itself a problem and must not read as everything being fine.

`status_for_misuse()`: one line. Keeping misuse apart from failure is what
lets a script tell a broken input from a broken machine, which are two
problems with two different people to tell about them.

`succeeded(int status)`: one comparison, and it is the counter-intuitive one.

Then the decision, which is the actual lesson: resist returning the count.

## Use It

```cpp
#include <rc/core/exit_status.hpp>

int main() {
  const int problems = check_everything();
  if (problems > 0) {
    std::cerr << problems << " problem(s) found\n";
    return rc::core::status_for(problems);
  }
  return rc::core::kSuccess;
}
```

From here on in this curriculum: data a later program reads goes to `stdout`,
anything a person reads goes to `stderr`, and the number says only whether to
carry on.

## What Breaks First

`E-CPP-0060` is `main` asked to hand back a message, a stream, or nothing at
all, and why each is refused.

`E-NUM-0024` is the count that was truncated into success, with the full table.

`E-DEBUG-0009` is the print that never appeared, which is the one to read
before you next debug a crash by printing.

## Ship It

`rc/core/exit_status.hpp`, in `namespace rc::core`.

Three functions and three named constants. The header is short and the comment
above it is not, because the measurements are the part that is hard to come by
and easy to forget.
