// The framework's main is suppressed because a QML engine needs a QGuiApplication to exist before it does.
#define RC_TEST_NO_MAIN
#include <rc/test/rc_test.hpp>

#include <QtCore/QFile>
#include <QtCore/QMetaEnum>
#include <QtCore/QObject>
#include <QtCore/QTemporaryDir>
#include <QtCore/QTextStream>
#include <QtCore/QUrl>
#include <QtGui/QGuiApplication>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlContext>
#include <QtQml/QQmlEngine>
#include <QtQml/qqml.h>

#include <iostream>
#include <memory>

#include "solution.hpp"

// An enum that is only a C++ enum, in a namespace of its own: no Q_ENUM_NS.
namespace Plain {
Q_NAMESPACE
enum class Mode { Idle = 0, Auto = 1 };
}  // namespace Plain

// A property of an enum type, which QML can assign any number to.
class Holder : public QObject {
  Q_OBJECT
  Q_PROPERTY(int mode READ mode WRITE setMode NOTIFY modeChanged)
 public:
  int mode() const { return mode_; }
  void setMode(int m) { mode_ = m; emit modeChanged(); }
 signals:
  void modeChanged();
 private:
  int mode_ = 0;
};

namespace {

QUrl write_qml(QTemporaryDir& dir, const QString& body) {
  static int serial = 0;
  const QString path = dir.filePath(QString("main%1.qml").arg(++serial));
  QFile file(path);
  if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    QTextStream out(&file);
    out << body;
  }
  return QUrl::fromLocalFile(path);
}

void register_both() {
  register_mode("Rc", 1, 0);
  static bool done = false;
  if (done) return;
  done = true;
  qmlRegisterUncreatableMetaObject(Plain::staticMetaObject, "Rc", 1, 0, "PlainMode", QStringLiteral("a namespace of names"));
}

}  // namespace

RC_TEST("an enum declared to the meta object is a name in QML and one that is not is undefined") {
  register_mode("Rc", 1, 0);
  register_both();
  QTemporaryDir dir;
  QQmlEngine engine;
  QQmlComponent c(&engine, write_qml(dir,
      "import QtQuick\nimport Rc 1.0\n"
      "Item {\n"
      "  property var declared: RobotMode.Auto\n"
      "  property var fault: RobotMode.Fault\n"
      "  property var not_declared: PlainMode.Auto\n"
      "}\n"));
  std::unique_ptr<QObject> root(c.create());
  RC_CHECK(root != nullptr);
  if (root == nullptr) return;
  const QVariant declared = root->property("declared");
  const QVariant fault = root->property("fault");
  const QVariant undeclared = root->property("not_declared");
  std::cout << "  RobotMode.Auto in QML: " << declared.toInt() << " (valid " << declared.isValid() << "), RobotMode.Fault: " << fault.toInt() << "; the enum with no Q_ENUM_NS, PlainMode.Auto: valid "
            << undeclared.isValid() << "\n\n";
  RC_CHECK(declared.isValid());
  RC_CHECK_EQ(declared.toInt(), 1);
  RC_CHECK_EQ(fault.toInt(), 9);                        // the values are the C++ values, not 0 to 3
  RC_CHECK(!undeclared.isValid());                      // undefined, and the file loaded
}

RC_TEST("names and values come from the meta object, and a number that is not a mode has no name") {
  RC_CHECK_EQ(mode_name(0).toStdString(), std::string("Idle"));
  RC_CHECK_EQ(mode_name(1).toStdString(), std::string("Auto"));
  RC_CHECK_EQ(mode_name(2).toStdString(), std::string("Manual"));
  RC_CHECK_EQ(mode_name(9).toStdString(), std::string("Fault"));
  RC_CHECK_EQ(mode_name(3).toStdString(), std::string("unknown"));       // between the modes
  RC_CHECK_EQ(mode_name(99).toStdString(), std::string("unknown"));
  RC_CHECK_EQ(mode_name(-1).toStdString(), std::string("unknown"));
  RC_CHECK(is_valid_mode(0));
  RC_CHECK(is_valid_mode(9));
  RC_CHECK(!is_valid_mode(3));
  RC_CHECK(!is_valid_mode(99));
  const QStringList names = mode_names();
  std::cout << "  the names, in order: " << names.join(", ").toStdString() << "\n\n";
  RC_CHECK_EQ(names.size(), 4);
  if (names.size() == 4) {
    RC_CHECK_EQ(names[0].toStdString(), std::string("Idle"));
    RC_CHECK_EQ(names[3].toStdString(), std::string("Fault"));
  }
}

RC_TEST("a name is read exactly: the case matters, nothing is trimmed, and a name that is not one leaves the result alone") {
  Mode m = Mode::Fault;
  RC_CHECK(mode_from_name("Auto", &m));
  RC_CHECK(m == Mode::Auto);
  RC_CHECK(mode_from_name("Idle", &m));
  RC_CHECK(m == Mode::Idle);
  m = Mode::Manual;
  RC_CHECK(!mode_from_name("auto", &m));
  RC_CHECK(!mode_from_name("AUTO", &m));
  RC_CHECK(!mode_from_name("Auto ", &m));
  RC_CHECK(!mode_from_name("", &m));
  RC_CHECK(!mode_from_name("Fault2", &m));
  RC_CHECK(!mode_from_name("1", &m));
  RC_CHECK(m == Mode::Manual);                              // untouched by every refusal
  RC_CHECK(mode_from_name("Fault", nullptr));              // a null result is allowed
  RC_CHECK(!mode_from_name("nothing", nullptr));
}

RC_TEST("QML can assign any number to a property that is meant to hold a mode, and is_valid_mode is what says no") {
  register_mode("Rc", 1, 0);
  QTemporaryDir dir;
  QQmlEngine engine;
  Holder holder;
  engine.rootContext()->setContextProperty("holder", &holder);
  QQmlComponent c(&engine, write_qml(dir,
      "import QtQuick\nimport Rc 1.0\n"
      "Item { Component.onCompleted: { holder.mode = RobotMode.Manual; holder.mode = 99 } }\n"));
  std::unique_ptr<QObject> root(c.create());
  const int stored = holder.mode();
  std::cout << "  holder.mode = RobotMode.Manual and then 99: stored " << stored << ", which is the mode \"" << mode_name(stored).toStdString() << "\", valid " << is_valid_mode(stored) << "\n\n";
  RC_CHECK_EQ(stored, 99);                    // accepted: nothing in QML stopped it
  RC_CHECK(!is_valid_mode(stored));
  RC_CHECK_EQ(mode_name(stored).toStdString(), std::string("unknown"));
  holder.setMode(static_cast<int>(Mode::Auto));
  RC_CHECK(is_valid_mode(holder.mode()));
}

RC_TEST("registering twice is the same as once, and the names are there for an engine made later") {
  register_mode("Rc", 1, 0);
  register_mode("Rc", 1, 0);
  register_mode("Other", 3, 2);                 // ignored: it was registered already
  QTemporaryDir dir;
  for (int i = 0; i < 2; ++i) {
    QQmlEngine engine;
    QQmlComponent c(&engine, write_qml(dir, "import QtQuick\nimport Rc 1.0\nItem { property int m: RobotMode.Manual }\n"));
    std::unique_ptr<QObject> root(c.create());
    RC_CHECK(root != nullptr);
    if (root) RC_CHECK_EQ(root->property("m").toInt(), 2);
  }
}

int main(int argc, char** argv) {
  qputenv("QT_QPA_PLATFORM", "offscreen");
  QGuiApplication app(argc, argv);
  return rc::test::run_all();
}

#include "test_mode.moc"
