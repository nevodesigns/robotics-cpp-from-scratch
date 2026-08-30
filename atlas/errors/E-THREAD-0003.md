id: E-THREAD-0003
title: Relaxed ordering where release and acquire were needed
match: WARNING: ThreadSanitizer: data race
match: expected this to be true: !wrong_contents
platforms: linux, windows
teaches: 07-02-a-queue-without-a-lock
---

## Symptom

A lock free queue occasionally delivers an item whose contents are wrong or half
written, even though the indices are atomic and the count is right. It happens
rarely, more often on a busy machine, and almost never on the developer's.

## Cause

An atomic index guarantees the index is never seen half written. It says nothing
about any other variable.

With relaxed ordering the consumer can observe the new index before it observes
the write to the slot that index refers to. The item is then read out of a slot
the producer has not finished writing, which is a torn read arriving by a subtler
route than the one in lesson 07-01.

## Fix

Release when publishing an index, acquire when reading the other thread's index:

```cpp
buffer_[tail] = reading;                       // ordinary write
tail_.store(next, std::memory_order_release);  // publishes it

const auto tail = tail_.load(std::memory_order_acquire);   // sees that write
out = buffer_[head];
```

Release and acquire work as a pair on the same variable. Release says everything
written before this store is visible to whoever acquires and sees this value.

Loading your own index relaxed is correct, because no other thread writes it.

Reach for sequentially consistent ordering, the default, when unsure. It is
slower and always correct, which is a far better position than fast and subtly
wrong. Run lock free code under the thread sanitizer, since this is exactly the
class of bug that testing does not find.
