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

// An outer item, and an inner one whose width follows it.
const char* kPair =
    "import QtQuick\n"
    "Item {\n"
    "  id: outer\n"
    "  width: 200; height: 100\n"
    "  property alias inner: i\n"
    "  readonly property int fixed: 7\n"
    "  Item { id: i; width: outer.width / 2 }\n"
    "}\n";

struct Pair {
  QTemporaryDir dir;
  QQmlEngine engine;
  std::unique_ptr<QObject> outer;
  QObject* inner = nullptr;
  Pair() {
    QQmlComponent c(&engine, write_qml(dir, kPair));
    outer.reset(c.create());
    if (outer) inner = qvariant_cast<QObject*>(outer->property("inner"));
  }
  double inner_width() const { return inner ? inner->property("width").toDouble() : -1.0; }
  void set_outer(double w) { outer->setProperty("width", w); }
};

}  // namespace

RC_TEST("setProperty on a name that does not exist returns false, creates a property of that name and changes nothing") {
  Pair p;
  RC_CHECK(p.inner != nullptr);
  if (p.inner == nullptr) return;
  const bool accepted = p.outer->setProperty("widht", 300.0);              // a typo
  const bool dynamic = p.outer->dynamicPropertyNames().contains("widht");
  const double width = p.outer->property("width").toDouble();
  const QVariant typo = p.outer->property("widht");
  const QVariant unknown = p.outer->property("nothingLikeThis");
  std::cout << "  setProperty(\"widht\", 300): returned " << accepted << ", dynamic property created " << dynamic << ", width is still " << width
            << "; property(\"nothingLikeThis\") is valid: " << unknown.isValid() << ", and to double " << unknown.toDouble() << "\n";
  RC_CHECK(!accepted);
  RC_CHECK(dynamic);
  RC_CHECK_NEAR(width, 200.0, 1e-12);
  RC_CHECK(typo.isValid());                    // the dynamic property is now there to be read
  RC_CHECK(!unknown.isValid());
  RC_CHECK_NEAR(unknown.toDouble(), 0.0, 1e-12);

  // The probe refuses, and creates nothing.
  const ProbeResult refused = set(p.outer.get(), "wdth", 300.0);
  std::cout << "  the probe: ok " << refused.ok << ", \"" << refused.error.toStdString() << "\", dynamic properties now " << p.outer->dynamicPropertyNames().size() << "\n\n";
  RC_CHECK(!refused.ok);
  RC_CHECK(refused.error.contains("wdth"));
  RC_CHECK(!p.outer->dynamicPropertyNames().contains("wdth"));
  const ProbeResult ok = set(p.outer.get(), "width", 250.0);
  RC_CHECK(ok.ok);
  RC_CHECK_NEAR(p.outer->property("width").toDouble(), 250.0, 1e-12);
}

RC_TEST("read gives the value of a property that exists and an error for one that does not") {
  Pair p;
  RC_CHECK(p.inner != nullptr);
  const ProbeResult width = read(p.outer.get(), "width");
  RC_CHECK(width.ok);
  RC_CHECK_NEAR(width.value.toDouble(), 200.0, 1e-12);
  const ProbeResult typo = read(p.outer.get(), "widht");
  RC_CHECK(!typo.ok);
  RC_CHECK(typo.error.contains("widht"));
  RC_CHECK(!typo.value.isValid());
  const ProbeResult none = read(nullptr, "width");
  RC_CHECK(!none.ok);
  const ProbeResult object_valued = read(p.outer.get(), "inner");
  RC_CHECK(object_valued.ok);
  RC_CHECK(qvariant_cast<QObject*>(object_valued.value) == p.inner);

  // A property that exists and cannot be written.
  const ProbeResult fixed = read(p.outer.get(), "fixed");
  RC_CHECK(fixed.ok);
  RC_CHECK_EQ(fixed.value.toInt(), 7);
  const ProbeResult read_only = set(p.outer.get(), "fixed", QVariant(8));
  std::cout << "  writing the readonly property fixed: ok " << read_only.ok << ", \"" << read_only.error.toStdString() << "\"\n\n";
  RC_CHECK(!read_only.ok);
  RC_CHECK(read_only.error.contains("fixed"));
  RC_CHECK_EQ(p.outer->property("fixed").toInt(), 7);
  RC_CHECK(!set(nullptr, "width", 1).ok);
}

RC_TEST("an assignment replaces a binding for good, and Qt.binding puts it back") {
  Pair p;
  RC_CHECK(p.inner != nullptr);
  if (p.inner == nullptr) return;
  RC_CHECK_NEAR(p.inner_width(), 100.0, 1e-12);
  p.set_outer(300.0);
  const double followed = p.inner_width();                                  // the binding follows its source
  const ProbeResult assigned = evaluate(p.engine, p.inner, "width = 10");
  p.set_outer(400.0);
  const double after_assignment = p.inner_width();                          // and no longer does
  const ProbeResult rebound = evaluate(p.engine, p.inner, "width = Qt.binding(function() { return outer.width / 2 })");
  const double after_rebinding = p.inner_width();
  p.set_outer(500.0);
  const double following_again = p.inner_width();
  std::cout << "  outer 300: inner " << followed << "; after \"width = 10\" and outer 400: inner " << after_assignment << "; after Qt.binding: " << after_rebinding
            << "; outer 500: inner " << following_again << "\n\n";
  RC_CHECK_NEAR(followed, 150.0, 1e-12);
  RC_CHECK(assigned.ok);
  RC_CHECK_NEAR(after_assignment, 10.0, 1e-12);          // not 200
  RC_CHECK(rebound.ok);
  RC_CHECK_NEAR(after_rebinding, 200.0, 1e-12);
  RC_CHECK_NEAR(following_again, 250.0, 1e-12);
}

RC_TEST("evaluate runs in the object's own QML and reports errors, never throws, and never reports ok for one") {
  Pair p;
  RC_CHECK(p.inner != nullptr);
  const ProbeResult doubled = evaluate(p.engine, p.inner, "width * 2");
  RC_CHECK(doubled.ok);
  RC_CHECK_NEAR(doubled.value.toDouble(), 200.0, 1e-12);
  const ProbeResult through_id = evaluate(p.engine, p.inner, "outer.height + 1");          // the ids of the file are in scope
  RC_CHECK(through_id.ok);
  RC_CHECK_NEAR(through_id.value.toDouble(), 101.0, 1e-12);

  const ProbeResult missing = evaluate(p.engine, p.inner, "nothingCalledThis.value");
  const ProbeResult syntax = evaluate(p.engine, p.inner, "width * ");
  const ProbeResult thrown = evaluate(p.engine, p.inner, "(function() { throw new Error('boom'); })()");
  const ProbeResult empty = evaluate(p.engine, p.inner, "   ");
  const ProbeResult nobody = evaluate(p.engine, nullptr, "1 + 1");
  std::cout << "  an undefined name: \"" << missing.error.toStdString() << "\"\n  a syntax error: \"" << syntax.error.toStdString() << "\"\n  a throw: \""
            << thrown.error.toStdString() << "\"\n\n";
  RC_CHECK(!missing.ok);
  RC_CHECK(missing.error.contains("nothingCalledThis"));
  RC_CHECK(!syntax.ok);
  RC_CHECK(!syntax.error.isEmpty());
  RC_CHECK(!thrown.ok);
  RC_CHECK(thrown.error.contains("boom"));
  RC_CHECK(!empty.ok);
  RC_CHECK(!nobody.ok);
  RC_CHECK(!missing.value.isValid());
}

RC_TEST("a value that does not fit the property is an error, and the property keeps what it had") {
  Pair p;
  RC_CHECK(p.inner != nullptr);
  const ProbeResult bad = set(p.outer.get(), "width", QVariant(QString("wide")));
  std::cout << "  writing the string \"wide\" to width: ok " << bad.ok << ", width is " << p.outer->property("width").toDouble() << "\n\n";
  RC_CHECK(!bad.ok);
  RC_CHECK_NEAR(p.outer->property("width").toDouble(), 200.0, 1e-12);
  RC_CHECK(set(p.outer.get(), "width", 321.0).ok);                      // and a number that fits is written
  RC_CHECK_NEAR(p.outer->property("width").toDouble(), 321.0, 1e-12);
  const ProbeResult numeric_text = set(p.outer.get(), "width", QVariant(QString("320")));
  RC_CHECK(numeric_text.ok == (p.outer->property("width").toDouble() == 320.0));          // whatever Qt accepts, the probe agrees with what happened
}

int main(int argc, char** argv) {
  qputenv("QT_QPA_PLATFORM", "offscreen");
  QGuiApplication app(argc, argv);
  return rc::test::run_all();
}
