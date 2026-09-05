# Who Deletes This: Ownership, deleteLater and the Loop That Has to Run

> After deleteLater: one alive. After processEvents: one alive.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 09-03, 02-06

## The Problem

Lesson 09-01 introduced the parent-child tree: a `QObject` given a parent is
destroyed with it, and a stack object given a parent is a double delete waiting
to happen.

That is the easy half. The hard half is everything that follows from ownership
being decided at runtime, by somebody else, possibly on another thread, at a
moment nothing tells you about.

## The Concept

### deleteLater deletes nothing

It posts an event. The event is acted on when an event loop reaches it, and a
test usually has no event loop at all.

The part that surprises people is that `processEvents` does not reach it either:

| | alive |
|---|---|
| after `deleteLater` | 1 |
| after `processEvents` | **1** |
| after `sendPostedEvents(nullptr, QEvent::DeferredDelete)` | 0 |

That is deliberate, and the reason is good: deferred deletions are held back so
that an object cannot be destroyed while a nested loop is still executing inside
one of its own member functions. It is exactly the case `deleteLater` exists to
protect against.

In an application it works, because the main loop returns to the top and runs
them. In a test nothing collects them, which makes a real leak look like a
testing problem.

```cpp
inline void drain_deferred_deletes(QObject* only = nullptr) {
  QCoreApplication::sendPostedEvents(only, QEvent::DeferredDelete);
}
```

One line, in one named place, and the test says what it means.

Note also that an object with a pending `deleteLater` is still alive and still
connected. It will receive signals in the meantime, and a slot that assumes it
is on its way out has to check for itself.

### A pointer that knows

A raw pointer to a destroyed `QObject` holds an address that used to mean
something. There is no way to ask it anything.

```cpp
QPointer<Counted> watching(object);
delete object;
// watching.isNull() is true; the raw pointer is unchanged and unusable
```

A `QPointer` is told when its object is destroyed. It is the only way to hold a
reference to something owned elsewhere and still be able to check, and Qt's
ownership makes "owned elsewhere" the normal case: a parent destroys its
children, so a `delete` in a completely different file invalidates your pointer.

This is the `weak_ptr` from lesson 02-06 arrived at from the other direction.
There, ownership is shared and a weak reference declines to extend it. Here,
ownership belongs to a parent and a `QPointer` declines to pretend otherwise.

### Ownership moves

| | first parent's children | second's | alive |
|---|---|---|---|
| after construction | 1 | 0 | 3 |
| after `setParent(second)` | 0 | 1 | 3 |
| after `delete first` | | 1 | **2** |

The child survives its original parent, because responsibility for it moved.

So **"who deletes this" is a question whose answer can change at runtime**, and
nothing tells the holder of a raw pointer either way.

Two related facts worth having straight:

A child deleted on its own **removes itself from the parent's list**, so the
parent has nothing to delete twice. That is why deleting a heap child early is
safe.

And it is why a stack object with a parent is not: its destructor runs at the
closing brace whatever Qt believes, and by then the parent may already have
deleted it, or may be about to.

### The thread that owns it

A `QObject` belongs to a thread. Its destructor takes it out of that thread's
event queue and unhooks its connections, and both are structures that thread
owns.

Deleting it from another thread modifies them without synchronisation. Like most
races it usually appears to work, because the window is a few instructions wide.

```cpp
inline void delete_from_its_own_thread(QObject* object) {
  if (object == nullptr) return;
  if (object->thread() == QThread::currentThread()) {
    delete object;
    return;
  }
  object->deleteLater();
}
```

On its own thread the deletion is immediate and needs no loop. From anywhere
else, `deleteLater` posts it to the owning thread, and **that thread's loop
performs it**: measured, the object stays alive until the worker is given a
chance to run and is gone once the thread has finished.

This is the one place `deleteLater` is not a convenience. It is the only correct
answer.

Which fixes the order of a shutdown: **ask, then quit, then wait**. Quitting
first can leave the deletion posted to a loop that will never run again, which
is a leak rather than a crash and therefore harder to see.

And give worker objects no parent across a thread boundary, or the parent will
try to delete them from its own thread on the way out.

## Build It

Implement `drain_deferred_deletes`, `delete_from_its_own_thread` and
`still_alive` in `exercise/solution.hpp`.

```
rcpp verify 09-04
```

The suite counts objects into and out of existence, watches `deleteLater` do
nothing twice, reparents a child out from under a deletion, and makes an object
on a worker thread for that thread to destroy.

Everything runs offscreen with no display.

## Use It

**Say who owns each object**, in a comment, where it is created. A parent, a
`unique_ptr`, or a container: one of the three, named.

**Hold a `QPointer` to anything you do not own**, and check it before use.

**Never give a stack object a parent.**

**Use `delete_from_its_own_thread`** rather than deciding case by case, and let
the function carry the rule.

**Drain the deferred deletes in tests**, and assert with a `QPointer` that the
object actually went.

## What Breaks First

- **deleteLater in a test, and an object still there.** See `E-QT-0015`.
- **A raw pointer to an object somebody else deleted.** See `E-QT-0016`.
- **A QObject deleted by the wrong thread.** See `E-QT-0017`.

## Ship It

`drain_deferred_deletes`, `delete_from_its_own_thread` and `still_alive` join
`rc::qt` beside the worker from 09-03. Every Qt test in this curriculum can now
prove an object was destroyed rather than assuming it, and every worker has one
correct way to be shut down.
