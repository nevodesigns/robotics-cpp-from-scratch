// The framework's main is suppressed because a QML engine needs a QGuiApplication to exist before it does.
#define RC_TEST_NO_MAIN
#include <rc/test/rc_test.hpp>

#include <QtCore/QCoreApplication>
#include <QtCore/QEvent>
#include <QtCore/QFile>
#include <QtCore/QObject>
#include <QtCore/QTemporaryDir>
#include <QtCore/QTextStream>
#include <QtCore/QUrl>
#include <QtGui/QGuiApplication>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlContext>
#include <QtQml/QQmlEngine>

#include <iostream>
#include <memory>

#include <rc/qt/message_capture.hpp>

#include "solution.hpp"

namespace {

class Sensor : public QObject {
  Q_OBJECT
 public:
  Q_INVOKABLE void fire() { emit reading(); }
 signals:
  void reading();
};

class Tally : public QObject {
  Q_OBJECT
 public:
  int hits = 0;
 public slots:
  void hit() { ++hits; }
};

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

// An item that is loaded and reloaded, three times after the first. `handler` is what its content does when it is created.
QString reloading(const QString& handler) {
  return QString(
      "import QtQuick\n"
      "Item {\n"
      "  Component { id: child; Item { %1 } }\n"
      "  Loader { id: l; sourceComponent: child }\n"
      "  function reload() { l.sourceComponent = null; l.sourceComponent = child }\n"
      "}\n").arg(handler);
}

// Reloads three times and lets the old items be deleted and collected, and then fires the signal once.
struct Run {
  int hits = 0;
  std::size_t warnings = 0;
  bool reference_errors = false;
  int cpp_hits = 0;               // a handler in C++, kept by a ScopedConnections, for the same signal
  int cpp_after_clear = 0;
};

Run reload_and_fire(const QString& handler) {
  QTemporaryDir dir;
  QQmlEngine engine;
  Sensor sensor;
  Tally tally;
  engine.rootContext()->setContextProperty("sensor", &sensor);
  engine.rootContext()->setContextProperty("tally", &tally);
  QQmlComponent c(&engine, write_qml(dir, reloading(handler)));
  std::unique_ptr<QObject> root(c.create());
  Run run;
  if (!root) return run;
  int cpp_hits = 0;
  ScopedConnections scope;
  scope.add(QObject::connect(&sensor, &Sensor::reading, [&cpp_hits] { ++cpp_hits; }));
  rc::qt::MessageCapture capture;
  for (int i = 0; i < 3; ++i) {
    QMetaObject::invokeMethod(root.get(), "reload");
    QCoreApplication::processEvents();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
  }
  engine.collectGarbage();
  QCoreApplication::processEvents();
  sensor.fire();
  run.hits = tally.hits;
  run.warnings = capture.count(QtWarningMsg);
  run.reference_errors = capture.contains("ReferenceError");
  run.cpp_hits = cpp_hits;
  scope.clear();
  sensor.fire();
  run.cpp_after_clear = cpp_hits;                 // ended: not called again
  return run;
}

}  // namespace

RC_TEST("the same slot connected three times runs three times for one signal, and connect_once makes one connection") {
  Sensor sensor;
  Tally plain;
  QObject::connect(&sensor, &Sensor::reading, &plain, &Tally::hit);
  QObject::connect(&sensor, &Sensor::reading, &plain, &Tally::hit);
  QObject::connect(&sensor, &Sensor::reading, &plain, &Tally::hit);
  sensor.fire();

  Sensor other;
  Tally once;
  const bool first = connect_once(&other, &Sensor::reading, &once, &Tally::hit);
  const bool second = connect_once(&other, &Sensor::reading, &once, &Tally::hit);
  const bool third = connect_once(&other, &Sensor::reading, &once, &Tally::hit);
  other.fire();
  std::cout << "  one signal, three connections: " << plain.hits << " calls; connect_once three times: made " << first << ", " << second << ", " << third << " and " << once.hits
            << " call\n\n";
  RC_CHECK_EQ(plain.hits, 3);
  RC_CHECK(first);
  RC_CHECK(!second);
  RC_CHECK(!third);
  RC_CHECK_EQ(once.hits, 1);
}

RC_TEST("a scope keeps its connections and ends them together, by clear and by being destroyed") {
  Sensor sensor;
  Tally tally;
  {
    ScopedConnections scope;
    scope.add(QObject::connect(&sensor, &Sensor::reading, &tally, &Tally::hit));
    scope.add(QObject::connect(&sensor, &Sensor::reading, [&tally] { ++tally.hits; }));      // a lambda with no receiver
    scope.add(QMetaObject::Connection());                                                    // a failed connect is not kept
    RC_CHECK_EQ(scope.count(), 2u);
    sensor.fire();
    RC_CHECK_EQ(tally.hits, 2);
    scope.clear();
    RC_CHECK_EQ(scope.count(), 0u);
    sensor.fire();
    RC_CHECK_EQ(tally.hits, 2);                  // nothing is connected now
    scope.add(QObject::connect(&sensor, &Sensor::reading, [&tally] { tally.hits += 10; }));
    sensor.fire();
    RC_CHECK_EQ(tally.hits, 12);
  }
  sensor.fire();                                 // the scope is gone, and so is its lambda, which referred to tally
  std::cout << "  after the scope ended, one more signal: " << tally.hits << " calls in all\n\n";
  RC_CHECK_EQ(tally.hits, 12);
}

RC_TEST("a handler that was connected in JavaScript by an item that is gone still runs, and throws") {
  const Run run = reload_and_fire("Component.onCompleted: sensor.reading.connect(function() { tally.hit() })");
  std::cout << "  three reloads and one signal: the live item's handler ran " << run.hits << " time, and " << run.warnings << " warnings came from handlers of items that were gone"
            << (run.reference_errors ? " (ReferenceError)" : "") << "\n";
  RC_CHECK_EQ(run.hits, 1);
  RC_CHECK_EQ(run.warnings, 3u);
  RC_CHECK(run.reference_errors);
  RC_CHECK_EQ(run.cpp_hits, 1);                  // the handler in C++ ran once for the one signal
  RC_CHECK_EQ(run.cpp_after_clear, 1);           // and not again once its scope was cleared
}

RC_TEST("a Connections element goes with its item, so no handler outlives it") {
  const Run run = reload_and_fire("Connections { target: sensor; function onReading() { tally.hit() } }");
  std::cout << "  the same with Connections: the live handler ran " << run.hits << " time, " << run.warnings << " warnings\n\n";
  RC_CHECK_EQ(run.hits, 1);
  RC_CHECK_EQ(run.warnings, 0u);
  RC_CHECK_EQ(run.cpp_hits, 1);
  RC_CHECK_EQ(run.cpp_after_clear, 1);
}

RC_TEST("a connection with a receiver ends with it, and one with none does not until it is ended") {
  Sensor sensor;
  Tally outer;
  {
    auto receiver = std::unique_ptr<QObject>(new QObject());
    QObject::connect(&sensor, &Sensor::reading, receiver.get(), [&outer] { ++outer.hits; });     // the receiver is the context
    sensor.fire();
    RC_CHECK_EQ(outer.hits, 1);
  }
  sensor.fire();                                                                                 // the receiver is gone: nothing is called
  std::cout << "  after the receiver was deleted, one more signal: " << outer.hits << " calls in all\n\n";
  RC_CHECK_EQ(outer.hits, 1);

  // The scope of this lesson is what ends one that has no receiver.
  ScopedConnections scope;
  scope.add(QObject::connect(&sensor, &Sensor::reading, [&outer] { outer.hits += 100; }));
  sensor.fire();
  RC_CHECK_EQ(outer.hits, 101);
  scope.clear();
  sensor.fire();
  RC_CHECK_EQ(outer.hits, 101);
}

int main(int argc, char** argv) {
  qputenv("QT_QPA_PLATFORM", "offscreen");
  QGuiApplication app(argc, argv);
  return rc::test::run_all();
}

#include "test_connections.moc"
