id: E-TEST-0005
title: A test suite written by the person who wrote the code
match: nine careful examples, and what they miss
platforms: linux, windows
teaches: 05-04-inputs-you-did-not-think-of
---

## Symptom

Good coverage, careful cases, a green suite, and a bug in production on an input
nobody argued about. In hindsight the input is not exotic. It is just longer, or
emptier, or more repetitive than anything anybody typed.

## Cause

Examples come from the same head that wrote the code, and that head has already
decided what the interesting cases are.

A parser for unsigned decimals with no overflow check, against nine cases a
careful person writes:

```
"0"  "1"  "42"  "999"  "1000000"  ""  "x"  "12x"  "007"
```

None of them fail. Empty, non-numeric, trailing rubbish, leading zeros, a large
round number: every category somebody would think to list, and the parser passes
all of them while silently wrapping around on any number of twenty digits or
more.

The value that catches it is `18446744073709551616`. Nothing clever about it. It
is simply longer than anybody types into a test by hand.

## Fix

State a property and let the machine choose the inputs.

```cpp
// Not: parse("42") == 42
// But: if the parser accepted the text, the value prints back as the text.
bool reads_what_was_written(const std::string& text) {
  const auto parsed = parse(text);
  return !parsed.ok || std::to_string(parsed.value) == without_leading_zeros(text);
}
```

Generated inputs found that fault in **1.6 values on average**, across twenty
seeds, where nine examples found it in none.

Choosing the property is most of the work, and it is worth choosing a strong
one. A weaker property here, that the value survives being printed and read
again, is true even when the parser has wrapped, because the wrapped value is a
perfectly good number and round trips as itself. "It read the number you wrote"
is the statement that has content.

Keep the examples as well. They document intent, they run in microseconds, and
they are what a reader of the test file learns the interface from. What they are
not is a search.

Two things that go with this:

**The generator decides what is reachable**, which is `E-TEST-0006`.

**Prove the property can fail.** Break the code deliberately and watch the
property report it, exactly as lesson 05-03 does for an ordinary test. A property
that passes on the broken code and the fixed code is measuring neither, which is
`E-TEST-0007`.
