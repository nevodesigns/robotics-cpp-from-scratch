id: E-THREAD-0004
title: A ring buffer that reports full when empty, or loses one slot
match: expected .*size\(\) to equal
match: expected this to be true: queue.push
platforms: linux, windows
teaches: 07-02-a-queue-without-a-lock
---

## Symptom

A queue of capacity four accepts only three items. Or it reports itself full
immediately after being drained. Or everything works until the indices wrap for
the first time and then one item goes missing on every lap.

## Cause

Two arithmetic mistakes that look identical from outside.

The wasted slot is not accounted for. Keeping one element empty is what makes
head equal to tail mean empty unambiguously, so the storage must be one larger
than the advertised capacity. Allocating exactly the capacity loses one slot from
the queue the caller was promised.

Or the wrap is taken against the wrong number. The modulus belongs against the
storage size, which is capacity plus one. Using the capacity works perfectly
until the first wrap and then drops an element per lap, which is the kind of
error that reaches a robot because the first pass through any test is clean.

## Fix

Allocate capacity plus one, and wrap against the storage size:

```cpp
buffer_(capacity + 1)

std::size_t advance(std::size_t index) const {
  return (index + 1) % buffer_.size();
}
```

Test past the first wrap. A test that pushes and pops a few items exercises none
of this; one that goes round a small queue a thousand times exercises all of it.
