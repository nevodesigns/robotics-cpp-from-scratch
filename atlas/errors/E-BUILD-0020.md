id: E-BUILD-0020
title: A private field added to a class that every file includes
match: adding a field changes the size of an open class and not of one behind a pointer
match: expected sizeof
platforms: linux, windows
teaches: 04-14-the-header-that-changed
---

## Symptom

A field is added to a class that lives in a header. Every file that includes the header is built again, which takes twenty minutes. Worse, a library built before the change is linked with code built after it, and the program corrupts memory in a place nowhere near the change.

## Cause

A caller that holds an object of the class reserves `sizeof` bytes for it, worked out when it was compiled. A new field changes `sizeof`, so anything built against the old header is reserving too little. The compiler cannot see across a library boundary to notice.

Measured: the same filter with its data in the class was **24** bytes in version 1 and **104** in version 2, once a history of ten readings was added. With its data behind a pointer the class was **8** bytes in both. A `Smoother` is 8 bytes, the size of one pointer.

## Fix

Keep the data in a type that the header only names, and own it through a pointer:

```cpp
struct Impl;
std::unique_ptr<Impl> impl_;
```

The fields live where `Impl` is defined, in one source file.
