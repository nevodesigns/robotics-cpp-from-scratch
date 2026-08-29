# Reading a Crash: Finding Your Own Frame

> Nine frames of template soup, and the answer is in the one that mentions a file you wrote.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 04-01

## The Problem

Here is a real crash, produced while writing this lesson. It has been shortened,
and nothing has been rephrased.

```text
ERROR: AddressSanitizer: heap-buffer-overflow on address 0x511000000140
    #0 in __gnu_cxx::__ops::_Iter_comp_iter<bool (*)(int, int)>::operator()<...>
       /usr/include/c++/11/bits/predefined_ops.h:158
    #1 in std::__unguarded_partition<...>  /usr/include/c++/11/bits/stl_algo.h:1884
    #7 in std::sort<...>                   /usr/include/c++/11/bits/stl_algo.h:4875
    #8 in main                             /tmp/badcomp.cpp:10
```

The first instinct is to read frame zero, and frame zero is inside the standard
library. From there the reasonable conclusion is that something is wrong with
`std::sort`, and the next hour goes into reading the standard library.

Nothing is wrong with `std::sort`. The only frame in code anybody wrote is `#8`,
`main` at line 10, which is the line that calls it. The actual defect is a
comparator that reports an element as less than itself, so the sort walks off the
end of the range looking for a pivot that cannot exist.

The skill this lesson teaches is the thirty seconds before the hour: what kind of
failure is it, and which frame is mine.

## The Concept

### A backtrace is the stack, most recent first

Frame zero is where execution was when it stopped. Frame one called frame zero,
frame two called frame one, and so on out to `main`.

Two formats matter and they are nearly the same. A debugger writes:

```text
#0  0x000055555555527c in worse (a=..., b=...) at deep.cpp:7
```

A sanitizer writes:

```text
    #0 0x6308fea44b35 in sum_readings(int const*, int) /tmp/crash.cpp:4
```

Both give an index, an address, a function after the word `in`, and a file with a
line number. The debugger puts `at` before the file and the sanitizer does not.
That is the whole difference, and it is why one small parser handles both.

### The first frame in your own code is where to look

Frames inside `/usr/include`, `/lib`, or the C library are almost never the
defect. They are where a correct library reacted to something you handed it.

So the rule is: **walk down from frame zero until you reach a file you wrote,
and start there.** In the crash above that is frame eight, `main`, and the bug is
in the argument passed on that line.

It is not an absolute rule. A genuine library bug exists somewhere. But it is
right so overwhelmingly often that starting anywhere else is a poor bet, and
noticing the rule is what separates half a day from five minutes.

### What the kind of crash already tells you

The first line names the failure, and each kind has a short list of causes.

| Report says | Nearly always |
|---|---|
| `stack-buffer-overflow` | Ran past the end of a local array. An index or a loop bound |
| `heap-buffer-overflow` | Ran past the end of an allocation, or an invalid comparator |
| `heap-use-after-free` | Held a pointer or a view past the life of what it pointed at |
| `SIGSEGV` | Dereferenced null, or something already destroyed |
| `SIGFPE` | Integer division by zero, despite the name mentioning floating point |
| `LeakSanitizer` | Something acquired was never released |

Reading that line before the frames narrows the search before you have looked at
a single line of code.

### Using a debugger, briefly

The exercise is a triage tool, but the tool does not replace knowing five
commands. On Linux and macOS:

```text
gdb ./build/default/bin/some_test     start it under the debugger
run                                   run until it stops
bt                                    the backtrace
frame 8                               move to a frame
print variable                        show a value in that frame
break file.cpp:42                     stop at a line, then run again
```

`lldb` uses the same words with small differences, and Visual Studio has all of
it behind buttons. Under a debugger you can also ask questions a report cannot
answer, such as what the value actually was at the moment it went wrong.

The quickest route to a backtrace with no interaction at all is the one used to
capture the fixtures in this lesson:

```text
gdb -q -batch -ex run -ex bt ./your_program
```

## Build It

`exercise/solution.hpp` gives you `Frame` and `CrashKind`. The tests use three
crash reports captured while writing this lesson, verbatim.

Implement:

- `parse_frames(report)`, handling both the debugger and sanitizer shapes, and
  ignoring lines that are not frames.
- `is_system_frame(frame)`, true for a frame with no file, or a file under
  `/usr/`, `/lib`, or a relative path into the C library such as `../sysdeps/`.
- `first_own_frame(frames)`, the first frame that is not a system frame, or
  `TriageError::NoOwnFrame` when every frame is one.
- `classify(report)`, returning the `CrashKind` from the first line.

```
rcpp verify 04-02
```

## Use It

This is `rcpp explain` from the other side. The atlas matches an error to a
catalogued cause; this pulls the facts out of a crash so you know which entry to
look for and which line to open.

In production the same job is done by a crash reporter that symbolises a
backtrace from the field, and the same rule applies: find the first frame in your
own code, because that is where your build has source for and where the fix will
be.

## What Breaks First

- **A buffer overflow with no obvious index mistake.** Look one call further out.
  An invalid comparator, or a size passed separately from a pointer, produces
  this. See `E-CPP-0007`.
- **A use after free with the free nowhere near the read.** Something held a view
  of data that had been released. See `E-MEM-0001`.
- **A backtrace with no line numbers at all.** The binary was built without
  debug information. See `E-DEBUG-0001`.

## Ship It

The triage tool joins `rc::core`. The habit is the artifact: read the kind, find
your own frame, then open that line. It works on every crash you will meet for
the rest of your career, in any language with a stack.
