#define RC_TEST_NO_MAIN
#include <rc/test/rc_test.hpp>

#include <QCoreApplication>
#include <QMetaObject>
#include <QString>

#include <iostream>
#include <string>

#include "solution.hpp"

namespace {

// One reading arrives. While it is being worked on, two more arrive from the sensor, as they would in a real program.
void deliver_three(Controller& c) {
  c.set_while_working([&c](int value) {
    if (value == 1) {
      QMetaObject::invokeMethod(&c, "on_reading", Qt::QueuedConnection, Q_ARG(int, 2));
      QMetaObject::invokeMethod(&c, "on_reading", Qt::QueuedConnection, Q_ARG(int, 3));
    }
  });
  QMetaObject::invokeMethod(&c, "on_reading", Qt::QueuedConnection, Q_ARG(int, 1));
  QCoreApplication::processEvents();
  QCoreApplication::processEvents();            // anything left over
}

std::string joined(const QStringList& log) { return log.join(", ").toStdString(); }

}  // namespace

RC_TEST("a handler that lets the loop run is entered again by the readings that arrive while it works, from inside itself") {
  Controller naive(false);
  deliver_three(naive);
  std::cout << "  the reading 1, with 2 and 3 arriving while it works, handled by a handler that runs the event loop in the middle:\n    " << joined(naive.log()) << "\n"
            << "    deepest nesting " << naive.max_depth() << ", readings found overwritten " << naive.corrupted() << "\n\n";
  RC_CHECK_EQ(naive.handled(), 3);
  // 2 and 3 are delivered from inside 1, so 1 is the first to begin and the last to end. How deep it goes depends on the version of Qt: on
  // 6.2 and 6.4 the third reading waited until the second had finished (a depth of 2), and on 6.5.3 it was delivered from inside the second
  // (a depth of 3). So the test asks for what holds in both: nested, not sequential, and something was overwritten.
  const QStringList log = naive.log();
  RC_CHECK_EQ(log.size(), 6);
  if (log.size() == 6) {
    RC_CHECK_EQ(log.first().toStdString(), std::string("begin 1"));
    RC_CHECK_EQ(log.at(1).toStdString(), std::string("begin 2"));            // 2 began before 1 ended: it is nested
    RC_CHECK_EQ(log.last().toStdString(), std::string("end 1"));
  }
  RC_CHECK(naive.max_depth() >= 2);
  RC_CHECK(naive.max_depth() <= 3);
  RC_CHECK(naive.corrupted() >= 1);             // 1 came back to find the member holding another reading's value

  // The guarded one handles the same three, which is what makes the comparison fair.
  Controller guarded(true);
  deliver_three(guarded);
  RC_CHECK_EQ(guarded.handled(), 3);
}

RC_TEST("with a guard the readings are handled one after another, in order, and nothing is overwritten") {
  Controller guarded(true);
  deliver_three(guarded);
  std::cout << "  the same, with a guard:\n    " << joined(guarded.log()) << "\n"
            << "    deepest nesting " << guarded.max_depth() << ", readings found overwritten " << guarded.corrupted() << "\n\n";
  RC_CHECK_EQ(guarded.handled(), 3);
  RC_CHECK_EQ(joined(guarded.log()), std::string("begin 1, end 1, begin 2, end 2, begin 3, end 3"));
  RC_CHECK_EQ(guarded.max_depth(), 1);
  RC_CHECK_EQ(guarded.corrupted(), 0);
}

RC_TEST("a single reading is handled the same either way") {
  Controller naive(false);
  Controller guarded(true);
  QMetaObject::invokeMethod(&naive, "on_reading", Qt::QueuedConnection, Q_ARG(int, 7));
  QMetaObject::invokeMethod(&guarded, "on_reading", Qt::QueuedConnection, Q_ARG(int, 7));
  QCoreApplication::processEvents();
  RC_CHECK_EQ(joined(naive.log()), std::string("begin 7, end 7"));
  RC_CHECK_EQ(joined(guarded.log()), std::string("begin 7, end 7"));
  RC_CHECK_EQ(naive.corrupted(), 0);
  RC_CHECK_EQ(guarded.corrupted(), 0);
}

RC_TEST("a long line of readings is still one at a time, and none is lost or handled twice") {
  Controller guarded(true);
  const int n = 200;
  for (int v = 1; v <= n; ++v) QMetaObject::invokeMethod(&guarded, "on_reading", Qt::QueuedConnection, Q_ARG(int, v));
  for (int i = 0; i < 5; ++i) QCoreApplication::processEvents();

  std::cout << "  " << n << " queued readings, guarded: handled " << guarded.handled() << ", deepest nesting " << guarded.max_depth() << ", overwritten "
            << guarded.corrupted() << "\n";
  RC_CHECK_EQ(guarded.handled(), n);
  RC_CHECK_EQ(guarded.max_depth(), 1);
  RC_CHECK_EQ(guarded.corrupted(), 0);
  RC_CHECK_EQ(guarded.log().size(), 2 * n);
  // In order: the first is begin 1 and the last is end 200.
  RC_CHECK(!guarded.log().isEmpty());
  if (guarded.log().isEmpty()) return;
  RC_CHECK_EQ(guarded.log().first().toStdString(), std::string("begin 1"));
  RC_CHECK_EQ(guarded.log().last().toStdString(), std::string("end 200"));
}

int main(int argc, char** argv) {
  qputenv("QT_QPA_PLATFORM", "offscreen");
  QCoreApplication app(argc, argv);
  return rc::test::run_all();
}
