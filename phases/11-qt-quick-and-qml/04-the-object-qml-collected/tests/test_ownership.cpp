// The framework's main is suppressed because a QML engine needs a QGuiApplication to exist before it does.
#define RC_TEST_NO_MAIN
#include <rc/test/rc_test.hpp>

#include <QtCore/QCoreApplication>
#include <QtCore/QEvent>
#include <QtCore/QFile>
#include <QtCore/QPointer>
#include <QtCore/QTemporaryDir>
#include <QtCore/QTextStream>
#include <QtCore/QUrl>
#include <QtGui/QGuiApplication>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlContext>
#include <QtQml/QQmlEngine>

#include <iostream>
#include <memory>

#include "solution.hpp"

namespace {

int g_alive = 0;

class Tracked : public QObject {
  Q_OBJECT
 public:
  explicit Tracked(QObject* parent = nullptr) : QObject(parent) { ++g_alive; }
  ~Tracked() override { --g_alive; }
};

// What QML calls. Each function returns a new object, in a different way.
class Factory : public QObject {
  Q_OBJECT
  Q_PROPERTY(QObject* held READ held CONSTANT)
 public:
  explicit Factory(QObject* parent = nullptr) : QObject(parent), held_(new Tracked(this)) { remembered_ = nullptr; }

  QObject* held() const { return held_; }
  QObject* remembered() const { return remembered_; }

  // No parent, and a raw pointer kept for later: the mistake.
  Q_INVOKABLE QObject* make_loose() {
    Tracked* t = new Tracked();
    remembered_ = t;
    return t;
  }
  // Given a parent.
  Q_INVOKABLE QObject* make_parented() {
    Tracked* t = new Tracked(this);
    remembered_ = t;
    return t;
  }
  // Told the engine that C++ owns it, with no parent: then C++ has to delete it.
  Q_INVOKABLE QObject* make_cpp_owned() {
    Tracked* t = new Tracked();
    QQmlEngine::setObjectOwnership(t, QQmlEngine::CppOwnership);
    remembered_ = t;
    return t;
  }
  // The pool of the lesson owns it.
  Q_INVOKABLE QObject* make_pooled() {
    Tracked* t = new Tracked();
    remembered_ = t;
    return pool_.add(t);
  }
  Pool& pool() { return pool_; }

 private:
  QObject* held_;
  QObject* remembered_;
  Pool pool_;
};

QUrl write_qml(QTemporaryDir& dir, const QString& body) {
  const QString path = dir.filePath("main.qml");
  QFile file(path);
  if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    QTextStream out(&file);
    out << body;
  }
  return QUrl::fromLocalFile(path);
}

}  // namespace

RC_TEST("an object returned with no parent is deleted by the garbage collector, and the pointer C++ kept is dangling") {
  g_alive = 0;
  QPointer<QObject> watcher;
  {
    QTemporaryDir dir;
    QQmlEngine engine;
    Factory factory;
    engine.rootContext()->setContextProperty("factory", &factory);
    const QUrl url = write_qml(dir, "import QtQuick\nItem { Component.onCompleted: { var q = factory.make_pooled(); var o = factory.make_loose(); } }\n");
    QQmlComponent component(&engine, url);
    std::unique_ptr<QObject> root(component.create());
    watcher = factory.remembered();
    const int alive_before = g_alive;
    engine.collectGarbage();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCoreApplication::processEvents();
    std::cout << "  returned with no parent: " << alive_before << " Tracked objects alive after the call, " << g_alive << " after a collection; the pointer C++ kept is "
              << (watcher.isNull() ? "gone" : "still valid") << "\n";
        RC_CHECK(watcher.isNull());                       // the collector deleted it
    RC_CHECK_EQ(g_alive, 2);                           // the one the Factory holds for its property, and the pooled one
    RC_CHECK_EQ(factory.pool().count(), 1u);           // which the collection did not touch
  }
}

RC_TEST("a parent keeps the object alive, and it goes when the parent does") {
  g_alive = 0;
  QTemporaryDir dir;
  QQmlEngine engine;
  QPointer<QObject> watcher;
  {
    Factory factory;
    engine.rootContext()->setContextProperty("factory", &factory);
    const QUrl url = write_qml(dir, "import QtQuick\nItem { Component.onCompleted: { var q = factory.make_pooled(); var o = factory.make_parented(); } }\n");
    QQmlComponent component(&engine, url);
    std::unique_ptr<QObject> root(component.create());
    watcher = factory.remembered();
    engine.collectGarbage();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCoreApplication::processEvents();
    std::cout << "  returned with a parent: after a collection the pointer is " << (watcher.isNull() ? "gone" : "valid") << "; alive " << g_alive << "\n";
    RC_CHECK(!watcher.isNull());
    RC_CHECK_EQ(g_alive, 3);                           // the held one, the pooled one and this one
    RC_CHECK_EQ(factory.pool().count(), 1u);
  }
  RC_CHECK(watcher.isNull());                          // the factory is gone, and its children
  RC_CHECK_EQ(g_alive, 0);
}

RC_TEST("telling the engine that C++ owns an unparented object keeps it, and then C++ must delete it") {
  g_alive = 0;
  QTemporaryDir dir;
  QQmlEngine engine;
  QPointer<QObject> watcher;
  Factory factory;
  engine.rootContext()->setContextProperty("factory", &factory);
  const QUrl url = write_qml(dir, "import QtQuick\nItem { Component.onCompleted: { var q = factory.make_pooled(); var o = factory.make_cpp_owned(); } }\n");
  QQmlComponent component(&engine, url);
  std::unique_ptr<QObject> root(component.create());
  watcher = factory.remembered();
  engine.collectGarbage();
  QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
  QCoreApplication::processEvents();
  std::cout << "  returned as CppOwnership: after a collection the pointer is " << (watcher.isNull() ? "gone" : "valid") << ", alive " << g_alive << "\n";
  RC_CHECK(!watcher.isNull());
  RC_CHECK_EQ(g_alive, 3);
  RC_CHECK_EQ(factory.pool().count(), 1u);
  delete watcher.data();                               // nobody else will
  RC_CHECK_EQ(g_alive, 2);
}

RC_TEST("an object read from a property is not collected, and the pool is one owner for everything it was given") {
  g_alive = 0;
  QTemporaryDir dir;
  QQmlEngine engine;
  QPointer<QObject> watcher;
  Factory factory;
  engine.rootContext()->setContextProperty("factory", &factory);
  const QUrl url = write_qml(dir, "import QtQuick\nItem { Component.onCompleted: { var h = factory.held; var p1 = factory.make_pooled(); var p2 = factory.make_pooled(); } }\n");
  QQmlComponent component(&engine, url);
  std::unique_ptr<QObject> root(component.create());
  watcher = factory.remembered();
  engine.collectGarbage();
  QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
  QCoreApplication::processEvents();
  std::cout << "  the held property and two pooled objects after a collection: alive " << g_alive << ", the pool owns " << factory.pool().count() << "\n";
  RC_CHECK_EQ(g_alive, 3);                             // the held one, and the two from the pool
  RC_CHECK_EQ(factory.pool().count(), 2u);
  RC_CHECK(!watcher.isNull());
  if (watcher.isNull()) return;                        // nothing more to look at
  RC_CHECK(QQmlEngine::objectOwnership(watcher.data()) == QQmlEngine::CppOwnership);
  RC_CHECK(watcher->parent() == &factory.pool());
  factory.pool().clear();
  std::cout << "  after pool.clear(): alive " << g_alive << ", the pool owns " << factory.pool().count() << "\n\n";
  RC_CHECK_EQ(g_alive, 1);
  RC_CHECK_EQ(factory.pool().count(), 0u);
  RC_CHECK(watcher.isNull());
}

RC_TEST("keep_alive sets the parent and the ownership, twice is the same as once, and null is harmless") {
  QQmlEngine engine;
  QPointer<QObject> watcher;
  {
    QObject owner;
    QObject* object = new QObject();
    watcher = object;
    keep_alive(object, &owner);
    RC_CHECK(object->parent() == &owner);
    RC_CHECK(QQmlEngine::objectOwnership(object) == QQmlEngine::CppOwnership);
    keep_alive(object, &owner);
    RC_CHECK_EQ(owner.children().size(), 1);
    keep_alive(nullptr, &owner);
    RC_CHECK_EQ(owner.children().size(), 1);
    QObject other;
    keep_alive(object, &other);                          // moved to a new owner
    RC_CHECK(object->parent() == &other);
    RC_CHECK_EQ(owner.children().size(), 0);
  }
  // With a working keep_alive the last owner has deleted it. If not, it is still here, and is deleted so that the failure does not also leak.
  delete watcher.data();
}

int main(int argc, char** argv) {
  qputenv("QT_QPA_PLATFORM", "offscreen");
  QGuiApplication app(argc, argv);
  return rc::test::run_all();
}

#include "test_ownership.moc"
