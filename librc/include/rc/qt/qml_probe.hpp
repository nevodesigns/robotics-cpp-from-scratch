// rc/qt/qml_probe.hpp
//
// Reading, writing and evaluating on a QML object without silences, from lesson 11-06, graduated.
//
// setProperty on a misspelt name returned false and created a dynamic property, and property() gave an empty QVariant; a JavaScript assignment to a bound width replaced the binding for good, and Qt.binding put it back. The probe reports each of
// those as an error with its text.

#ifndef RC_QT_QML_PROBE_HPP
#define RC_QT_QML_PROBE_HPP

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QVariant>
#include <QtQml/QQmlContext>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlExpression>
#include <QtQml/QQmlProperty>
#include <QtQml/qqml.h>

namespace rc {
namespace qt {

// A test of a QML interface reads and writes properties from C++, and the easy way to do it is the quiet way. QObject::setProperty on a name that is not a property returns
// false and creates a dynamic property of that name, so a misspelt width is accepted and changes nothing. QObject::property on a name that does not exist gives an empty
// QVariant, which converts to 0 and to the empty string. A test that does either asserts about a property that was never there.
//
// The probe says what happened: a result with the value, whether it worked, and why not. And an assignment in JavaScript, or through QQmlProperty, replaces a binding
// for good: the property has a value and no longer follows what it followed.

struct ProbeResult {
  QVariant value;
  bool ok = false;
  QString error;
};

// An expression evaluated as if it were written in the object's own QML: its properties by name, the ids of the file that made it, JavaScript. Any error, a syntax
// error, a name that is not defined, a thrown exception, an empty expression or no object, is ok == false with the message.
inline ProbeResult evaluate(QQmlEngine& engine, QObject* scope, const QString& expression) {
  ProbeResult result;
  if (scope == nullptr) {
    result.error = QStringLiteral("no object to evaluate in");
    return result;
  }
  if (expression.trimmed().isEmpty()) {
    result.error = QStringLiteral("the expression is empty");
    return result;
  }
  QQmlContext* context = qmlContext(scope);
  QQmlExpression query(context != nullptr ? context : engine.rootContext(), scope, expression);
  result.value = query.evaluate();
  if (query.hasError()) {
    result.error = query.error().description();
    result.value = QVariant();
    return result;
  }
  result.ok = true;
  return result;
}

// The value of a property that the object declares. A name that is not a property is an error and not an empty value.
inline ProbeResult read(QObject* object, const QString& name) {
  ProbeResult result;
  if (object == nullptr) {
    result.error = QStringLiteral("no object to read from");
    return result;
  }
  QQmlProperty property(object, name);
  if (!property.isValid()) {
    result.error = QStringLiteral("no property named ") + name;
    return result;
  }
  result.value = property.read();
  result.ok = true;
  return result;
}

// Writes a property that the object declares. A name that is not a property, a property that cannot be written and a value of a type that does not fit are each an
// error, and nothing is created.
inline ProbeResult set(QObject* object, const QString& name, const QVariant& value) {
  ProbeResult result;
  if (object == nullptr) {
    result.error = QStringLiteral("no object to write to");
    return result;
  }
  QQmlProperty property(object, name);
  if (!property.isValid()) {
    result.error = QStringLiteral("no property named ") + name;
    return result;
  }
  if (!property.isWritable()) {
    result.error = name + QStringLiteral(" cannot be written");
    return result;
  }
  if (!property.write(value)) {
    result.error = QStringLiteral("could not write that value to ") + name;
    return result;
  }
  result.ok = true;
  return result;
}
}  // namespace qt
}  // namespace rc

#endif  // RC_QT_QML_PROBE_HPP
