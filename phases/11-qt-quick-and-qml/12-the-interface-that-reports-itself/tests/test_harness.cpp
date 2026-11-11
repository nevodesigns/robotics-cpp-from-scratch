// The framework's main is suppressed because a QML engine needs a QGuiApplication to exist before it does.
#define RC_TEST_NO_MAIN
#include <rc/test/rc_test.hpp>

#include <QtCore/QFile>
#include <QtCore/QTemporaryDir>
#include <QtCore/QTextStream>
#include <QtCore/QUrl>
#include <QtGui/QGuiApplication>
#include <QtQml/QQmlEngine>

#include <iostream>
#include <string>
#include <vector>

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

struct Case {
  const char* what;
  const char* qml;
  const char* expect_kind;
  const char* expect_text;
};

}  // namespace

RC_TEST("a clean file loaded through the harness has no failures, and its object is there to use") {
  QTemporaryDir dir;
  QQmlEngine engine;
  QmlHarness harness(engine);
  QObject* root = harness.load(write_qml(dir, "import QtQuick\nItem { width: 120; height: 40; property string name: \"gauge\" }\n"));
  std::cout << "  a clean file: object " << (root ? "made" : "null") << ", " << harness.failures().size() << " failures\n\n";
  RC_CHECK(root != nullptr);
  RC_CHECK(harness.ok());
  RC_CHECK_EQ(harness.failures().size(), 0u);
  RC_CHECK(harness.report().isEmpty());
  if (root == nullptr) return;
  const rc::qt::ProbeResult width = harness.read(root, "width");
  RC_CHECK(width.ok);
  RC_CHECK_NEAR(width.value.toDouble(), 120.0, 1e-12);
  RC_CHECK(harness.ok());                              // a read that worked is not a failure
}

RC_TEST("each way a file can go wrong is a failure, of the right kind, with its text") {
  const Case cases[] = {
      {"a syntax error", "import QtQuick\nItem { width: }\n", "load", "Expected"},
      {"a type that does not exist", "import QtQuick\nButtn { }\n", "load", "Buttn"},
      {"a property that does not exist", "import QtQuick\nItem { widht: 100 }\n", "load", "widht"},
      {"a required property that was not given", "import QtQuick\nItem { required property real limit }\n", "load", "Required property"},
      {"a handler that throws", "import QtQuick\nItem { Component.onCompleted: nothingCalledThis.value }\n", "warning", "nothingCalledThis"},
      {"a binding to a name that does not exist", "import QtQuick\nItem { width: missingThing.value }\n", "warning", "missingThing"},
      {"a binding loop", "import QtQuick\nItem { property int a: b + 1; property int b: a + 1 }\n", "warning", "inding loop"},
      {"a console.warn", "import QtQuick\nItem { Component.onCompleted: console.warn('careful with this') }\n", "warning", "careful with this"},
  };
  int reported = 0;
  int right_kind = 0;
  int with_text = 0;
  for (const Case& c : cases) {
    QTemporaryDir dir;
    QQmlEngine engine;
    QmlHarness harness(engine);
    harness.load(write_qml(dir, c.qml));
    const std::vector<Failure> failures = harness.failures();
    bool kind = false;
    bool text = false;
    for (const Failure& f : failures) {
      if (f.kind == c.expect_kind) kind = true;
      if (f.text.contains(c.expect_text)) text = true;
    }
    if (!failures.empty()) ++reported;
    if (kind) ++right_kind;
    if (text) ++with_text;
    std::cout << "  " << (failures.empty() ? "SILENT " : "failure") << "  " << c.what << (failures.empty() ? "" : ": ") << (failures.empty() ? "" : failures.front().kind.toStdString())
              << "\n";
    RC_CHECK(!failures.empty());
    RC_CHECK(!harness.ok());
  }
  std::cout << "  " << reported << " of 8 reported, " << right_kind << " of the right kind, " << with_text << " with the text that names the mistake\n\n";
  RC_CHECK_EQ(reported, 8);
  RC_CHECK_EQ(right_kind, 8);
  RC_CHECK_EQ(with_text, 8);
}

RC_TEST("an initial property that is not one is a load failure that names it, and a good one is used") {
  QTemporaryDir dir;
  QQmlEngine engine;
  QmlHarness harness(engine);
  const QUrl url = write_qml(dir, "import QtQuick\nItem { property real limit: 1; property real seen: -1; Component.onCompleted: seen = limit }\n");
  QObject* typo = harness.load(url, {{"limt", 5.0}});
  RC_CHECK(typo == nullptr);
  RC_CHECK(!harness.ok());
  RC_CHECK(harness.report().contains("limt"));
  harness.clear();
  RC_CHECK(harness.ok());                              // clear forgets
  QObject* good = harness.load(write_qml(dir, "import QtQuick\nItem { property real limit: 1; property real seen: -1; Component.onCompleted: seen = limit }\n"), {{"limit", 5.0}});
  RC_CHECK(good != nullptr);
  RC_CHECK(harness.ok());
  if (good) RC_CHECK_NEAR(good->property("seen").toDouble(), 5.0, 1e-12);
}

RC_TEST("a read, a write or an expression that is refused is a failure, and one that worked is not") {
  QTemporaryDir dir;
  QQmlEngine engine;
  QmlHarness harness(engine);
  QObject* root = harness.load(write_qml(dir, "import QtQuick\nItem { width: 50; property int count: 3 }\n"));
  RC_CHECK(root != nullptr);
  if (root == nullptr) return;
  RC_CHECK(harness.read(root, "count").ok);
  RC_CHECK(harness.set(root, "count", 4).ok);
  RC_CHECK(harness.evaluate(root, "count + 1").ok);
  RC_CHECK(harness.ok());

  const rc::qt::ProbeResult typo = harness.read(root, "cuont");
  const rc::qt::ProbeResult wrong_set = harness.set(root, "widht", 10.0);
  const rc::qt::ProbeResult bad_expression = harness.evaluate(root, "nothingCalledThis.x");
  RC_CHECK(!typo.ok);
  RC_CHECK(!wrong_set.ok);
  RC_CHECK(!bad_expression.ok);
  const std::vector<Failure> failures = harness.failures();
  std::cout << "  a typo read, a typo write and a bad expression: " << failures.size() << " failures\n";
  int probes = 0;
  for (const Failure& f : failures)
    if (f.kind == "probe") ++probes;
  RC_CHECK(probes >= 3);
  RC_CHECK(harness.report().contains("cuont"));
  RC_CHECK(harness.report().contains("widht"));
}

RC_TEST("failures come in the order they happened, each file adds to them, and the report says one on each line") {
  QTemporaryDir dir;
  QQmlEngine engine;
  QmlHarness harness(engine);
  harness.load(write_qml(dir, "import QtQuick\nItem { width: }\n"));
  QObject* good = harness.load(write_qml(dir, "import QtQuick\nItem { width: 10 }\n"));
  harness.read(good, "nothing");
  harness.load(write_qml(dir, "import QtQuick\nButtn { }\n"));
  const std::vector<Failure> failures = harness.failures();
  std::cout << "  four steps, three of them bad: " << failures.size() << " failures, in order:";
  for (const Failure& f : failures) std::cout << " " << f.kind.toStdString();
  std::cout << "\n\n";
  RC_CHECK_EQ(failures.size(), 3u);
  if (failures.size() == 3) {
    RC_CHECK(failures[0].kind == "load");
    RC_CHECK(failures[1].kind == "probe");
    RC_CHECK(failures[2].kind == "load");
    RC_CHECK(failures[2].text.contains("Buttn"));
  }
  const QString report = harness.report();
  RC_CHECK_EQ(report.count(QLatin1Char('\n')), 3);
  RC_CHECK(report.startsWith("load: "));
}

RC_TEST("what no message says is not a failure, and only asking for the property shows it") {
  QTemporaryDir dir;
  QQmlEngine engine;
  QmlHarness harness(engine);
  QObject* root = harness.load(write_qml(dir, "import QtQuick\nItem { width: 0 / 0; height: 40 }\n"));
  RC_CHECK(root != nullptr);
  if (root == nullptr) return;
  const bool silent = harness.ok();
  const rc::qt::ProbeResult width = harness.read(root, "width");
  std::cout << "  width: 0 / 0 loaded with " << harness.failures().size() << " failures; the width it has is " << width.value.toDouble() << "\n\n";
  RC_CHECK(silent);                                    // nothing was printed and nothing was refused
  RC_CHECK(width.ok);
  RC_CHECK_NEAR(width.value.toDouble(), 0.0, 1e-12);   // and it is not the width anyone meant: a test has to ask
}

int main(int argc, char** argv) {
  qputenv("QT_QPA_PLATFORM", "offscreen");
  QGuiApplication app(argc, argv);
  return rc::test::run_all();
}
