// The framework's main is suppressed because a QML engine needs a QGuiApplication to exist before it does.
#define RC_TEST_NO_MAIN
#include <rc/test/rc_test.hpp>

#include <QtCore/QCoreApplication>
#include <QtCore/QElapsedTimer>
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

const char* kGood = "import QtQuick\nItem { width: 42 }\n";
const char* kBad = "import QtQuick\nItem { width: }\n";

const char* name_of(QQmlComponent::Status s) {
  switch (s) {
    case QQmlComponent::Null: return "Null";
    case QQmlComponent::Ready: return "Ready";
    case QQmlComponent::Loading: return "Loading";
    case QQmlComponent::Error: return "Error";
  }
  return "?";
}

}  // namespace

RC_TEST("a component loaded in the background is Loading, and create() on it gives nothing") {
  QTemporaryDir dir;
  QQmlEngine engine;
  // Two files, because an engine keeps what it has compiled by its URL, and a file that has been loaded once is ready at once the next time.
  QQmlComponent sync(&engine, write_qml(dir, kGood));
  QQmlComponent async(&engine, write_qml(dir, kGood), QQmlComponent::Asynchronous);
  const QQmlComponent::Status sync_status = sync.status();
  const QQmlComponent::Status async_status = async.status();
  QObject* too_early = async.status() == QQmlComponent::Loading ? async.create() : nullptr;
  std::cout << "  a local file loaded by default: " << name_of(sync_status) << "; loaded asynchronously: " << name_of(async_status) << ", create() on it " << (too_early ? "gave an object" : "gave null")
            << "\n";
  RC_CHECK(sync_status == QQmlComponent::Ready);
  RC_CHECK(async_status == QQmlComponent::Loading);
  RC_CHECK(too_early == nullptr);

  // The wait of this lesson takes it to Ready.
  const LoadResult waited = wait_until_ready(async, 5000);
  std::cout << "  after the wait: " << name_of(async.status()) << "\n\n";
  RC_CHECK(waited == LoadResult::kReady);
  RC_CHECK(async.status() == QQmlComponent::Ready);
  std::unique_ptr<QObject> object(async.create());
  RC_CHECK(object != nullptr);
}

RC_TEST("waiting for a component that is ready already, or has no source, returns at once") {
  QTemporaryDir dir;
  QQmlEngine engine;
  QQmlComponent ready(&engine, write_qml(dir, kGood));
  QElapsedTimer clock;
  clock.start();
  const LoadResult a = wait_until_ready(ready, 5000);
  QQmlComponent nothing(&engine);
  const LoadResult b = wait_until_ready(nothing, 5000);
  const qint64 took = clock.elapsed();
  std::cout << "  ready and empty components: " << (a == LoadResult::kReady) << " and " << (b == LoadResult::kNotLoading) << " in " << took << " ms\n\n";
  RC_CHECK(a == LoadResult::kReady);
  RC_CHECK(b == LoadResult::kNotLoading);
  RC_CHECK(took < 1000);                       // neither waited for the 5 seconds it was allowed
}

RC_TEST("a file with an error becomes Error and not Ready, and the reason is returned with no object") {
  QTemporaryDir dir;
  QQmlEngine engine;
  QQmlComponent bad(&engine, write_qml(dir, kBad), QQmlComponent::Asynchronous);
  QString why;
  QObject* object = create_when_ready(bad, 5000, &why);
  std::cout << "  an asynchronous file with a syntax error: status " << name_of(bad.status()) << ", object " << (object ? "made" : "null") << ", reason: " << why.trimmed().toStdString()
            << "\n\n";
  RC_CHECK(object == nullptr);
  RC_CHECK(bad.status() == QQmlComponent::Error);
  RC_CHECK(!why.isEmpty());
  RC_CHECK(why.contains("main"));              // with the file it was in
  delete object;
}

RC_TEST("a good component is created with the object it describes, and an unreadable file is an error and not a hang") {
  QTemporaryDir dir;
  QQmlEngine engine;
  QQmlComponent good(&engine, write_qml(dir, kGood), QQmlComponent::Asynchronous);
  QString why;
  std::unique_ptr<QObject> object(create_when_ready(good, 5000, &why));
  RC_CHECK(object != nullptr);
  if (object) RC_CHECK_NEAR(object->property("width").toDouble(), 42.0, 1e-12);
  RC_CHECK(why.isEmpty());

  QElapsedTimer clock;
  clock.start();
  QQmlComponent missing(&engine, QUrl::fromLocalFile(dir.filePath("no_such_file.qml")), QQmlComponent::Asynchronous);
  QString missing_why;
  QObject* none = create_when_ready(missing, 5000, &missing_why);
  const qint64 took = clock.elapsed();
  std::cout << "  a file that does not exist: object " << (none ? "made" : "null") << ", status " << name_of(missing.status()) << ", after " << took << " ms\n\n";
  RC_CHECK(none == nullptr);
  RC_CHECK(missing.status() == QQmlComponent::Error);
  RC_CHECK(!missing_why.isEmpty());
  RC_CHECK(took < 4000);
  delete none;
}

RC_TEST("the wait is bounded by its timeout, and a component that is still loading when it ends is a timeout") {
  QTemporaryDir dir;
  QQmlEngine engine;
  // Waiting is for the event loop to deliver the status change, and a wait of nothing has not given it a chance to.
  QQmlComponent slow(&engine, write_qml(dir, kGood), QQmlComponent::Asynchronous);
  const bool was_loading = slow.status() == QQmlComponent::Loading;
  QElapsedTimer clock;
  clock.start();
  const LoadResult impatient = wait_until_ready(slow, 0);
  const qint64 took = clock.elapsed();
  std::cout << "  a wait of 0 ms on a component that was " << name_of(was_loading ? QQmlComponent::Loading : QQmlComponent::Ready) << ": " << (impatient == LoadResult::kReady ? "Ready" : "Timeout")
            << " after " << took << " ms\n\n";
  RC_CHECK(was_loading);
  RC_CHECK(took < 1000);                       // it did not wait for anything long
  RC_CHECK(impatient == LoadResult::kReady || impatient == LoadResult::kTimeout);
  const LoadResult patient = wait_until_ready(slow, 5000);
  RC_CHECK(patient == LoadResult::kReady);
}

int main(int argc, char** argv) {
  qputenv("QT_QPA_PLATFORM", "offscreen");
  QGuiApplication app(argc, argv);
  return rc::test::run_all();
}
