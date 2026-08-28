# unique_ptr and Move: Handing Ownership Over

> Copying asks who else has one. Moving asks who has it now. For anything that owns a resource, only the second question has a good answer.

**Type:** Build
**Time:** about 105 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 02-03

## The Problem

Lesson 02-03 made a handle safe by refusing to copy it. That works right up to
the moment you need to give it to somebody.

A robot opens six devices at startup and hands them to whichever subsystem uses
each one. A factory function creates a device and returns it. A registry holds
them all and hands one out on request. None of that is copying, and all of it is
currently impossible, because a non copyable object cannot leave the function
that built it.

What is needed is a way to say: I had this, now you have it, and I no longer do.

## The Concept

### Moving is not copying

C++11 added a second way to pass an object. A **copy** produces a second
independent thing. A **move** transfers what the object holds and leaves the
original in a valid but empty state.

For a resource that is exactly right. There is still one device, one file, one
lock. Only the owner changed.

```cpp
DeviceHandle a;
DeviceHandle b = std::move(a);   // b holds the device, a holds nothing
```

`std::move` does not move anything. It is a cast that says "treat this as
something you may take from". The actual transfer is done by the move constructor
of the type. The name is misleading and it is worth saying that out loud once.

### The moved from object is empty, not destroyed

After a move, the source still exists and its destructor will still run. It must
therefore be left in a state that is safe to destroy, which for a handle means
holding nothing.

That gives the one rule to remember: **after moving from an object, do not use it
for anything except assigning to it or destroying it.** Reading a moved from
handle is not undefined behaviour, but the value is not yours to rely on.

### unique_ptr is the handle from last lesson, for heap objects

`std::unique_ptr<Device>` owns exactly one heap object and deletes it in its
destructor. It cannot be copied. It can be moved. It is the whole of lesson 02-03
applied to memory, written once by the standard library so nobody writes it
again:

```cpp
std::unique_ptr<Device> device = std::make_unique<Device>(7);
// used like a pointer
device->send();
// deleted automatically, on every path out, including exceptions
```

Prefer `std::make_unique` over `new`. It is one function call rather than an
allocation and a constructor written separately, and it means the word `new` need
not appear in your code at all.

### A member that cannot be copied does not always make the class say so

You would expect a class holding a `std::unique_ptr` to become non copyable
automatically, and for a direct member that is exactly what happens.

A `std::vector<std::unique_ptr<Device>>` member is different, and the difference
is worth knowing. `std::vector` declares a copy constructor no matter what it
holds, and that copy only fails when somebody instantiates it. So the class still
answers yes when asked `std::is_copy_constructible`, while any actual attempt to
copy it produces a page of template errors from deep inside the standard library.

The fix is to write the decision down:

```cpp
DeviceRegistry(const DeviceRegistry&) = delete;
DeviceRegistry& operator=(const DeviceRegistry&) = delete;
DeviceRegistry(DeviceRegistry&&) = default;
DeviceRegistry& operator=(DeviceRegistry&&) = default;
```

Now the type is honest about itself, the error appears on the line that attempts
the copy, and moving still works. This is a small example of a large habit: when
a class owns something, state what copying and moving mean rather than leaving it
to be inferred.

### Ownership becomes visible in the signature

This is the part that changes how you read code:

| Signature | What it says |
|---|---|
| `void use(const Device&)` | I will look at it and not keep it |
| `void use(Device&)` | I will change it and not keep it |
| `void take(std::unique_ptr<Device>)` | Give it to me, it is mine now |
| `std::unique_ptr<Device> make()` | Here is a new one, it is yours |

A reader knows who owns what without reading a single line of the body, and the
compiler enforces every one of those claims. Comments describing ownership stop
being necessary, which is fortunate, because comments describing ownership are
usually out of date.

## Build It

`exercise/solution.hpp` provides a `Device` that counts how many exist, so the
tests can prove nothing is leaked or destroyed twice.

Implement:

- `make_device(int id)` returns a `std::unique_ptr<Device>` holding a new device
  with that id. Use `std::make_unique`.
- `DeviceRegistry::add(std::unique_ptr<Device> device)` takes ownership. Ignore a
  null pointer rather than storing it.
- `DeviceRegistry::take(int id)` finds the device with that id, removes it from
  the registry, and returns it to the caller, transferring ownership out. Returns
  nullptr when there is no such device.
- `DeviceRegistry::find(int id)` returns a plain `Device*` for a caller that
  wants to look without taking, or nullptr. Returning a raw pointer here is
  correct: it says look but do not keep.
- `DeviceRegistry::count()` reports how many are held.
- The registry itself refuses to be copied and allows itself to be moved, stated
  explicitly for the reason given above.

```
rcpp verify 02-04
```

## Use It

Every collection of drivers in this curriculum is this class. Phase 15 holds
sensors this way, phase 17 holds ROS 2 nodes this way, and the Qt control station
in phase 12 holds its panels this way.

When you meet a codebase where every object is a `shared_ptr`, that is usually
not a design. It is a codebase that never decided who owns what, and it has the
leaks to show for it. Lesson 02-06 covers when sharing is genuinely the answer,
which is rarer than it looks.

## What Breaks First

- **The compiler refuses to copy your unique_ptr.** That is the type doing its
  job. You wanted `std::move`, or to return by value. See `E-MEM-0007`.
- **You used a variable after moving from it.** It is empty now. See
  `E-CPP-0010`.
- **The device count never returns to zero.** Something is still holding one, or
  the registry never released what it took. See `E-MEM-0004`.

## Ship It

The registry graduates into `rc::io`. The habit graduates with it: ownership goes
in the signature, so a reader can answer who owns this without reading the body,
and the compiler makes sure the answer stays true.
