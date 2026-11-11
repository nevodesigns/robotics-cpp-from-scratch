// The framework's main is suppressed because a QML engine needs a QGuiApplication to exist before it does.
#define RC_TEST_NO_MAIN
#include <rc/test/rc_test.hpp>

#include <QtCore/QAbstractListModel>
#include <QtCore/QCoreApplication>
#include <QtCore/QFile>
#include <QtCore/QStringList>
#include <QtCore/QTemporaryDir>
#include <QtCore/QTextStream>
#include <QtCore/QUrl>
#include <QtGui/QGuiApplication>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlContext>
#include <QtQml/QQmlEngine>

#include <iostream>
#include <memory>
#include <vector>

#include "solution.hpp"

namespace {

// Two list properties that QML pushes onto.
class Holder : public QObject {
  Q_OBJECT
  Q_PROPERTY(QStringList names READ names WRITE setNames NOTIFY namesChanged)
  Q_PROPERTY(QVariantList values READ values WRITE setValues NOTIFY valuesChanged)
 public:
  QStringList names() const { return names_; }
  void setNames(const QStringList& v) { names_ = v; emit namesChanged(); }
  QVariantList values() const { return values_; }
  void setValues(const QVariantList& v) { values_ = v; emit valuesChanged(); }
 signals:
  void namesChanged();
  void valuesChanged();
 private:
  QStringList names_{"a", "b"};
  QVariantList values_{1, 2};
};

// A model that changes its rows and does not say so.
class SilentModel : public QAbstractListModel {
  Q_OBJECT
 public:
  int rowCount(const QModelIndex& parent = QModelIndex()) const override { return parent.isValid() ? 0 : static_cast<int>(values_.size()); }
  QVariant data(const QModelIndex& index, int role) const override {
    if (!index.isValid() || role != Qt::UserRole + 1) return QVariant();
    return values_[static_cast<std::size_t>(index.row())];
  }
  QHash<int, QByteArray> roleNames() const override { return {{Qt::UserRole + 1, "value"}}; }
  void append(double v) { values_.push_back(v); }          // no beginInsertRows, no endInsertRows
 private:
  std::vector<double> values_;
};

// A file of a name that has not been used: an engine keeps a compiled copy of a file by its URL, and a file that is written again under the same name in the
// same engine is not read again.
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

// A Repeater over `model_object`, and the count and the second delegate's value, read back through the root.
const char* kRepeater =
    "import QtQuick\n"
    "Item {\n"
    "  property alias count: r.count\n"
    "  property real second: r.count > 1 ? r.itemAt(1).v : -1\n"
    "  Repeater { id: r; model: theModel; delegate: Item { property real v: value } }\n"
    "}\n";

}  // namespace

RC_TEST("a push from JavaScript onto a QStringList property is written back, and whether a QVariantList follows depends on the version of Qt") {
  QTemporaryDir dir;
  QQmlEngine engine;
  Holder holder;
  int names_signals = 0, values_signals = 0;
  QObject::connect(&holder, &Holder::namesChanged, [&] { ++names_signals; });
  QObject::connect(&holder, &Holder::valuesChanged, [&] { ++values_signals; });
  engine.rootContext()->setContextProperty("holder", &holder);
  QQmlComponent c(&engine, write_qml(dir, "import QtQuick\nItem { Component.onCompleted: { holder.names.push('c'); holder.values.push(3); } }\n"));
  std::unique_ptr<QObject> root(c.create());
  std::cout << "  after one push onto each: the QStringList has " << holder.names().size() << " (changed signals " << names_signals << "), the QVariantList has "
            << holder.values().size() << " (changed signals " << values_signals << ")\n";
  RC_CHECK_EQ(holder.names().size(), 3);
  // Qt 6.2 pushed onto a copy of the QVariantList (2, no signal); Qt 6.4 and 6.5 write it back (3, one signal). Either way the size and the signals agree.
  RC_CHECK(holder.values().size() == 2 || holder.values().size() == 3);
  RC_CHECK_EQ(values_signals, holder.values().size() - 2);

  // The model of this lesson takes the same push through a function, and says so.
  SampleModel model;
  int inserted = 0;
  QObject::connect(&model, &QAbstractItemModel::rowsInserted, [&] { ++inserted; });
  engine.rootContext()->setContextProperty("sample", &model);
  QQmlComponent c2(&engine, write_qml(dir, "import QtQuick\nItem { Component.onCompleted: { sample.append(1.5); sample.append(2.5); } }\n"));
  std::unique_ptr<QObject> root2(c2.create());
  std::cout << "  two appends to the model: " << model.rowCount() << " rows, " << inserted << " rowsInserted signals\n\n";
  RC_CHECK_EQ(model.rowCount(), 2);
  RC_CHECK_EQ(inserted, 2);
}

RC_TEST("a model that changes without begin and end has the right row count and a view that does not know") {
  QTemporaryDir dir;
  QQmlEngine engine;
  SilentModel silent;
  SampleModel good;
  silent.append(1.0);
  good.append(1.0);
  QQmlContext* silent_ctx = new QQmlContext(engine.rootContext(), &engine);
  silent_ctx->setContextProperty("theModel", &silent);
  QQmlContext* good_ctx = new QQmlContext(engine.rootContext(), &engine);
  good_ctx->setContextProperty("theModel", &good);

  QQmlComponent c(&engine, write_qml(dir, kRepeater));
  std::unique_ptr<QObject> view_of_silent(c.create(silent_ctx));
  std::unique_ptr<QObject> view_of_good(c.create(good_ctx));
  RC_CHECK_EQ(view_of_silent->property("count").toInt(), 1);
  RC_CHECK_EQ(view_of_good->property("count").toInt(), 1);

  silent.append(2.0);
  good.append(2.0);
  QCoreApplication::processEvents();
  std::cout << "  after a second row: the silent model has " << silent.rowCount() << " rows and its view shows " << view_of_silent->property("count").toInt()
            << "; the good model has " << good.rowCount() << " rows and its view shows " << view_of_good->property("count").toInt() << "\n\n";
  RC_CHECK_EQ(silent.rowCount(), 2);
  RC_CHECK_EQ(view_of_silent->property("count").toInt(), 1);        // stale
  RC_CHECK_EQ(good.rowCount(), 2);
  RC_CHECK_EQ(view_of_good->property("count").toInt(), 2);          // followed
}

RC_TEST("each appended row is one rowsInserted for exactly that row, and a delegate reads it by its role name") {
  QTemporaryDir dir;
  QQmlEngine engine;
  SampleModel model;
  std::vector<int> firsts, lasts;
  QObject::connect(&model, &QAbstractItemModel::rowsInserted, [&](const QModelIndex& parent, int first, int last) {
    if (!parent.isValid()) {
      firsts.push_back(first);
      lasts.push_back(last);
    }
  });
  engine.rootContext()->setContextProperty("theModel", &model);
  QQmlComponent c(&engine, write_qml(dir, kRepeater));
  std::unique_ptr<QObject> view(c.create());
  RC_CHECK_EQ(view->property("count").toInt(), 0);
  model.append(10.5);
  model.append(20.5);
  model.append(30.5);
  QCoreApplication::processEvents();
  std::cout << "  three appends: rowsInserted for rows";
  for (std::size_t i = 0; i < firsts.size(); ++i) std::cout << " " << firsts[i] << "-" << lasts[i];
  std::cout << "; the Repeater has " << view->property("count").toInt() << " items and the second reads " << view->property("second").toDouble() << "\n\n";
  RC_CHECK_EQ(static_cast<int>(firsts.size()), 3);
  if (firsts.size() == 3) {
    RC_CHECK(firsts[0] == 0 && lasts[0] == 0);
    RC_CHECK(firsts[1] == 1 && lasts[1] == 1);
    RC_CHECK(firsts[2] == 2 && lasts[2] == 2);
  }
  RC_CHECK_EQ(view->property("count").toInt(), 3);
  RC_CHECK_NEAR(view->property("second").toDouble(), 20.5, 1e-12);
  const QHash<int, QByteArray> names = model.roleNames();
  RC_CHECK(names.value(SampleModel::ValueRole) == QByteArray("value"));
  RC_CHECK(names.value(SampleModel::PositionRole) == QByteArray("position"));
}

RC_TEST("a row, a position, and anything that is not a row or a role of this model") {
  SampleModel model;
  model.append(4.0);
  model.append(5.0);
  RC_CHECK_NEAR(model.data(model.index(0), SampleModel::ValueRole).toDouble(), 4.0, 1e-12);
  RC_CHECK_NEAR(model.data(model.index(1), SampleModel::ValueRole).toDouble(), 5.0, 1e-12);
  RC_CHECK_EQ(model.data(model.index(1), SampleModel::PositionRole).toInt(), 1);
  RC_CHECK(!model.data(model.index(2), SampleModel::ValueRole).isValid());      // one past the end
  RC_CHECK(!model.data(model.index(99), SampleModel::ValueRole).isValid());
  RC_CHECK(!model.data(QModelIndex(), SampleModel::ValueRole).isValid());
  RC_CHECK(!model.data(model.index(0), Qt::DisplayRole).isValid());               // a role it does not know
  RC_CHECK_EQ(model.rowCount(model.index(0)), 0);                                  // a list has no children
}

RC_TEST("clear is one reset, and the view is empty") {
  QTemporaryDir dir;
  QQmlEngine engine;
  SampleModel model;
  model.append(1.0);
  model.append(2.0);
  int resets = 0;
  QObject::connect(&model, &QAbstractItemModel::modelReset, [&] { ++resets; });
  engine.rootContext()->setContextProperty("theModel", &model);
  QQmlComponent c(&engine, write_qml(dir, kRepeater));
  std::unique_ptr<QObject> view(c.create());
  RC_CHECK_EQ(view->property("count").toInt(), 2);
  model.clear();
  QCoreApplication::processEvents();
  std::cout << "  clear: " << resets << " reset, " << model.rowCount() << " rows, the Repeater has " << view->property("count").toInt() << " items\n\n";
  RC_CHECK_EQ(resets, 1);
  RC_CHECK_EQ(model.rowCount(), 0);
  RC_CHECK_EQ(view->property("count").toInt(), 0);
  model.append(9.0);
  QCoreApplication::processEvents();
  RC_CHECK_EQ(view->property("count").toInt(), 1);
}

int main(int argc, char** argv) {
  qputenv("QT_QPA_PLATFORM", "offscreen");
  QGuiApplication app(argc, argv);
  return rc::test::run_all();
}

#include "test_model.moc"
