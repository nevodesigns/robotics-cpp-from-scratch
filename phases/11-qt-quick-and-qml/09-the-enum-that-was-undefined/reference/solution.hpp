#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <QtCore/QMetaEnum>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtQml/qqml.h>

// A robot has modes, and an interface wants to say them by name: `mode: RobotMode.Auto`. For QML to know that name the enum has to be declared to the meta object system
// (Q_NAMESPACE and Q_ENUM_NS, or Q_ENUM in a class), and the namespace has to be registered with the engine. An enum that is only a C++ enum is not a name in QML: the
// expression is `undefined`, and nothing is reported when the file loads.
//
// And an enum in QML is an integer. A property of an enum type takes whatever number it is given, and a mode of 99 is stored without complaint, so the C++ that receives it has
// to ask whether it is one of the names.

// Q_NAMESPACE has to be inside a named namespace, and a namespace has one meta object: this one is the namespace of the modes and nothing else.
namespace robot_mode {
Q_NAMESPACE

enum class Mode { Idle = 0, Auto = 1, Manual = 2, Fault = 9 };
Q_ENUM_NS(Mode)
}  // namespace robot_mode
using robot_mode::Mode;

// The name of a mode, read from the meta object, or "unknown" for a number that is not one.
inline QString mode_name(int value) {
  const QMetaEnum meta = QMetaEnum::fromType<Mode>();
  const char* key = meta.valueToKey(value);
  return key != nullptr ? QString::fromLatin1(key) : QStringLiteral("unknown");
}

// Whether a number is one of the modes. A mode that QML assigned is an integer, and any integer is accepted there.
inline bool is_valid_mode(int value) { return QMetaEnum::fromType<Mode>().valueToKey(value) != nullptr; }

// The mode with this name, exactly: the case matters and nothing is trimmed. Returns false and leaves `*out` alone for a name that is not one.
inline bool mode_from_name(const QString& name, Mode* out) {
  bool ok = false;
  const int value = QMetaEnum::fromType<Mode>().keyToValue(name.toUtf8().constData(), &ok);
  if (!ok) return false;
  if (out != nullptr) *out = static_cast<Mode>(value);
  return true;
}

// Every name, in the order the enum declares them.
inline QStringList mode_names() {
  const QMetaEnum meta = QMetaEnum::fromType<Mode>();
  QStringList names;
  for (int i = 0; i < meta.keyCount(); ++i) names.append(QString::fromLatin1(meta.key(i)));
  return names;
}

// Makes `RobotMode` a name in QML, under `import <uri> <major>.<minor>`. Asking twice does nothing the second time.
inline void register_mode(const char* uri, int major, int minor) {
  static bool done = false;
  if (done) return;
  done = true;
  qmlRegisterUncreatableMetaObject(robot_mode::staticMetaObject, uri, major, minor, "RobotMode", QStringLiteral("RobotMode is a namespace of names"));
}

#endif  // LESSON_SOLUTION_HPP
