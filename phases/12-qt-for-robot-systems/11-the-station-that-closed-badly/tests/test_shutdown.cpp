// The framework's main is suppressed because deleteLater needs a QCoreApplication to exist before it does.
#define RC_TEST_NO_MAIN
#include <rc/test/rc_test.hpp>

#include <QtCore/QCoreApplication>
#include <QtCore/QObject>
#include <QtCore/QPointer>

#include <iostream>
#include <stdexcept>

#include "solution.hpp"

namespace {

int g_alive = 0;

class Tracked : public QObject {
 public:
  Tracked() { ++g_alive; }
  ~Tracked() override { --g_alive; }
};

}  // namespace

RC_TEST("things are closed in the reverse of the order they were added, so that each is closed before what it used") {
  QStringList log;
  ShutdownSequence s;
  s.add("settings", [&] { log.append("settings"); return true; });
  s.add("log", [&] { log.append("log"); return true; });
  s.add("link", [&] { log.append("link"); return true; });
  s.add("monitor", [&] { log.append("monitor"); return true; });
  RC_CHECK_EQ(s.size(), 4u);
  const ShutdownReport report = s.run();
  std::cout << "  added settings, log, link, monitor; closed:";
  for (const QString& n : report.order) std::cout << " " << n.toStdString();
  std::cout << "\n";
  RC_CHECK(report.ok());
  RC_CHECK_EQ(report.order.size(), 4);
  RC_CHECK((report.order == QStringList() << "monitor" << "link" << "log" << "settings"));
  RC_CHECK((log == report.order));                  // what ran is what the report says ran
  RC_CHECK(s.done());
}

RC_TEST("a step that fails or throws does not stop the ones after it, and the report says which") {
  QStringList log;
  ShutdownSequence s;
  s.add("settings", [&] { log.append("settings"); return true; });
  s.add("link", [&]() -> bool { log.append("link"); throw std::runtime_error("socket busy"); });
  s.add("monitor", [&] { log.append("monitor"); return false; });                    // could not close cleanly
  s.add("planner", [&]() -> bool { log.append("planner"); throw 42; });              // not even an exception
  s.add("robot stop", [&] { log.append("robot stop"); return true; });
  const ShutdownReport report = s.run();
  std::cout << "  five steps, three of them bad: ran " << log.size() << ", failed " << report.failed.size() << ":";
  for (const QString& f : report.failed) std::cout << "\n    " << f.toStdString();
  std::cout << "\n\n";
  RC_CHECK_EQ(log.size(), 5);                       // every step ran, the last ones included
  RC_CHECK((log == QStringList() << "robot stop" << "planner" << "monitor" << "link" << "settings"));
  RC_CHECK(!report.ok());
  RC_CHECK_EQ(report.failed.size(), 3);
  if (report.failed.size() == 3) {
    RC_CHECK(report.failed[0].startsWith("planner"));
    RC_CHECK(report.failed[0].contains("not an exception"));
    RC_CHECK(report.failed[1] == "monitor");
    RC_CHECK(report.failed[2].startsWith("link"));
    RC_CHECK(report.failed[2].contains("socket busy"));
  }
}

RC_TEST("asking again does nothing, and a sequence with nothing added has nothing to do") {
  int runs = 0;
  ShutdownSequence s;
  s.add("only", [&] { ++runs; return true; });
  const ShutdownReport first = s.run();
  const ShutdownReport second = s.run();
  const ShutdownReport third = s.run();
  std::cout << "  three requests to quit: the step ran " << runs << " time; reports of " << first.order.size() << ", " << second.order.size() << " and " << third.order.size() << " steps\n\n";
  RC_CHECK_EQ(runs, 1);
  RC_CHECK_EQ(first.order.size(), 1);
  RC_CHECK_EQ(second.order.size(), 0);
  RC_CHECK_EQ(third.order.size(), 0);
  RC_CHECK(second.ok());

  ShutdownSequence empty;
  RC_CHECK_EQ(empty.run().order.size(), 0);
  RC_CHECK(empty.done());
}

RC_TEST("only what was started is closed: steps added after a failure to start are not in the sequence") {
  QStringList log;
  ShutdownSequence s;
  s.add("settings", [&] { log.append("settings"); return true; });
  s.add("log", [&] { log.append("log"); return true; });
  // The link failed to start, so nothing after it was added, and nothing is closed that was never opened.
  const ShutdownReport report = s.run();
  RC_CHECK((report.order == QStringList() << "log" << "settings"));
  RC_CHECK_EQ(log.size(), 2);
}

RC_TEST("an object told deleteLater is still there when no loop runs again, and drain_deferred_deletes deletes it") {
  g_alive = 0;
  Tracked* object = new Tracked();
  QPointer<QObject> watcher(object);
  object->deleteLater();
  QCoreApplication::processEvents();                // ordinary events are processed, and a deferred delete is not delivered from here
  const bool alive_after_processing = !watcher.isNull();
  drain_deferred_deletes();
  const bool alive_after_drain = !watcher.isNull();
  std::cout << "  after deleteLater and processEvents the object is " << (alive_after_processing ? "still alive" : "gone") << ", after the drain " << (alive_after_drain ? "still alive" : "gone")
            << "; alive count " << g_alive << "\n\n";
  if (alive_after_drain) {
    delete object;                                   // do not leak whatever the outcome
  }
  RC_CHECK(!alive_after_drain);
  RC_CHECK_EQ(g_alive, 0);
}

int main(int argc, char** argv) {
  qputenv("QT_QPA_PLATFORM", "offscreen");
  QCoreApplication app(argc, argv);
  return rc::test::run_all();
}
