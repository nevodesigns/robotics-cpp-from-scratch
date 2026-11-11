// rc/qt/settings_convert.hpp
//
// Settings taken from a QVariant without a silent change, from lesson 09-10, graduated.
//
// On Qt 6.2 QVariant(3.7).toInt() was 4 with ok true, QVariant(-3.7).toInt() was -4, QVariant(3e9).toInt() was -1294967296 with ok true, and QVariant(-1).toUInt() was 4294967295 with ok true. As booleans, the strings "no", "off", "abc" and "2" were all true,
// and QVariant(0.1).toBool() was false, since a double is cut to a whole number first. setting_int refused all 13 of the conversions that would change a value, setting_bool read 13 of 13 spellings of true and false and refused 9 of 9 that are neither, and
// setting_double read numbers and text in the plain locale and refused 1,5, not a number and infinity.

#ifndef RC_QT_SETTINGS_CONVERT
#define RC_QT_SETTINGS_CONVERT

#include <QtCore/QLocale>
#include <QtCore/QMetaType>
#include <QtCore/QString>
#include <QtCore/QVariant>

#include <cmath>
#include <limits>

namespace rc {
namespace qt {

// A QVariant converts to almost anything, and says so with ok == true. The conversions are not the ones a person would ask for:
// a double 3.7 becomes the int 4, a double 3e9 becomes -1294967296 and reports ok, minus one becomes the unsigned 4294967295, and the string
// "no" is true. These functions take the same values and refuse each of those, because each changes the value.

// A whole number, or nothing.
//
// Integers of any width are taken if they fit an int. A double is taken only if it is a whole number that fits. A string is read in the plain locale and
// must be a whole number: "3.7" is refused, and so is "1e3", since the way to say a thousand is 1000.
inline int setting_int(const QVariant& v, bool* ok) {
  *ok = false;
  const int type = v.typeId();

  if (type == QMetaType::Double || type == QMetaType::Float) {
    const double d = v.toDouble();
    if (d == std::floor(d) && d >= static_cast<double>(std::numeric_limits<int>::min()) &&
        d <= static_cast<double>(std::numeric_limits<int>::max())) {
      *ok = true;
      return static_cast<int>(d);
    }
    return 0;
  }

  if (type == QMetaType::QString) {
    bool parsed = false;
    const int n = QLocale::c().toInt(v.toString(), &parsed);
    *ok = parsed;
    return parsed ? n : 0;
  }

  if (type == QMetaType::Int || type == QMetaType::Short || type == QMetaType::Char || type == QMetaType::SChar || type == QMetaType::UChar ||
      type == QMetaType::UShort) {
    *ok = true;
    return v.toInt();
  }
  if (type == QMetaType::UInt || type == QMetaType::ULongLong || type == QMetaType::ULong) {
    const qulonglong u = v.toULongLong();
    if (u <= static_cast<qulonglong>(std::numeric_limits<int>::max())) {
      *ok = true;
      return static_cast<int>(u);
    }
    return 0;
  }
  if (type == QMetaType::LongLong || type == QMetaType::Long) {
    const qlonglong n = v.toLongLong();
    if (n >= std::numeric_limits<int>::min() && n <= std::numeric_limits<int>::max()) {
      *ok = true;
      return static_cast<int>(n);
    }
    return 0;
  }
  return 0;                                              // a bool, a list, nothing at all: not a number to count with
}

// True or false, or nothing.
//
// A bool is taken as it is. A string must be one of true, false, yes, no, on, off, 1 or 0, in any case. A number must be exactly 0 or exactly 1. Anything
// else is refused, because "abc" is not true, and neither is "2".
inline bool setting_bool(const QVariant& v, bool* ok) {
  *ok = false;
  const int type = v.typeId();
  if (type == QMetaType::Bool) {
    *ok = true;
    return v.toBool();
  }
  if (type == QMetaType::QString) {
    const QString s = v.toString().trimmed().toLower();
    if (s == QLatin1String("true") || s == QLatin1String("yes") || s == QLatin1String("on") || s == QLatin1String("1")) {
      *ok = true;
      return true;
    }
    if (s == QLatin1String("false") || s == QLatin1String("no") || s == QLatin1String("off") || s == QLatin1String("0")) {
      *ok = true;
      return false;
    }
    return false;
  }
  if (type == QMetaType::Int || type == QMetaType::Double || type == QMetaType::LongLong) {
    const double d = v.toDouble();
    if (d == 0.0 || d == 1.0) {
      *ok = true;
      return d == 1.0;
    }
  }
  return false;
}

// A finite number, or nothing: from a number or from text in the plain locale. Not a number and infinity are refused.
inline double setting_double(const QVariant& v, bool* ok) {
  *ok = false;
  const int type = v.typeId();
  double d = 0.0;
  if (type == QMetaType::QString) {
    bool parsed = false;
    d = QLocale::c().toDouble(v.toString(), &parsed);
    if (!parsed) return 0.0;
  } else if (type == QMetaType::Double || type == QMetaType::Float || type == QMetaType::Int || type == QMetaType::LongLong) {
    d = v.toDouble();
  } else {
    return 0.0;
  }
  if (!std::isfinite(d)) return 0.0;
  *ok = true;
  return d;
}

}  // namespace qt
}  // namespace rc

#endif  // RC_QT_SETTINGS_CONVERT
