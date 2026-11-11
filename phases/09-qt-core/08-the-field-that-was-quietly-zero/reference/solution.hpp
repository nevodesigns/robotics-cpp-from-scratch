#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonParseError>
#include <QtCore/QJsonValue>
#include <QtCore/QString>

#include <cmath>

// Why a field could not be read. None means it could.
enum class FieldError {
  None,
  Missing,       // the key is not there
  WrongType,     // it is there and it is not a number: a string, true, null
  NotWhole,      // an identifier that has a fractional part, or is too big to be held exactly
  OutOfRange,    // a number outside the limits the caller gave
};

inline const char* describe(FieldError e) {
  switch (e) {
    case FieldError::None: return "ok";
    case FieldError::Missing: return "missing";
    case FieldError::WrongType: return "wrong type";
    case FieldError::NotWhole: return "not a whole number";
    case FieldError::OutOfRange: return "out of range";
  }
  return "unknown";
}

struct NumberField {
  double value = 0.0;
  FieldError error = FieldError::Missing;
};

// A number from an object, or the reason there is not one.
//
// QJsonValue's toDouble(default) answers every problem the same way: with the default. A missing key, a string where a number was
// meant, a null and a real value that happens to equal the default all look alike. So the value is looked at before it is converted.
// `value(key)`, not `object[key]`: on a const object the first is Undefined for a missing key, which is the thing to test for.
inline NumberField read_number(const QJsonObject& object, const QString& key, double low, double high) {
  NumberField f;
  const QJsonValue v = object.value(key);
  if (v.isUndefined()) {
    f.error = FieldError::Missing;
    return f;
  }
  if (!v.isDouble()) {
    f.error = FieldError::WrongType;
    return f;
  }
  const double x = v.toDouble();
  if (!(x >= low && x <= high)) {                       // also false for a number that is not a number
    f.error = FieldError::OutOfRange;
    return f;
  }
  f.value = x;
  f.error = FieldError::None;
  return f;
}

struct IdField {
  qint64 value = 0;
  FieldError error = FieldError::Missing;
};

// An identifier, which must be a whole number held exactly.
//
// A double holds whole numbers exactly only up to 2^53, about nine thousand million million. A JSON reader that turns every number into a
// double hands back 9007199254740992 for 9007199254740993, a different identifier. QJsonValue::toInteger keeps the whole number when the
// text was one, and a number written with a fraction, or too big for 64 bits, is refused rather than rounded.
inline IdField read_id(const QJsonObject& object, const QString& key) {
  IdField f;
  const QJsonValue v = object.value(key);
  if (v.isUndefined()) {
    f.error = FieldError::Missing;
    return f;
  }
  if (!v.isDouble()) {
    f.error = FieldError::WrongType;
    return f;
  }
  const double as_double = v.toDouble();
  if (as_double != std::floor(as_double) || std::fabs(as_double) >= 9.2e18) {
    f.error = FieldError::NotWhole;
    return f;
  }
  f.value = v.toInteger();
  f.error = FieldError::None;
  return f;
}

// Parse a document that must be an object, and say why not if it is not.
inline bool parse_object(const QByteArray& text, QJsonObject* out, QString* why) {
  QJsonParseError error;
  const QJsonDocument doc = QJsonDocument::fromJson(text, &error);
  if (error.error != QJsonParseError::NoError) {
    *why = error.errorString();
    return false;
  }
  if (!doc.isObject()) {
    *why = QStringLiteral("not an object");
    return false;
  }
  *out = doc.object();
  return true;
}

#endif  // LESSON_SOLUTION_HPP
