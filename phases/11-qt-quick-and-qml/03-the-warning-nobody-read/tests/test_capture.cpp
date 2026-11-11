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

#include <atomic>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

#include "solution.hpp"

namespace {

QUrl write_qml(QTemporaryDir& dir, const QString& name, const QString& body) {
  const QString path = dir.filePath(name);
  QFile file(path);
  if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    QTextStream out(&file);
    out << body;
  }
  return QUrl::fromLocalFile(path);
}

// Loads a file and creates its root, so that handlers on completion run.
void load(QQmlEngine& engine, const QUrl& url) {
  QQmlComponent component(&engine, url);
  std::unique_ptr<QObject> root(component.create());
}

// A handler of our own, to tell that the old one is back.
std::atomic<int> g_seen{0};
void counting_handler(QtMsgType, const QMessageLogContext&, const QString&) { ++g_seen; }

}  // namespace

RC_TEST("a QML handler that throws is a warning, and without a scope nothing in a test sees it") {
  QTemporaryDir dir;
  const QUrl url = write_qml(dir, "throws.qml", "import QtQuick\nItem { Component.onCompleted: nothingCalledThis.value }\n");
  QQmlEngine engine;
  {
    MessageCapture capture;
    load(engine, url);
    std::cout << "  a handler that reads a name that does not exist: " << capture.count(QtWarningMsg) << " warning kept";
    if (capture.size() > 0) std::cout << ": \"" << capture.messages().front().text.toStdString() << "\"";
    std::cout << "\n\n";
    RC_CHECK_EQ(capture.size(), 1u);
    RC_CHECK_EQ(capture.count(QtWarningMsg), 1u);
    RC_CHECK(capture.contains("nothingCalledThis"));
    RC_CHECK(capture.contains("ReferenceError"));
  }
}

RC_TEST("console.log, console.warn and qWarning from C++ each arrive with their own type") {
  QTemporaryDir dir;
  const QUrl url = write_qml(dir, "logs.qml",
                             "import QtQuick\nItem { Component.onCompleted: { console.log('hello'); console.warn('careful'); } }\n");
  QQmlEngine engine;
  MessageCapture capture;
  load(engine, url);
  qWarning("from C++: %d", 7);
  qInfo("an info");
  qDebug("a debug");
  const std::vector<CapturedMessage> all = capture.messages();
  std::cout << "  " << all.size() << " messages:";
  for (const CapturedMessage& m : all) std::cout << " [" << static_cast<int>(m.type) << "] " << m.text.toStdString() << ";";
  std::cout << "\n\n";
  RC_CHECK(capture.contains("hello"));
  RC_CHECK(capture.contains("careful"));
  RC_CHECK(capture.contains("from C++: 7"));
  RC_CHECK(capture.contains("an info"));
  RC_CHECK(capture.contains("a debug"));
  RC_CHECK(capture.size() >= 5);
  RC_CHECK(capture.count(QtWarningMsg) >= 2);          // the careful one and our own
  RC_CHECK_EQ(capture.count(QtInfoMsg), 1u);
  bool saw_location = false;
  for (const CapturedMessage& m : all)
    if (m.text.contains("from C++") && m.line > 0 && !m.file.isEmpty()) saw_location = true;
  RC_CHECK(saw_location);                              // C++ messages carry the file and line they came from
}

RC_TEST("the old handler is put back when the scope ends, and a nested scope hands back to the one around it") {
  qInstallMessageHandler(&counting_handler);
  g_seen = 0;
  qWarning("before");
  RC_CHECK_EQ(g_seen.load(), 1);
  {
    MessageCapture outer;
    qWarning("one");
    {
      MessageCapture inner;
      qWarning("two");
      qWarning("three");
      RC_CHECK_EQ(inner.size(), 2u);
    }
    qWarning("four");
    std::cout << "  outer kept " << outer.size() << " messages, the old handler saw " << g_seen.load() << " during the scopes\n";
    RC_CHECK_EQ(outer.size(), 2u);                     // one and four, and not two or three
    RC_CHECK_EQ(g_seen.load(), 1);                     // nothing reached the old handler: the scope swallowed it
  }
  qWarning("after");
  std::cout << "  after the scopes the old handler has seen " << g_seen.load() << "\n\n";
  RC_CHECK_EQ(g_seen.load(), 2);                       // before and after: it is back
  qInstallMessageHandler(nullptr);
}

RC_TEST("a scope that forwards keeps the messages and still shows them, and clear forgets them") {
  g_seen = 0;
  qInstallMessageHandler(&counting_handler);
  {
    MessageCapture capture(true);
    qWarning("a");
    qWarning("b");
    RC_CHECK_EQ(capture.size(), 2u);
    RC_CHECK_EQ(g_seen.load(), 2);                     // both kept and passed on
    capture.clear();
    RC_CHECK_EQ(capture.size(), 0u);
    RC_CHECK(!capture.contains("a"));
    qWarning("c");
    RC_CHECK_EQ(capture.size(), 1u);
  }
  qInstallMessageHandler(nullptr);
}

RC_TEST("messages printed on other threads are all kept") {
  MessageCapture capture;
  const int threads = 8;
  const int each = 1000;
  std::vector<std::thread> pool;
  for (int t = 0; t < threads; ++t)
    pool.emplace_back([] {
      for (int i = 0; i < each; ++i) qWarning("worker message");
    });
  for (std::thread& t : pool) t.join();
  std::cout << "  " << threads << " threads printing " << each << " each: " << capture.size() << " kept\n\n";
  RC_CHECK_EQ(capture.size(), static_cast<std::size_t>(threads * each));
  RC_CHECK_EQ(capture.count(QtWarningMsg), static_cast<std::size_t>(threads * each));
}

int main(int argc, char** argv) {
  qputenv("QT_QPA_PLATFORM", "offscreen");
  QGuiApplication app(argc, argv);
  return rc::test::run_all();
}
