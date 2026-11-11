# The Object QML Collected: Who Owns What a C++ Function Hands to JavaScript

> An object with no parent that a function returns to QML belongs to the garbage collector. Say who owns it: a parent, or the engine told that C++ does.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 11-02, 09-04

## The Problem

Lesson 09-04 asked who deletes a QObject. QML adds an owner that C++ code does not expect: a garbage collector. A function that QML calls returns a new object, the JavaScript value goes out of scope, and the object is deleted, while a C++ member still points at it.

## The Concept

### The rule

When an invokable returns a `QObject*` that has no parent, the engine takes ownership: the object is deleted when the JavaScript value that refers to it is unreachable. An object with a parent, or one the engine has been told C++ owns, is left alone, and so is an object that is read from a property.

### What was measured

Four ways of returning a new object, each kept in a local variable that is gone when the handler returns, then `collectGarbage()` and the deferred deletes:

| returned | alive after the call | after the collection | the pointer C++ kept |
|---|---|---|---|
| no parent | 3 | **2** | **gone** |
| a parent | 3 | **3** | valid |
| `CppOwnership`, no parent | 3 | **3** | valid |

(The counts include the object held for a property and one object from the pool, which every case also created.) The first row is the mistake: the member that remembered the object is dangling and nothing says so. See `E-QT-0069`.

### C++ owns it, and so must delete it

The third row survived because the engine was told not to delete it, and no parent will either. The test deleted it by hand: alive went from 3 to **2**. `CppOwnership` without a parent is a leak with a name. See `E-QT-0070`.

### A pool

`keep_alive(object, owner)` sets both, the parent and the ownership, so that there is one owner and it is in C++. `Pool::add` does it and hands back the pointer. Two pooled objects and the one held property: alive **3** after a collection, the pool owning **2**, the ownership `CppOwnership` and the parent the pool. After `pool.clear()`: alive **1** and the pool owning **0**. See `E-QT-0071`.

## Build It

`keep_alive`, and the `Pool` functions `add`, `count` and `clear`, in `exercise/solution.hpp`.

```
rcpp verify 11-04
```

## Use It

`rc/qt/qml_ownership.hpp` joins its module.

```cpp
class Robot : public QObject {
  Q_OBJECT
 public:
  Q_INVOKABLE QObject* new_waypoint() { return pool_.add(new Waypoint()); }
 private:
  rc::qt::Pool pool_;
};
```

**What this does not do.** It decides ownership and does not manage lifetime: a pool that is never cleared grows for as long as the program runs, so something has to say when an object is no longer wanted. It handles `QObject`s, and a value type returned by value is copied into JavaScript and needs none of this. The counts are those of Qt 6.2 to 6.5: a collection is a request and the engine decides when to run it, so a test that waits on one has to collect explicitly, as these do.

## What Breaks First

- **A returned object with no parent.** See `E-QT-0069`.
- **C++ ownership with no one to delete.** See `E-QT-0070`.
- **Two owners, or none.** See `E-QT-0071`.

## Ship It

`rc/qt/qml_ownership.hpp` holds `keep_alive` and `Pool`.

The harness of the last lesson of this phase hands QML nothing it does not own.
