id: E-THREAD-0002
title: A thread that never notices a flag and runs forever
match: WARNING: ThreadSanitizer: data race.*bool
match: Timeout
platforms: linux, windows
teaches: 07-01-threads-and-the-torn-read
---

## Symptom

A worker thread is told to stop and keeps running. The program will not exit, or
a join never returns. Adding a print statement to the loop makes it work, which
is the strongest possible hint about the cause.

## Cause

The flag is a plain bool shared between threads with no synchronisation. That is
a data race, and a compiler entitled to assume no races may load the flag once
and keep it in a register for the life of the loop. The thread then never sees
the write, whatever the writer does.

Adding a print appears to fix it because the call forces the compiler to reload.
The race is still there.

## Fix

```cpp
std::atomic<bool> stop_{false};
```

An atomic is safe to read and write from several threads and establishes the
ordering the compiler must respect. For a single value that fits in a machine
word it costs almost nothing, and it is exactly the right tool for a stop flag.

std::atomic_flag and std::stop_token in C++20 are alternatives. volatile is not:
it prevents the compiler eliding the access but provides no ordering guarantee
between threads.

Note that this failure frequently does not reproduce. Measured on one machine,
the unsynchronised version passed its test every time. The race was real
throughout, and the thread sanitizer reported it immediately.
