id: E-TSAN-0001
title: ThreadSanitizer refuses to start with an unexpected memory mapping
match: FATAL: ThreadSanitizer: unexpected memory mapping
platforms: linux
teaches: 07-01-threads-and-the-torn-read
---

## Symptom

A program built with the thread sanitizer exits immediately, before running
anything, with:

```text
FATAL: ThreadSanitizer: unexpected memory mapping 0x60b5f6c91000-0x60b5f6c94000
```

The address differs every run, which is the clue.

## Cause

The thread sanitizer needs specific regions of the address space for its shadow
memory. Linux kernels from around 6.6 onward place mappings differently under
address space layout randomisation, and the sanitizer finds its regions already
occupied. It has nothing to do with your code.

Confirmed on Ubuntu 22.04 with kernel 6.8 and GCC 11.

## Fix

Disable randomisation for that process only:

```
setarch $(uname -m) -R ./your_test
setarch $(uname -m) -R ctest --test-dir build/tsan
```

The whole machine does not need changing, and `sudo sysctl kernel.randomize_va_space=0`
is a worse idea because it weakens every other process too.

This matters more than an inconvenience. Testing alone does not find races
reliably: measured in lesson 07-01, two genuine races were caught by tests zero
times out of five, and the thread sanitizer reported both immediately. A tool
that will not start is a tool that finds nothing.
