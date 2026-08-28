# RAII: Attaching Cleanup to the Object Instead of the Code Path

> The best idea in this language has an unfortunate name. What it means is that cleanup happens because the object ended, not because you remembered.

**Type:** Build
**Time:** about 105 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** none, the device is simulated
**Prerequisites:** 02-02

## The Problem

Last lesson ended with a structural complaint: the release is written at a place
in the text, and every way out of the function has to remember it. Adding an
early return silently leaks.

Memory is only the mildest version. A robot holds far more dangerous things:

- A serial port, which no other process can open while you hold it.
- A file, whose data is lost if it is not flushed and closed.
- A lock, which stops every other thread until you release it.
- A motor enable line, which leaves the machine live until you drop it.

Forget the release on one path and you leak a few bytes. Forget it on a motor
enable and the robot keeps driving.

## The Concept

C++ gives every object a **destructor**: a function that runs automatically when
the object's lifetime ends. Not when you call it. When the object ends.

```cpp
class DeviceHandle {
 public:
  DeviceHandle() { id_ = open_device(); }     // acquire
  ~DeviceHandle() { close_device(id_); }      // release
 private:
  int id_ = -1;
};
```

Now consider every way out of a function holding one of these on the stack:

- It returns normally. The destructor runs.
- It returns early, from inside a loop, from inside a branch. The destructor
  runs.
- It throws an exception. The destructor runs as the stack unwinds.
- Somebody adds a fourth exit next year without reading this file. The destructor
  runs.

The cleanup is attached to the **object's lifetime** rather than to a line of
code, so the compiler is the one remembering, on every path, including paths
nobody has written yet.

The name is Resource Acquisition Is Initialisation, usually shortened to RAII. It
is a poor name for a good idea: acquire the resource in the constructor, release
it in the destructor, and put the object on the stack.

### Order is guaranteed

Objects are destroyed in reverse order of construction, so a handle created after
a lock is destroyed before that lock. That is what makes layered resources work
without any bookkeeping.

### Copying is the trap

The moment a handle can be copied, RAII breaks:

```cpp
DeviceHandle a;
DeviceHandle b = a;   // both hold the same id
                      // both destructors run, closing it twice
```

A double release is worse than a leak. For a file descriptor it can close a
descriptor another part of the program has since opened for something else, and
the resulting bug appears nowhere near the cause.

So a type that owns a resource must decide what copying means. Usually the answer
is that it cannot be copied at all:

```cpp
DeviceHandle(const DeviceHandle&) = delete;
DeviceHandle& operator=(const DeviceHandle&) = delete;
```

`= delete` is not a comment or a convention. The compiler refuses the copy at the
point somebody tries to write it, which is where the mistake actually is. Lesson
02-04 covers the alternative answer, which is to move the ownership rather than
duplicate it.

### The standard library is made of this

`std::vector`, `std::string`, `std::unique_ptr`, `std::lock_guard` and
`std::fstream` are all this idea. Once you see it, most of the library stops
looking like a collection of features and starts looking like one pattern applied
to different resources.

## Build It

`exercise/solution.hpp` provides a fake device with an API deliberately shaped
like a real one: `fake_open` hands out an id, `fake_close` gives it back, and
`fake_open_count` reports how many are outstanding.

Implement `DeviceHandle` so that:

- The constructor opens a device and remembers its id.
- The destructor closes it, and only if it was actually opened.
- Copying is refused by the compiler.
- `is_open()` reports whether it holds a device.
- `id()` returns the id it holds, or `kNoDevice` when it holds none.
- `close()` closes early and is safe to call twice, so the destructor does not
  close an already closed device.

```
rcpp verify 02-03
```

The tests check that the count returns to zero after normal exits, early
returns, and an exception in flight.

## Use It

`std::unique_ptr` is exactly this with a custom release action, and `std::fstream`
is exactly this for files. In phase 08 you will write a real serial port class
with this shape, and in phase 12 a Qt device wrapper with it.

Outside C++ the same idea appears with a different price: Python's `with` and
C#'s `using` require the caller to remember the block. RAII puts the guarantee in
the type, so a caller who forgets nothing still gets it right by default.

## What Breaks First

- **The device is closed twice.** Your handle was copied, and both copies closed
  the same id. Delete the copy operations. See `E-MEM-0002`.
- **The destructor closes a device that was never opened.** Guard on the id
  before releasing. See `E-MEM-0006`.
- **The resource leaks anyway.** The object was allocated with `new` and never
  deleted, so its destructor never ran. Put it on the stack. See `E-MEM-0004`.

## Ship It

`DeviceHandle` is the shape of every driver in this curriculum. The serial port
in phase 08, the camera in phase 15 and the ROS 2 node in phase 17 are all this
class with a different resource, and none of them will need a line of cleanup at
a call site.
