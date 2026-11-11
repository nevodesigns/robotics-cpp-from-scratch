#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QVariant>
#include <QtQml/QQmlContext>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlExpression>
#include <QtQml/QQmlProperty>
#include <QtQml/qqml.h>

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
  // TODO: no scope: ok false, error "no object to evaluate in". An empty expression (after trimmed()): ok false, error "the expression is empty". Otherwise
  // context = qmlContext(scope), or engine.rootContext() if that is null; QQmlExpression query(context, scope, expression); value = query.evaluate().
  // If query.hasError(): ok false, error = query.error().description(), and an empty value. Otherwise ok true.
  (void)engine;
  (void)scope;
  (void)expression;
  ProbeResult result;
  result.error = QStringLiteral("not written yet");
  return result;
}

// The value of a property that the object declares. A name that is not a property is an error and not an empty value.
inline ProbeResult read(QObject* object, const QString& name) {
  // TODO: no object: ok false. QQmlProperty property(object, name); if !property.isValid(): ok false, error "no property named " + name. Otherwise value =
  // property.read(), ok true.
  (void)object;
  (void)name;
  ProbeResult result;
  result.error = QStringLiteral("not written yet");
  return result;
}

// Writes a property that the object declares. A name that is not a property, a property that cannot be written and a value of a type that does not fit are each an
// error, and nothing is created.
inline ProbeResult set(QObject* object, const QString& name, const QVariant& value) {
  // TODO: no object: ok false. QQmlProperty property(object, name); not valid: error "no property named " + name; not property.isWritable(): error
  // name + " cannot be written"; property.write(value) false: error "could not write that value to " + name. Otherwise ok true.
  (void)object;
  (void)name;
  (void)value;
  ProbeResult result;
  result.error = QStringLiteral("not written yet");
  return result;
}

#endif  // LESSON_SOLUTION_HPP
