# The Word That Means Three Things: static in a Function, static in a Class, and the Counter That Forgot Copies

> A counter of live objects went down by one for every copy made, because the
> copy was new and the constructor never saw it.

**Type:** Build
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 02-03, 02-05, 02-11

## The Problem

Lesson 02-03 contains this, in code you were asked to read:

```cpp
static int next_id = 1;
const int id = next_id++;
```

It hands out a different number each time it runs, and nothing said how. The word
`static` is in a dozen lessons after that, and it means a different thing in each
of three places. Lesson 04-04 covers one of them, the file-scope one, where it
decides which files can see a name. This covers the other two, because they are
the ones that surprise.

## The Concept

### Inside a function, it outlives the call

An ordinary local is built when control reaches it and gone when the function
returns. A `static` local is built the first time control reaches it, **once**,
and keeps its value from then on.

```cpp
inline int next_id() {
  static int next = 1;
  return next++;
}
```

```
  three calls to next_id(): 1 2 3
```

The function has a memory. That is the use, and it is also the cost: what it
returns depends on every call before it, so the same call with the same arguments
gives a different answer, and a test cannot start it from zero. Lesson 05-06
measures what that does to a suite, from the other side. The suite here checks
that each call is one more than the last, and not that the first is 1, for the
same reason.

### When it is built

```
  sensors built:
    before shared_sensor_id() was ever called   0
    after the first call                        1
    after ten more calls                        1
    ten calls to a version without static       10
```

A static local is built **when control first reaches it**, and not when the
program starts. Until the first call there is nothing, which is also why a static
local inside a function is the safe way to have a shared object that depends on
other things being ready: it is built on demand.

Experiment, method stated: a global object and two static locals, reached in the
order B, A, each printing when it was built and destroyed. Run once on gcc 11 and
clang 14, with identical results:

```
    built     the global           before main began
    built     static local B       when second() was first called
    built     static local A       when first() was first called
    destroyed static local A       after main returned
    destroyed static local B
    destroyed the global
```

Built in the order reached. Destroyed after `main`, in the **reverse** of the order
they were built. A static local whose destructor uses another is safe only if the
other was built first, and the order it was reached in decides that.

`E-CPP-0072`.

### Inside a class, it belongs to the class

```cpp
class Wheel {
 public:
  static int count() { return live_; }
 private:
  double radius_;
  inline static int live_ = 0;
};
```

A static data member is **one per class**, not one per object, and so a static
member function can be called with no object at all, `Wheel::count()`.

```
  sizeof(Wheel), the count kept in the class      8
  sizeof of the same, with the count in each one  16
```

The class with the static count is the size of its one `double`. The count is not
part of the object. It lives with the class, wherever the program keeps such
things, and every `Wheel` sees the same one.

That has a consequence. A static member function has no object, so it has no
`this`, and a member that is part of an object has no meaning in it:

```
error: invalid use of member 'Wheel::radius_' in static member function
```

Whose radius? `checks/a_static_function_with_no_object.cpp` is that refusal.

`E-CPP-0071`.

### Declaring it is not making it

Inside the class, `static int live_;` is a **declaration**. A class that has only
that, and a function that reads the variable, compiles, and then fails to link:

```
undefined reference to `Wheel::live_'
```

It is lesson 00-02's error, arriving for a variable. Before C++17 the cure was
one definition in exactly one `.cpp` file, `int Wheel::live_ = 0;`, and putting it
in a header gave the duplicate symbol instead. In C++17,

```cpp
inline static int live_ = 0;
```

declares and defines it in one line, with the same `inline` as lesson 00-02: one
definition, however many files include the header.

`E-LNK-0006`.

### A counter has to see every way an object is made

```cpp
explicit Wheel(double radius) : radius_(radius) { ++live_; }
Wheel(const Wheel& other)     : radius_(other.radius_) { ++live_; }
~Wheel()                       { --live_; }
```

Leave out the middle line and nothing complains. A copy is a **new** object that
the ordinary constructor never ran for, the compiler's own copy constructor does
not touch your counter, and the destructor will still count it out. The count
drifts down by one for every copy made, and reaches zero while a wheel still
exists.

The suite makes one copy and requires the count to go to 2 and back to 1. If the
class were movable, the move constructor would need the same line, and lesson
02-05 is the list of every way an object can be made.

### The three meanings

| where | what `static` does | lesson |
| --- | --- | --- |
| inside a function | the variable outlives the call, built once, on first use | this one |
| inside a class | one for the whole class, no `this` in a function | this one |
| at file scope | the name is visible to that file only | 04-04 |

All three are state that the function or the class does not own the whole life
of. That is the thing to be careful with.

## Build It

Three pieces.

`next_id` keeps its counter in a static local and returns it, post-incremented.

`shared_sensor_id` makes the sensor a static local. The stub builds one on every
call, which looks right and gives a different id each time; the suite counts how
many were built.

`Wheel` gets a static data member defined where it is declared, a constructor and
destructor that adjust it, a **copy constructor** that adjusts it too, and a
static `count()`.

The stubs keep no count and have no memory, so each of the three pieces fails for
the reason it exists.

## Use It

```cpp
const Wheel front(0.15);
const Wheel rear(0.20);
std::cout << Wheel::count() << " wheels\n";       // 2, with no wheel named
```

Two limits.

**A static local is not a license for hidden state.** Prefer to pass state in, and
use a static when something is genuinely one-per-program and built on demand. This
curriculum's `rc::test` registries are exactly that.

**Initialisation is safe, mutation is not.** Since C++11 the standard guarantees a
static local is built once even if two threads reach it together. It says nothing
about two threads changing `next` at once, which is lesson 07-01.

## What Breaks First

`E-CPP-0071` is the static function that reached for a member of an object.

`E-LNK-0006` is the static member that was declared and never defined, which links
as an undefined reference to a variable.

`E-CPP-0072` is the function that answers differently each time, and the counter
that drifted down by one per copy.

## Ship It

No header graduates from this lesson. Its output is knowing what the word means in
each place it is written.

From here on, a counter of live objects in this curriculum counts copies and moves
as well as the ordinary constructor.
