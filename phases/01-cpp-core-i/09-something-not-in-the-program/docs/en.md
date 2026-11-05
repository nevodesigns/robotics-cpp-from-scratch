# Something Not In the Program: Undefined Behaviour, and Checking Before Instead of After

> The same loop, same compiler, same source. 31 iterations at `-O0`, and at
> `-O2` it does not stop.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 01-07

## The Problem

Lesson 01-07 ended on a careful sentence that was easy to skip past: the
unsigned countdown is wrong, and **it is not undefined behaviour**, which is
why no sanitiser reports it.

This lesson is about the other kind, and it is the most important idea in C++
that most courses never state plainly.

Up to now, every mistake you have made produced a wrong answer. A loop ran too
many times, a number came back zero, a tolerance was unsatisfiable. All
annoying, all findable, because the program did what it said and what it said
was wrong.

Undefined behaviour is not that. It is the case where the program does
something that is not in it.

## The Concept

### Defined and wrong, against undefined

Unsigned arithmetic wraps, and wrapping is **defined**. The standard specifies
the result exactly: modulo two to the width of the type. `UINT_MAX + 1u` is 0,
and `0u - 1u` is 4294967295, on every machine and every compiler, every time.
That is a wrong answer for what you meant, and it is a fact you can write down
in a test and assert.

Signed overflow is **undefined**. The standard places no requirement on the
program at all. Not "the result is unspecified", not "it wraps on most
machines". No requirement.

The practical consequence is not that you get a strange number. It is that
**the compiler is entitled to assume it never happens**, and to optimise on
that assumption.

### What that actually costs, measured

A loop doubling an int from 1, continuing while `i > 0`, with a counter bounded
at 200 so the result could be reported at all:

| build | doublings counted |
|---|---|
| g++ -O0 | 31 |
| g++ -O1 | 31 |
| g++ -O2 | **201, the bound** |
| clang -O2 | **201, the bound** |

At `-O0` the loop ends after 31 doublings, when the value rolls negative. At
`-O2` both compilers removed the exit test entirely, because an int doubling
from 1 is positive unless it overflows, and overflow is assumed not to occur.
**Without the bound, that loop does not end.**

Nothing in the source changed between those rows.

And separately, a function returning `x + 1 > x`, called with `INT_MAX`,
returned **true** on both compilers at every optimisation level including
`-O0`. That is wrong for the only value it was asked about.

This is not a compiler bug, and reporting it would be rejected. It is the
bargain the language makes: it produces fast code, and in exchange you promise
never to overflow.

### Which is why you cannot check afterwards

Here is the guard everybody writes first:

```cpp
int total = a + b;
if (total < a) { return too_big(); }   // never taken
```

Two things are wrong with it. The overflow has already happened by the time the
comparison runs, so there is nothing left to inspect. And if `b` is positive
then `a + b` is greater than `a` unless it overflowed, the compiler may assume
it did not, so the condition is provably false and the branch is dead code.
Dead code is removed.

The question has an answer only while both operands are still ordinary numbers.
So it has to be asked first.

### Rearranging so the arithmetic stays in range

```cpp
inline bool add_overflows(int a, int b) {
  if (b > 0) return a > INT_MAX - b;
  if (b < 0) return a < INT_MIN - b;
  return false;
}
```

`a > INT_MAX - b` asks the same question as `a + b > INT_MAX`, and unlike it,
cannot itself overflow. That is the whole technique.

Three places it gets subtle, and each is a line in the artifact:

**Subtraction is not addition of the negation.** `add_overflows(a, -b)` is
undefined when `b` is `INT_MIN`, because `INT_MIN` has no positive counterpart.
Two's complement holds one more negative number than positive. This is also why
`std::abs(INT_MIN)` is undefined, which surprises people who think of `abs` as a
function that cannot fail.

**Multiplication needs division, and division has two landmines.** `b` may be
zero. And `INT_MIN / -1` overflows, which would be undefined behaviour inside
the function written to prevent undefined behaviour. Both cases go before the
general one.

**The guard must never perform the operation.** The suite runs its whole
boundary set under the undefined behaviour sanitizer, and a clean run is the
assertion.

### Where the compiler does have to object

There is exactly one place the standard requires a diagnosis: undefined
behaviour in a constant expression makes the program ill formed.

| constant expression | gcc | clang |
|---|---|---|
| `constexpr int x = INT_MAX + 1;` | refused | refused |
| `constexpr int x = 1 << 32;` | refused | refused |
| `constexpr int x = 1 / 0;` | refused | refused |
| `constexpr unsigned u = UINT_MAX + 1u;` | accepted | accepted |
| `constexpr unsigned u = 1u << 31;` | accepted | accepted |

Same arithmetic, two different rules, and the difference is exactly defined
against undefined. The three refusals are in this lesson's `checks/` directory
as code that must not compile.

Note what that table does not promise. These are refused because the compiler
can see the values. The moment either operand arrives from a sensor at runtime,
nothing is refused and nothing is said.

### What does find it at runtime

The undefined behaviour sanitizer, which this curriculum runs on every push:

```
runtime error: signed integer overflow: 1073741824 * 2 cannot be
represented in type 'int'
```

The operation, the file and the line. It only finds what your tests actually
execute, which is an argument for tests that reach the boundaries rather than
the middle.

### A note on testing near the limits

The lesson's first test sweeps every pair in four windows and checks each
prediction against `long long` arithmetic. Here is what it prints:

| sweep | predictions | said yes | wrong |
|---|---|---|---|
| around zero | 4327203 | **0** | 0 |
| against the top | 4327203 | 1799398 | 0 |
| against the bottom | 4327203 | 1799399 | 0 |
| where products turn over | 1083603 | 180289 | 0 |

Look at the first row. Four and a third million pairs, and not one of them
overflows, so every correct answer in that window is "no". A function that
always answered no would score zero wrong on it.

That window was the whole test when this lesson was first written, and the
exercise stub passed it. Testing near the middle of a range proves almost
nothing about code whose entire job is the edges.

## Build It

Implement `add_overflows`, `subtract_overflows`, `negate_overflows` and
`multiply_overflows` in `exercise/solution.hpp`.

```
rcpp verify 01-09
```

The suite sweeps fourteen million predictions against wider arithmetic, runs
the whole boundary set under the sanitizers to prove the checks never overflow
themselves, asserts the exact values unsigned arithmetic wraps to, and tries to
compile three constants the compiler must refuse.

## Use It

**Ask before, never after.** `rc::core::add_overflows(a, b)` takes the
operands, not a result, because after the fact there is no result.

**Use a wider type when you can.** If the values fit in `std::int64_t`, doing
the arithmetic there and range checking once is simpler than four predicates,
and it is frequently the right answer.

**Never switch a type to silence a wrap.** Changing `unsigned` to `int` to stop
a wrap replaces a defined wrong answer with undefined behaviour, which is
strictly worse because it cannot be tested for.

**Assert what unsigned does.** It is specified, so write it down. A defined
behaviour pinned in a test is a documented property rather than a bug waiting.

**Run the sanitizer lane, and aim your tests at the edges.** It reports only
what runs.

**Do not report it as a compiler bug.** It is not one.

## What Breaks First

- **A check for overflow that runs after the overflow.** See `E-CPP-0031`.
- **Unsigned wrapping and signed overflow confused.** See `E-CPP-0032`.
- **A guard that overflows inside the guard.** See `E-CPP-0033`.

## Ship It

`add_overflows`, `subtract_overflows`, `negate_overflows` and
`multiply_overflows` join `rc::core` beside `ssize` and `nearly_equal`. Phase
01 now has the three arithmetic traps covered: unsigned sizes that wrap,
floating point that is never exact, and signed integers that must not be
allowed to overflow at all.
