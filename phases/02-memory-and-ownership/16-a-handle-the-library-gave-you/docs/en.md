# A Handle the Library Gave You: Making a C Open and Close Pair Release Itself

> A C library gives you a pointer from an open function and expects one call to its close function on it.
> Every way out of your code between the two has to remember.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 02-03, 02-04

## The Problem

Serial ports, sockets, camera devices, files and the drivers of most sensors are C libraries. They give you an opaque pointer or a number from an open function, and they want it passed to their own close
function exactly once, because only the library knows how to give back what it took.

Lessons 02-03 and 02-04 made `new` and `delete` safe with an object that does it for you. The library's handle is not memory from `new`, and `delete` on it is wrong. The same idea has to be taught to use the library's own close.

## The Concept

### The pair, and the ways out

```cpp
Device* first = device_open(first_id);
if (first == nullptr) return false;
Device* second = device_open(second_id);
if (second == nullptr) return false;     // first is still open
```

There are several ways out between the open and the close: an early return, an exception, a condition nobody thought of. Each is a chance to skip the close.

Measured, with a fake library that counts what is done to it: **100** attempts where the second device could not be opened. Raw handles left **100** devices open. The owner version left **0**. See `E-MEM-0029`.

### unique_ptr with the library's own close

`std::unique_ptr` can be told what to call instead of `delete`. A small class with a call operator is enough:

```cpp
struct DeviceCloser {
  void operator()(Device* d) const {
    if (d != nullptr) device_close(d);
  }
};
using DevicePtr = std::unique_ptr<Device, DeviceCloser>;
```

Without that argument a `unique_ptr<Device>` would call `delete` on a pointer the library allocated, which is wrong however it looks. With it the handle is closed when the `DevicePtr` goes out of scope, by whichever way
it goes.

### Exceptions, and a handle that never opened

An exception thrown while holding a device: opened **1**, closed **1**. A device that failed to open leaves an empty `DevicePtr`, and its destruction made **0** bad closes: `unique_ptr` does not call the deleter for a null pointer, and the closer checks too, because a
closer that is called by hand might not. See `E-MEM-0030`.

### Handing it on

A raw pointer copied to two places has two owners and each closes. A `unique_ptr` cannot be copied. Two devices, one handed over with `std::move`, one replaced by an assignment: opened **2**, closed **2**, bad closes **0**. The assignment closed the
device it overwrote. Code that only uses a device takes `d.get()`, a plain pointer that does not own. See `E-MEM-0031`.

### What it costs

`sizeof(Device*)` was 8 and `sizeof(DevicePtr)` was 8: a closer with no data takes no space.

## Build It

`DeviceCloser`, `open_checked` and `read_pair` in `exercise/solution.hpp`. The fake library, with its counters, is given.

```
rcpp verify 02-16
```

The test also keeps the raw version of `read_pair`, so the leak can be measured. It records the leaked pointers so that it can close them afterwards, which a real program could not.

## Use It

`rc/core/handle.hpp` joins `rc::core`. You wrote the closer by hand for one library. The header is the same thing as a template, for any type and any close function, which lesson 03-07 explains:

```cpp
void close_port(Port*);
using PortPtr = rc::core::Handle<Port, close_port>;
```

**What this does not do.** It is for libraries whose close takes the same pointer the open returned. A handle that is a number, like a file descriptor, needs a small class around it, because `unique_ptr` holds a pointer. It does not make the library's close safe to
call twice, only makes sure it is called once. It does not know whether closing can fail: a close that returns an error code has nowhere to put it from a destructor. And it does not say anything about what other threads may do with the handle.

## What Breaks First

- **The early return.** The first device stays open. See `E-MEM-0029`.
- **The exception, and the null.** Left open, or closed when never opened. See `E-MEM-0030`.
- **The copied pointer.** Two owners, two closes. See `E-MEM-0031`.

## Ship It

`rc/core/handle.hpp` holds `Closer` and `Handle`.

That finishes the memory phase's own ground. The subsystem tree of lesson 02-06 releases everything it owns, and this is how the same is made true of what a library owns.
