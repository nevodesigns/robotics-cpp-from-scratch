# The Destructor That Did Not Run: Inheritance, Virtual Dispatch and the Two Lines Every Interface Needs

> One of the three bugs in this lesson cannot be demonstrated by a test,
> because demonstrating it is undefined behaviour and the address sanitizer
> stops the program. That is not a gap in the lesson. It is the reason the
> answer is a `static_assert` rather than a test.

**Type:** Build
**Time:** about 125 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 02-02, 02-03, 03-01, 03-05

## The Problem

Lesson 03-05 handed you an interface and asked you to implement it, because an
injectable clock was the only way to give `dt` a provenance. You have used
three of them since without being told how they work:

```cpp
class Clock {
 public:
  virtual ~Clock() = default;          // deleted through a base pointer, so this matters
  virtual Nanoseconds now() const = 0;
};
```

That comment is in `rc/core/clock.hpp` and it is not decoration. This lesson is
about what the machinery does, what it costs, and the three ways it goes
silently wrong.

Silently is the operative word. Of the three, one has a compiler warning in
half of its cases, and the other two have no diagnostic at all on any compiler
this curriculum claims.

## The Concept

### Dispatch, in one paragraph

A `virtual` function is chosen by the type of the **object**, not the type of
the **variable**. A `Sensor&` that refers to a `Thermometer` calls
`Thermometer::read`. That is the entire point: it lets code that was written
once work with things that did not exist when it was written.

The suite holds two thermometers behind two `Sensor` pointers and reads 4 and
-18 out of them. Nothing in the loop knows what a thermometer is.

### Trap one: the destructor that did not run

```cpp
Sensor* s = new Thermometer(21.5);
delete s;                    // runs ~Sensor, never ~Thermometer
```

`delete` through a base pointer calls the base destructor. If it is not
`virtual`, the derived one does not run and everything the derived class owns
is abandoned.

Measured with replaced global `operator new` and `operator delete`, on a
derived class owning one heap string:

| base destructor | allocations still live after `delete` |
| --- | --- |
| not virtual | 2 |
| `virtual` | 0 |

Two, because the string object and the buffer it owns are both lost.

Those numbers come from a standalone program, not from this lesson's suite,
and that is worth being explicit about. The suite cannot produce them. The
delete is undefined behaviour, and running it under this repository's
sanitizer lane gives:

```
ERROR: AddressSanitizer: new-delete-type-mismatch
```

which is the right thing for the sanitizer to do and useless as a
measurement. A test that performs undefined behaviour is not a test.

The warnings do not cover you either:

| the base | gcc | clang |
| --- | --- | --- |
| has some other virtual function | warns | warns |
| has no virtual functions at all | silent | silent |

The second row is the common shape for the types this bites: a handle, a
resource wrapper, a little base class somebody added to share two fields.
Nothing marks them as polymorphic, so nothing warns.

**So do not test for it. Require it.**

```cpp
#include <type_traits>
static_assert(std::has_virtual_destructor<Sensor>::value,
              "deleted through a base pointer, so this is required");
```

One line per interface, next to the declaration, checked at build time for
free. `checks/a_base_without_a_virtual_destructor.cpp` is that line failing on
purpose, and the suite asserts the same fact about `Sensor`.

### Trap two: the derived part that was thrown away

```cpp
double read_a_copy(Sensor s) { return s.read(); }   // by value
```

The suite reads the same object two ways:

| how it was passed | what `read()` returned |
| --- | --- |
| `const LooseSensor&` | 21.5 |
| `LooseSensor` by value | 0 |

Nothing failed. The copy really is a `LooseSensor`, so dispatch correctly
selected the base implementation, and `0` is the honest answer to the question
the parameter asked.

The sizes say why, and the suite prints them:

```
    sizeof(LooseSensor)        8
    sizeof(LooseThermometer)   16
```

The copy is eight bytes. The object was sixteen. There is nowhere to put the
rest. This is **slicing**, it is completely legal, and neither gcc 11 nor
clang 14 warns about it at any optimisation level, including clang under
`-Weverything`.

The fix is not vigilance. Delete the copy in the base:

```cpp
class Sensor {
 public:
  virtual ~Sensor() = default;
  Sensor(const Sensor&) = delete;
  Sensor& operator=(const Sensor&) = delete;
  virtual double read() const = 0;
 protected:
  Sensor() = default;         // an interface is not instantiated on its own
};
```

Nothing is lost, because copying an interface has no sensible meaning: the only
reason the type is an interface is that what sits behind it varies. The by
value parameter is now a compile error pointing at the call, and
`checks/slicing_a_sensor.cpp` is that error.

This is also why a container of polymorphic things is
`std::vector<std::unique_ptr<Sensor>>` and never `std::vector<Sensor>`.

### Trap three: an override that overrides nothing

```cpp
class Pyrometer : public Sensor {
 public:
  double read() override { return 900.0; }   // the base declares read() const
};
```

One word different, and without `override` that compiles into a new,
non virtual member that hides the base one. Calls through a `Sensor&` keep
reaching the base version and calls through a `Pyrometer&` reach this one, so
the same object answers differently in two functions of the same program.

The near misses, in rough order of frequency:

| base | derived | what happened |
| --- | --- | --- |
| `double read() const` | `double read()` | `const` dropped |
| `void set(const Pose&)` | `void set(Pose)` | parameter by value |
| `double read() const` | `float read() const` | return type differs |
| `virtual ~Sensor()` | `~Thermometer()` | the destructor, which also needs it |

`override` costs one word, generates no code, and turns every one of them into
an error at the declaration. Put it on everything you mean to override,
destructors included. Drop `virtual` from the derived declarations once you
do: the base says `virtual`, the derived says `override`, and each word then
means one thing in one place.

### What it costs

```
    sizeof(PlainReading)       16
    sizeof(PolymorphicReading) 24
    sizeof(void*)              8
```

Two structs with identical data. One virtual function adds one hidden pointer,
**per object**, not per class. A `std::vector` of a million readings pays eight
megabytes for a function it may never call.

That is the whole reason `rc::math::Pose` has no virtual anything, and neither
does any other value type in `librc`. The three that do are
`rc::core::Clock`, `rc::io::BytePort` and `rc::io::SensorSource`, and all three
are interfaces in the strict sense: things you swap.

**An interface is for things you swap. A value is not one.**

### Why the order is derived first

Construction runs base first, destruction runs derived first. It is the only
order that works: a derived destructor may still need the base parts it is
about to give up, and a base constructor cannot depend on a derived class that
does not exist yet.

The suite watches this rather than asserting it, by counting in both
destructors and checking the counts after a `unique_ptr<Sensor>` goes out of
scope.

## Build It

Four pieces.

Count the destruction. Increment a counter in `~Sensor` and another in
`~Thermometer`, so the suite can see which destructors ran instead of trusting
that they did.

Delete the copy constructor and copy assignment in `Sensor`, and make its
default constructor protected.

Change `read_by_value` to take its parameter by value. The body does not
change. Only the parameter does, and the answer changes with it.

The `virtual` on `~Sensor` is given to you rather than left as an exercise,
and that is the lesson rather than a shortcut: an exercise that asked you to
leave it out would be asking you to ship undefined behaviour.

## Use It

```cpp
#include <rc/core/clock.hpp>

class FakeClock : public rc::core::Clock {
 public:
  ~FakeClock() override = default;
  rc::core::Nanoseconds now() const override { return now_; }
  void advance(rc::core::Nanoseconds by) { now_ += by; }
 private:
  rc::core::Nanoseconds now_{0};
};
```

Every interface you write from here gets the same four things: a virtual
destructor, a `static_assert` that it has one, deleted copy operations, and
`override` on every function that replaces a base one.

Held by reference when you are only reading it during the call, and by
`std::unique_ptr` when you own it.

## What Breaks First

`E-CPP-0052` is the destructor that did not run, with the measured leak, the
sanitizer's response and the table of when each compiler warns.

`E-CPP-0053` is the derived part that was thrown away, with the two readings
and the sizes that explain them.

`E-CPP-0054` is the override that overrides nothing, with the full table of
near misses.

## Ship It

This lesson graduates no header, and that is deliberate. What it ships is two
lines you put in every interface you write for the rest of this curriculum,
and a third in the test beside it:

```cpp
virtual ~Sensor() = default;
Sensor(const Sensor&) = delete;

static_assert(std::has_virtual_destructor<Sensor>::value, "");
```

The three interfaces already in `librc` were written that way before this
lesson existed. Now you know why.
