#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <QtCore/QLocale>
#include <QtCore/QMetaType>
#include <QtCore/QString>
#include <QtCore/QVariant>

#include <cmath>
#include <limits>

// A QVariant converts to almost anything, and says so with ok == true. The conversions are not the ones a person would ask for:
// a double 3.7 becomes the int 4, a double 3e9 becomes -1294967296 and reports ok, minus one becomes the unsigned 4294967295, and the string
// "no" is true. These functions take the same values and refuse each of those, because each changes the value.

// A whole number, or nothing.
//
// Integers of any width are taken if they fit an int. A double is taken only if it is a whole number that fits. A string is read in the plain locale and
// must be a whole number: "3.7" is refused, and so is "1e3", since the way to say a thousand is 1000.
inline int setting_int(const QVariant& v, bool* ok) {
  // TODO: *ok starts false. Look at v.typeId().
  //  - Double or Float: d = v.toDouble(); taken only if d == std::floor(d) and it is within the range of an int; the value is static_cast<int>(d).
  //  - QString: QLocale::c().toInt(v.toString(), &parsed); *ok is parsed. ("3.7" and "1e3" are refused: they are not whole numbers.)
  //  - Int, Short, Char, SChar, UChar and UShort: taken, with v.toInt().
  //  - UInt, ULongLong, ULong: taken only if v.toULongLong() is at most the largest int.
  //  - LongLong, Long: taken only if v.toLongLong() is within the range of an int.
  //  - anything else, including Bool and an empty variant: refused.
  // A refusal returns 0 with *ok false.
  *ok = false;
  (void)v;
  return -1;
}

// True or false, or nothing.
//
// A bool is taken as it is. A string must be one of true, false, yes, no, on, off, 1 or 0, in any case. A number must be exactly 0 or exactly 1. Anything
// else is refused, because "abc" is not true, and neither is "2".
inline bool setting_bool(const QVariant& v, bool* ok) {
  // TODO: *ok starts false. A Bool is taken as it is. A QString, trimmed and lower cased, must be true, yes, on or 1 (true) or false, no, off or 0
  // (false), and anything else is refused. An Int, Double or LongLong must be exactly 0 or exactly 1 (use v.toDouble()). Anything else is refused.
  *ok = false;
  (void)v;
  return true;
}

// A finite number, or nothing: from a number or from text in the plain locale. Not a number and infinity are refused.
inline double setting_double(const QVariant& v, bool* ok) {
  // TODO: *ok starts false. A QString is read with QLocale::c().toDouble(text, &parsed) and refused if it did not parse. A Double, Float, Int or
  // LongLong is v.toDouble(). Anything else is refused. A result that is not finite (std::isfinite) is refused. Otherwise *ok is true.
  *ok = false;
  (void)v;
  return -1.0;
}

#endif  // LESSON_SOLUTION_HPP
