id: E-TSAN-0004
title: A seqlock that works, is a data race, and only one sanitizer says so
match: (seqlock|data race on a copy of a value another thread writes|memcpy of a shared value)
platforms: linux
teaches: 07-12-many-readers-one-writer
---

## Symptom

A seqlock built with a plain `memcpy` of the data produces no torn reads in any test, on any run,
and the thread sanitizer either reports a data race or says nothing, depending on the compiler.

## Cause

A seqlock reader copies the data while a writer may be changing it, and trusts the copy only if the
counter says nothing changed. Reading memory another thread may be writing, without atomics, is a
**data race** in C++, whatever the hardware does with it. The language does not promise the copy is
garbage that will be thrown away. It promises nothing.

Measured, 200000 writes against a reader, on x86:

| data held as | torn reads | gcc 11 tsan | clang 14 tsan |
|---|---|---|---|
| a plain struct, copied with `memcpy` | **0** | 0 reports in 5 runs | **20 reports** in 5 runs |
| an array of atomic words | 0 | 0 | 0 |

It worked every time, and clang 14's thread sanitizer was right to say it is a race. GCC 11's did
not see it, because it does not model the standalone fences the algorithm is built on. A clean report
from the tool that misses it proves nothing.

## Fix

Hold the data as atomic words, written and read with `relaxed` operations between the counter's
fences. That is legal C++ and costs nothing extra on x86:

```cpp
std::atomic<unsigned long long> data_[kWords];
...
for (std::size_t i = 0; i < kWords; ++i) data_[i].store(words[i], std::memory_order_relaxed);
```

The type is limited to trivially copyable ones, since the words are its bytes, and a type that is
not is refused at compile time with a `static_assert`.

Run both sanitizers on lock free code and trust the stricter. A bug that two compilers disagree
about is a bug.
