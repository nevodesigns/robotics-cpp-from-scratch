// The framework's main is suppressed because a QML engine needs a QGuiApplication to exist before it does.
#define RC_TEST_NO_MAIN
#include <rc/test/rc_test.hpp>

#include <QtCore/QFile>
#include <QtCore/QTemporaryDir>
#include <QtCore/QTextStream>
#include <QtCore/QUrl>
#include <QtGui/QGuiApplication>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlEngine>

#include <iostream>
#include <memory>

#include "solution.hpp"

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

// Reads the limit at the moment it starts, and has a binding that follows it.
const char* kStarts =
    "import QtQuick\n"
    "Item {\n"
    "  property real limit: 1\n"
    "  property real seen: -1\n"
    "  property real doubled: limit * 2\n"
    "  property int completions: 0\n"
    "  Component.onCompleted: { seen = limit; completions += 1 }\n"
    "}\n";

const char* kRequired =
    "import QtQuick\n"
    "Item { required property real limit; property real seen: limit }\n";

}  // namespace

RC_TEST("a property set after create() is not what Component.onCompleted saw, and one set before it is") {
  QTemporaryDir dir;
  QQmlEngine engine;
  QQmlComponent late(&engine, write_qml(dir, kStarts));
  std::unique_ptr<QObject> after(late.create());
  RC_CHECK(after != nullptr);
  if (after == nullptr) return;
  after->setProperty("limit", 5.0);

  QQmlComponent early(&engine, write_qml(dir, kStarts));
  QString why;
  std::unique_ptr<QObject> before(create_with(early, {{"limit", 5.0}}, &why));
  RC_CHECK(before != nullptr);
  if (before == nullptr) return;
  std::cout << "  limit 5 set after create(): onCompleted saw " << after->property("seen").toDouble() << ", the binding gives " << after->property("doubled").toDouble()
            << "; set as an initial property: onCompleted saw " << before->property("seen").toDouble() << ", the binding gives " << before->property("doubled").toDouble() << "\n\n";
  RC_CHECK_NEAR(after->property("seen").toDouble(), 1.0, 1e-12);          // the default: it ran before the set
  RC_CHECK_NEAR(after->property("doubled").toDouble(), 10.0, 1e-12);      // the binding followed
  RC_CHECK_NEAR(before->property("seen").toDouble(), 5.0, 1e-12);
  RC_CHECK_NEAR(before->property("doubled").toDouble(), 10.0, 1e-12);
  RC_CHECK_EQ(before->property("completions").toInt(), 1);                // completed once
}

RC_TEST("a required property that was not given makes create() fail, and giving it makes the object") {
  QTemporaryDir dir;
  QQmlEngine engine;
  QQmlComponent plain(&engine, write_qml(dir, kRequired));
  std::unique_ptr<QObject> none(plain.create());
  QString first_error = plain.errors().isEmpty() ? QString() : plain.errors().first().description();
  std::cout << "  create() without it: " << (none ? "an object" : "null") << ", error \"" << first_error.toStdString() << "\"\n";
  RC_CHECK(none == nullptr);
  RC_CHECK(first_error.contains("limit"));

  QQmlComponent given(&engine, write_qml(dir, kRequired));
  QString why;
  std::unique_ptr<QObject> made(create_with(given, {{"limit", 3.0}}, &why));
  RC_CHECK(made != nullptr);
  if (made) RC_CHECK_NEAR(made->property("seen").toDouble(), 3.0, 1e-12);

  QQmlComponent forgot(&engine, write_qml(dir, kRequired));
  QString forgot_why;
  QObject* nothing = create_with(forgot, QVariantMap(), &forgot_why);
  std::cout << "  create_with and an empty map: " << (nothing ? "an object" : "null") << ", \"" << forgot_why.trimmed().toStdString() << "\"\n\n";
  RC_CHECK(nothing == nullptr);
  RC_CHECK(forgot_why.contains("limit"));
  delete nothing;
}

RC_TEST("a name that the object does not declare is a refusal that names it, and nothing is created") {
  QTemporaryDir dir;
  QQmlEngine engine;
  QQmlComponent c(&engine, write_qml(dir, kStarts));
  QString why;
  QObject* object = create_with(c, {{"limt", 5.0}, {"nothing_like_this", 1}, {"limit", 2.0}}, &why);
  std::cout << "  initial properties limt, nothing_like_this and limit: " << (object ? "an object" : "null") << ", \"" << why.trimmed().toStdString() << "\"\n\n";
  RC_CHECK(object == nullptr);
  RC_CHECK(why.contains("limt"));
  RC_CHECK(why.contains("nothing_like_this"));
  RC_CHECK(!why.contains("property named limit"));              // the one that is right is not reported
  delete object;

  // And the same component is still good for the right names.
  QQmlComponent again(&engine, write_qml(dir, kStarts));
  std::unique_ptr<QObject> fine(create_with(again, {{"limit", 2.0}}, &why));
  RC_CHECK(fine != nullptr);
}

RC_TEST("a component that is not ready, or that has errors, makes no object and says why") {
  QTemporaryDir dir;
  QQmlEngine engine;
  QQmlComponent bad(&engine, write_qml(dir, "import QtQuick\nItem { width: }\n"));
  QString why;
  QObject* a = create_with(bad, QVariantMap(), &why);
  std::cout << "  a syntax error: " << (a ? "an object" : "null") << ", \"" << why.trimmed().toStdString() << "\"\n";
  RC_CHECK(a == nullptr);
  RC_CHECK(why.contains("Expected"));                         // the message of the syntax error, not an invented one
  delete a;

  QQmlComponent empty(&engine);
  QString why2;
  QObject* b = create_with(empty, QVariantMap(), &why2);
  std::cout << "  no source: " << (b ? "an object" : "null") << ", \"" << why2.toStdString() << "\"\n\n";
  RC_CHECK(b == nullptr);
  RC_CHECK(why2.contains("not ready"));
  delete b;

  // A null for the reason is allowed.
  QObject* c = create_with(empty, QVariantMap(), nullptr);
  RC_CHECK(c == nullptr);
}

RC_TEST("an object made with its properties set is the same object plain creation makes when there is nothing to set") {
  QTemporaryDir dir;
  QQmlEngine engine;
  QQmlComponent a(&engine, write_qml(dir, kStarts));
  QQmlComponent b(&engine, write_qml(dir, kStarts));
  std::unique_ptr<QObject> plain(a.create());
  QString why;
  std::unique_ptr<QObject> with(create_with(b, QVariantMap(), &why));
  RC_CHECK(plain != nullptr);
  RC_CHECK(with != nullptr);
  if (!plain || !with) return;
  RC_CHECK_NEAR(plain->property("seen").toDouble(), with->property("seen").toDouble(), 1e-12);
  RC_CHECK_NEAR(plain->property("doubled").toDouble(), with->property("doubled").toDouble(), 1e-12);
  RC_CHECK_EQ(plain->property("completions").toInt(), with->property("completions").toInt());
  RC_CHECK_EQ(with->property("completions").toInt(), 1);
}

int main(int argc, char** argv) {
  qputenv("QT_QPA_PLATFORM", "offscreen");
  QGuiApplication app(argc, argv);
  return rc::test::run_all();
}
