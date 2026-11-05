id: E-TSAN-0002
title: ThreadSanitizer reports a double lock on a condition variable that is correct
match: (double lock of a mutex|pthread_cond_clockwait)
platforms: linux
teaches: 07-06-waiting-without-spinning
---

## Symptom

The thread sanitizer reports `double lock of a mutex` and a handful of data races,
inside the code that waits on a condition variable with a timeout, in a program
that is correct.

```
WARNING: ThreadSanitizer: double lock of a mutex
    #4 BlockingQueue<int>::push(int const&) ...
```

The stack is a plain `std::lock_guard` in a producer.

## Cause

A tool limitation, and not a bug in the program.

`std::condition_variable::wait_until` with a `steady_clock` deadline, and `wait_for`,
call `pthread_cond_clockwait` in libstdc++ from GCC 10 onwards. The thread sanitizer
that ships with GCC 11 does not intercept that function. It therefore never sees the
mutex being released for the duration of the wait, and believes the next thread to
lock it is locking a mutex that is already held.

Measured on a program of twelve lines, a waiter and a producer:

| wait used | gcc 11 | clang 14 |
|---|---|---|
| `wait_until(steady_clock)` | **2 warnings** | 0 |
| `wait()` with no timeout | 0 | 0 |
| `wait_until(system_clock)` | 0 | 0 |

The lesson's own suite under the GCC 11 thread sanitizer reported 11 warnings: 8 data
races and 3 double locks. Under clang 14's it reported none, on three runs.

## Fix

Do not change the program to satisfy the tool. `system_clock` would silence it and
would make the timeout jump when the clock is adjusted, which is a real bug.

Run the thread sanitizer with clang for code that waits with a timeout:

```
cmake -S . -B build/clang-tsan -G Ninja -DCMAKE_CXX_COMPILER=clang++-14 \
      -DRC_SANITIZE_THREAD=ON
```

or use a newer GCC, where the interceptor exists. A report whose stack passes through
a `wait_until` and a plain `lock_guard` is the signature of this problem: check it
with a twelve line program like the one in the table before spending a day on it.
