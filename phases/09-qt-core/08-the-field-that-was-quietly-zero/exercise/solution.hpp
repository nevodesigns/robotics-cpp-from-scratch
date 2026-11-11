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
  // TODO: v = object.value(key). If v.isUndefined(): error Missing. If !v.isDouble(): error WrongType. Otherwise x = v.toDouble();
  // if !(x >= low && x <= high) the error is OutOfRange (written that way round so that a number that is not a number
  // fails too); else value = x and error None. Return the NumberField.
  (void)object;
  (void)key;
  (void)low;
  (void)high;
  return NumberField();
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
  // TODO: Missing if undefined, WrongType if not a number (isDouble). With d = v.toDouble(): NotWhole if d is not equal to
  // std::floor(d) or std::fabs(d) is at least 9.2e18. Otherwise the value is v.toInteger(), which keeps all the digits where
  // toDouble would not, and the error is None.
  (void)object;
  (void)key;
  return IdField();
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
