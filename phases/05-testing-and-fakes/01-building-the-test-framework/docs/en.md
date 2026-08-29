# Building the Test Framework You Have Been Using

> You have trusted this thing for twenty lessons. Today you find out it is about a hundred and fifty lines, and then you write it.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 03-02

## The Problem

Every lesson so far has opened with the same incantation:

```cpp
RC_TEST("a stopped robot stays where it is") {
  RC_CHECK_NEAR(next.x, 1.0, 1e-9);
}
```

You never wrote a `main`. You never registered anything. Somehow every one of
those blocks ran, in order, and reported itself.

That is the last piece of magic left in this curriculum, and magic in your tools
is a liability. A framework you do not understand is one you cannot debug when it
misleads you, and this one has already misled you once: lesson 09-01 shipped a
test that segfaulted rather than failing, because the framework did not stop a
test after a required check had failed.

So today you build it.

## The Concept

### Something has to run before main

A test file contains no call to anything. The tests must therefore register
themselves before `main` starts, and C++ has exactly one mechanism for that: the
constructor of an object at file scope runs before `main` is entered.

```cpp
struct Registrar {
  Registrar(const char* name, void (*fn)()) { registry().push_back({name, fn}); }
};

static const Registrar registrar_7("a stopped robot stays where it is", &test_fn_7);
```

That object exists only for the side effect of its constructor. When the program
starts, every such object in every translation unit is constructed, each one
appending to a list, and by the time `main` runs the list holds every test.

This pattern is not confined to test frameworks. It is how plugins register with
a factory, how serialisers register a type, and how a robotics framework
discovers the drivers it was linked against without naming any of them.

### The list must be a function, not a variable

This is the subtle part, and getting it wrong produces a crash that appears on
one compiler and not another.

If the registry were a plain global variable, then two file scope objects in
different translation units would have no defined order between them: a
registrar might run before the vector it appends to has been constructed.

Wrapping it in a function fixes it, because a static local is constructed the
first time control passes through its declaration, which is necessarily before
any use:

```cpp
std::vector<Case>& registry() {
  static std::vector<Case> cases;   // built on first call, whenever that is
  return cases;
}
```

That is the standard cure for the static initialisation order problem, and it
costs one pair of brackets.

### What a macro actually does

`RC_TEST` is a macro, which means the preprocessor pastes text before the
compiler sees anything. Given:

```cpp
#define MINI_TEST(name)                                        \
  static void mini_fn_##__LINE__();                            \
  static const mini::Registrar mini_reg_##__LINE__(name, &mini_fn_##__LINE__); \
  static void mini_fn_##__LINE__()
```

writing `MINI_TEST("adds") { ... }` expands into a function declaration, a
registrar object, and the start of a function definition, and the block you
wrote becomes that function's body. The trick is that the macro ends without a
brace, so your `{ ... }` completes it.

`##` pastes tokens together, and `__LINE__` supplies a different number for each
test in the file so the names do not collide. Pasting `__LINE__` needs two levels
of macro to expand the number rather than paste the literal text `__LINE__`,
which is a classic preprocessor wart and is why `RC_TEST_CONCAT` exists in the
real header.

`#expr` in `RC_CHECK` turns the expression into a string, which is how a failure
can print the code that failed rather than only the values.

### Why a check and a require are different

`RC_CHECK` records a failure and continues, which is useful because one run can
report several problems.

`RC_REQUIRE` records a failure and stops the test, and it exists because of a
real bug in this repository: a test checked that a vector had one element, then
read element zero. When the learner's code produced an empty vector, the check
recorded a failure and execution continued into the read, which crashed. The
learner saw a segmentation fault rather than a sentence naming the problem.

The rule that came out of it: **any check whose failure makes the next line
meaningless or dangerous must be a require.**

## Build It

`exercise/solution.hpp` contains a small framework named `mini`. The `MINI_TEST`
macro is written for you, because it is worth reading closely. Implement:

- `mini::registry()` and `mini::failures()`, both static locals inside functions
  for the reason above.
- `mini::Registrar`, whose constructor appends to the registry.
- `mini::fail(file, line, message)`, recording a failure against the running test.
- `mini::run_all()`, running every registered case and returning how many passed
  and how many failed. It must be callable more than once and give the same
  answer, which means clearing the recorded failures at the start.
- The `MINI_CHECK` macro, which records a failure with the text of the expression
  when it is false.

```
rcpp verify 05-01
```

The tests are written in `rc_test`, the real framework, and they test yours. A
framework tested by a framework is not circular here, because they are two
separate pieces of code and only one of them is under test.

## Use It

Catch2 and GoogleTest are this, plus twenty years of everything else: parameter
generation, mocking, test discovery, sharding, XML output for continuous
integration, and death tests that run in a forked process so a crash cannot take
the run with it.

This curriculum uses its own because it can be read in a sitting and vendored
with no package manager, and because reading it is this lesson. On a real project
that is not a reason to avoid Catch2, and you should reach for it.

The registration pattern, though, you will use directly. Any time a system needs
to discover things it was not told about at compile time, this is the mechanism.

## What Breaks First

- **No tests run, or the program crashes before main.** The registry is a plain
  global rather than a static local inside a function, so the order between
  translation units is undefined. See `E-CPP-0014`.
- **Two tests on different lines collide.** The token pasting expanded
  `__LINE__` as text rather than as its value, which needs two levels of macro.
  See `E-CPP-0015`.
- **A failing test crashes instead of reporting.** A check whose failure makes
  the following line invalid should have been a require. See `E-MEM-0010`.

## Ship It

Nothing in this lesson graduates into `librc`, because `rc/test/rc_test.hpp` is
already there and you have just rewritten it. What graduates is that the tool is
no longer magic: when it next misleads you, you can open it and read it.
