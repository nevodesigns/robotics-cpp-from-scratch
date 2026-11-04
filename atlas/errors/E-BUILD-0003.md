id: E-BUILD-0003
title: missing binary operator before token "("
match: missing binary operator before token
platforms: linux, windows
teaches: 04-07-the-tool-that-watches-it-run
---

## Symptom

```
error: missing binary operator before token "("
   25 |     (defined(__has_feature) && __has_feature(address_sanitizer))
      |                                             ^
```

A feature test that looks carefully guarded fails to compile, and it fails on
one compiler while the other accepts it, which makes it look like a compiler
bug.

## Cause

```cpp
#if defined(__has_feature) && __has_feature(address_sanitizer)   // wrong
```

The `&&` does not protect the right hand side. The preprocessor parses the
**whole** expression before evaluating any of it, so `__has_feature(...)` has
to be syntactically valid even on a compiler that has never heard of it.

The reason this surprises people is that the usual rule works the other way.
An unknown identifier in a `#if` is replaced by `0`, which is why
`#if SOME_MACRO_NOBODY_DEFINED` is legal and false. An unknown identifier
**followed by a parenthesis** is not a macro invocation the preprocessor can
expand, and it is not an identifier it can replace with zero either. It is a
syntax error.

So this compiles on clang, which has `__has_feature`, and fails on gcc, which
does not.

The same trap waits with `__has_include`, `__has_builtin`,
`__has_cpp_attribute` and `__builtin_available`.

## Fix

Nest the tests, so the one asking whether the facility exists is resolved
before the one using it is parsed.

```cpp
#define RC_EXPECT_ASAN 0
#if defined(__has_feature)
#  if __has_feature(address_sanitizer)
#    undef RC_EXPECT_ASAN
#    define RC_EXPECT_ASAN 1
#  endif
#endif
#if defined(__SANITIZE_ADDRESS__)
#  undef RC_EXPECT_ASAN
#  define RC_EXPECT_ASAN 1
#endif
```

Verbose, and there is no shorter correct form in C++17.

Two notes that save a second attempt.

**`__has_include` is the exception.** It is required by C++17, so every
conforming compiler has it and it may be used directly without a guard. The
others are extensions and need the nesting.

**Both spellings are usually needed.** Measured for lesson 04-07 across eight
configurations: gcc 11 defines `__SANITIZE_ADDRESS__` and has no
`__has_feature` at all, while clang 14 has `__has_feature(address_sanitizer)`
and does not define `__SANITIZE_ADDRESS__`. A detection written for one
compiler silently answers "no" on the other, and answering "no" is exactly the
failure mode that goes unnoticed, which is what `E-DEBUG-0010` is about.
