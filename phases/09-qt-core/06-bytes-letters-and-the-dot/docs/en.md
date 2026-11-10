# Bytes, Letters and the Dot: What a QString Length Counts, Where Text Goes Wrong on the Wire, and the Decimal Point a Locale Moves

> A name arrives from a sensor as bytes, goes through Qt as a string, and is written to a file as a number and some text.
> Three different things are called its length, two encodings can read it, and a machine in Germany moves the decimal point.

**Type:** Build
**Time:** about 150 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 09-01, 03-15

## The Problem

A sensor sends a name and a reading: `µs`, 1.5. The program receives bytes, turns them into a `QString`, shows it, logs it and writes the number to a file. Every one of those steps can go wrong without a message: the name can come out with other letters in it, a limit on its length can count the wrong thing, and the number can be written as `1,5` on one machine and `1.5` on another.

Lesson 03-15 wrote numbers down and read them back; lesson 01-05 introduced strings. Qt adds two things of its own to both: a string type that stores 16 bit units, and a notion of locale that is on by default.

## The Concept

### Three lengths

A byte string counts bytes. `QString::length()` counts the 16 bit units it stores. A person counts letters. For ASCII the three agree.

| text | bytes in UTF-8 | `length()` | letters |
|---|---|---|---|
| abc | 3 | 3 | 3 |
| micro sign | 2 | 1 | 1 |
| smiling emoji | 4 | 2 | 1 |

A character outside the first 65536 takes two units, so an emoji has a `length()` of 2. Letters are counted by converting to UCS-4, one number each: `text.toUcs4().size()`. A limit that protects a wire format is in bytes, and one that protects a screen is in letters. See `E-QT-0027`.

### Bytes are not text until an encoding is named

The bytes `C2 B5 73` read as Latin-1 are **3** letters (U+C2, U+B5, s). Read as UTF-8 they are **2**: the micro sign and an s. The euro sign cannot be written in Latin-1 at all: its Latin-1 byte was **63**, a question mark.

`decode_utf8` uses a `QStringDecoder` and asks whether it went wrong. The bytes 61 62 FF 63 gave 4 letters with U+FFFD, the replacement character, third, and `clean` false. Without the question it would be a log with a mark in it and no error.

### A cut character

A whole frame that ends in the middle of a character is an error: a euro sign cut after two of its three bytes was **not clean** from a stateless decoder. A decoder that keeps its state is for a stream: given the same two chunks, one after the other, it returned 2 letters with no error from the first and U+20AC from the second. Use the one that matches whether the bytes are a frame or a stream. See `E-QT-0028`.

### The dot

`QLocale()` is the locale of the application, which starts as the machine's. With German as the application's locale:

| call | result |
|---|---|
| `QLocale().toString(1.5)` | `1,5` |
| `QLocale().toDouble("1.500")` | **1500** |
| `QLocale().toDouble("1.5")` | refused |
| `format_number(1.5, 1)` | `1.5` |
| `parse_number("1.500")` | `1.5` |

The last two use `QLocale::c()`, the plain locale, and did not change when the default locale did. A file or a protocol names the plain locale; the application's own is for a number shown to a person. See `E-QT-0029`.

## Build It

`decode_utf8`, `letters_in`, `parse_number` and `format_number` in `exercise/solution.hpp`. The `Decoded` type and `encode_utf8` are given.

```
rcpp verify 09-06
```

## Use It

`rc/qt/text.hpp` joins `rc::qt`.

```cpp
const rc::qt::Decoded name = rc::qt::decode_utf8(bytes_from_wire);
if (!name.clean) { /* the sender used another encoding, or a frame was cut */ }
```

**What this does not do.** It reads UTF-8 only, and a protocol that says Latin-1 or UTF-16 needs another decoder. Counting code points is still not counting what a person sees: a letter with an accent can be one code point or two (an `e` and a combining mark), and an emoji with a skin tone is several. It does not normalise text, so two strings that look the same can compare
as different. And `QLocale::c()` accepts a grouping comma in the right place (`1,500.5` is 1500.5), which a strict parser might not want.

## What Breaks First

- **A length in the wrong unit.** An emoji is 2, or 4 bytes, or 1. See `E-QT-0027`.
- **Bytes read in an encoding they were not written in.** Other letters, or a replacement character no one reported. See `E-QT-0028`.
- **A number written with the machine's locale.** 1,5 and 1500. See `E-QT-0029`.

## Ship It

`rc/qt/text.hpp` holds `Decoded`, `decode_utf8`, `encode_utf8`, `letters_in`, `parse_number` and `format_number`.

Lesson 03-15 made a number read back as itself. This keeps that true on a machine whose locale says otherwise.
