// The framework's main is suppressed because a QML engine needs a QGuiApplication to exist before it does.
#define RC_TEST_NO_MAIN
#include <rc/test/rc_test.hpp>

#include <QtCore/QCoreApplication>
#include <QtCore/QFile>
#include <QtCore/QTemporaryDir>
#include <QtCore/QTextStream>
#include <QtCore/QUrl>
#include <QtGui/QGuiApplication>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlContext>
#include <QtQml/QQmlEngine>

#include <cstdlib>
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

const char* kListView =
    "import QtQuick\n"
    "ListView { width: 200; height: 300; model: theModel\n"
    "  delegate: Item { width: 200; height: 30; Component.onCompleted: counter.created(); Component.onDestruction: counter.gone() } }\n";

const char* kRepeater =
    "import QtQuick\n"
    "Item { Repeater { model: theModel; delegate: Item { Component.onCompleted: counter.created(); Component.onDestruction: counter.gone() } } }\n";

// A view over `rows` rows, laid out. Leaves the counter showing what was made.
struct View {
  QTemporaryDir dir;
  QQmlEngine engine;
  RangeModel model;
  DelegateCounter counter;
  std::unique_ptr<QObject> root;
  View(int rows, const char* source) : model(rows) {
    engine.rootContext()->setContextProperty("theModel", &model);
    engine.rootContext()->setContextProperty("counter", &counter);
    QQmlComponent c(&engine, write_qml(dir, source));
    root.reset(c.create());
    layout();
  }
  void layout() {
    if (root && root->metaObject()->indexOfMethod("forceLayout()") >= 0) QMetaObject::invokeMethod(root.get(), "forceLayout");
    QCoreApplication::processEvents();
  }
  void jump_to(int row) {
    if (!root) return;
    QMetaObject::invokeMethod(root.get(), "positionViewAtIndex", Q_ARG(int, row), Q_ARG(int, 0));
    layout();
  }
};

}  // namespace

RC_TEST("a Repeater makes a delegate for every row, and a ListView only for the rows it shows") {
  View repeater(10000, kRepeater);
  View list(10000, kListView);
  std::cout << "  10000 rows: a Repeater made " << repeater.counter.total() << " delegates (" << repeater.counter.alive() << " alive); a ListView of height 300 and rows of 30 made "
            << list.counter.total() << " (" << list.counter.alive() << " alive)\n\n";
  RC_CHECK_EQ(repeater.counter.total(), 10000);
  RC_CHECK_EQ(repeater.counter.alive(), 10000);
  RC_CHECK(list.counter.alive() >= 10);                  // the ten rows that fit
  RC_CHECK(list.counter.alive() < 60);                   // and a few beyond them, never the ten thousand
  RC_CHECK_EQ(list.counter.total(), list.counter.alive());
}

RC_TEST("jumping a long way down a ListView makes a few delegates and lets the old ones go") {
  View list(10000, kListView);
  const int before_total = list.counter.total();
  list.jump_to(5000);
  std::cout << "  after jumping to row 5000: " << list.counter.total() - before_total << " more delegates made, " << list.counter.alive() << " alive, the most at once " << list.counter.peak()
            << "\n\n";
  RC_CHECK(list.counter.total() > before_total);          // new rows needed new delegates
  RC_CHECK(list.counter.total() - before_total < 60);
  RC_CHECK(list.counter.alive() < 60);
  RC_CHECK(list.counter.peak() < 120);
  RC_CHECK_EQ(list.counter.unmatched(), 0);
}

RC_TEST("the number of delegates of a ListView does not depend on the number of rows") {
  int alive[3] = {0, 0, 0};
  const int sizes[3] = {100, 10000, 1000000};
  for (int i = 0; i < 3; ++i) {
    View list(sizes[i], kListView);
    alive[i] = list.counter.alive();
  }
  std::cout << "  alive for 100, 10000 and 1000000 rows: " << alive[0] << ", " << alive[1] << ", " << alive[2] << "\n\n";
  RC_CHECK(alive[0] >= 10);
  RC_CHECK(std::abs(alive[0] - alive[1]) <= 5);
  RC_CHECK(std::abs(alive[1] - alive[2]) <= 5);
  RC_CHECK(alive[2] < 60);

  // A model of ten rows has ten delegates and no more.
  View small(10, kListView);
  RC_CHECK_EQ(small.counter.alive(), 10);
}

RC_TEST("the counter keeps what was made, what exists and the most there were, and a report of one going when none exist is unmatched") {
  DelegateCounter c;
  RC_CHECK_EQ(c.total(), 0);
  c.created();
  c.created();
  c.created();
  c.gone();
  RC_CHECK_EQ(c.total(), 3);
  RC_CHECK_EQ(c.alive(), 2);
  RC_CHECK_EQ(c.peak(), 3);
  c.created();
  c.gone();
  c.gone();
  c.gone();
  RC_CHECK_EQ(c.total(), 4);
  RC_CHECK_EQ(c.alive(), 0);
  RC_CHECK_EQ(c.peak(), 3);
  c.gone();
  c.gone();
  RC_CHECK_EQ(c.alive(), 0);                      // never below nothing
  RC_CHECK_EQ(c.unmatched(), 2);
}

RC_TEST("a model that computes its rows answers for any row, and says nothing for what is not one") {
  RangeModel model(1000000);
  RC_CHECK_EQ(model.rowCount(), 1000000);
  RC_CHECK_EQ(model.data(model.index(0), RangeModel::ValueRole).toInt(), 0);
  RC_CHECK_EQ(model.data(model.index(1234), RangeModel::ValueRole).toInt(), 2468);
  RC_CHECK_EQ(model.data(model.index(999999), RangeModel::ValueRole).toInt(), 1999998);
  RC_CHECK(!model.data(model.index(1000000), RangeModel::ValueRole).isValid());
  RC_CHECK(!model.data(QModelIndex(), RangeModel::ValueRole).isValid());
  RC_CHECK(!model.data(model.index(5), Qt::DisplayRole).isValid());
  RC_CHECK_EQ(model.rowCount(model.index(0)), 0);
  RC_CHECK(model.roleNames().value(RangeModel::ValueRole) == QByteArray("value"));
  RangeModel negative(-5);
  RC_CHECK_EQ(negative.rowCount(), 0);
}

int main(int argc, char** argv) {
  qputenv("QT_QPA_PLATFORM", "offscreen");
  QGuiApplication app(argc, argv);
  return rc::test::run_all();
}
