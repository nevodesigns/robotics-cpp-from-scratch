# The Component That Was Not Ready Yet: Asynchronous Loading, a Status Nobody Checked and a Wait That Gives Up

> A component loaded in the background is Loading, and create() on it gives null. Let the loop run until it is Ready, an Error, or too late.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 11-03, 11-01, 09-13

## The Problem

Lesson 11-01 loaded a file and read its errors straight away, which works because a local file loads at once. A component loaded in the background, which is what a big interface or one over a network does, is not ready when the constructor returns, and every line written as if it were is a null pointer in waiting.

## The Concept

### Loading

Two components on a local file in Qt 6.2: loaded by default it was **Ready** when the constructor returned; loaded with `QQmlComponent::Asynchronous` it was **Loading**, and `create()` on it gave **null**. After `wait_until_ready` its status was Ready and `create()` gave the object. See `E-QT-0078`.

The file has to be a first load. An engine keeps what it has compiled by its URL, so a second component on the same file is ready at once, and a test that loads the file twice to compare the two modes compares nothing.

### Error is not Ready

An asynchronous file with a syntax error became **Error**: `create_when_ready` returned null, and the reason was `.../main.qml:2:15: Expected token ';'`, with the file and position. A file that does not exist was an Error after **0** ms and not a hang. A good component made its object with a width of 42 and an empty reason. See `E-QT-0079`.

### Nothing to wait for

A component that is Ready and one with no source (status Null) returned at once, in **0** ms: neither waited for the 5000 ms they were allowed. A wait with a time of **0** on a component that was Loading was a Timeout after **0** ms, and a later wait of 5 seconds got it to Ready. A wait that is not bounded by its time is a test that hangs. See `E-QT-0080`.

## Build It

`wait_until_ready` and `create_when_ready` in `exercise/solution.hpp`.

```
rcpp verify 11-07
```

## Use It

`rc/qt/qml_loader.hpp` joins its module.

```cpp
QQmlComponent component(&engine, url, QQmlComponent::Asynchronous);
QString why;
QObject* root = rc::qt::create_when_ready(component, 5000, &why);
if (root == nullptr) { /* say why, with the file and the line */ }
```

**What this does not do.** It waits on the status of one component, and not on what the component loads: a `Loader` or an `Image` inside it has its own status, and the object is ready for `create()` when its component is, not when its pictures are. It runs a nested event loop, which delivers every other event for the length of the wait, and a test that wants nothing else to happen should not use it. A timeout is a choice: 5 seconds is generous for a file on a disk and not for one over a slow link. And the object it returns has to be deleted by the caller.

## What Breaks First

- **create() on a component that is still Loading.** See `E-QT-0078`.
- **A failed load treated as ready.** See `E-QT-0079`.
- **A wait with no end.** See `E-QT-0080`.

## Ship It

`rc/qt/qml_loader.hpp` holds `LoadResult`, `wait_until_ready` and `create_when_ready`.

The harness of the last lesson of this phase loads every file through them.
