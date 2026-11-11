// The framework's main is suppressed because the event loop needs a QCoreApplication to exist before it does.
#define RC_TEST_NO_MAIN
#include <rc/test/rc_test.hpp>

#include <QtCore/QCoreApplication>
#include <QtCore/QElapsedTimer>
#include <QtCore/QEventLoop>
#include <QtCore/QThread>
#include <QtCore/QTimer>

#include <functional>
#include <iostream>
#include <vector>

#include "solution.hpp"

namespace {

bool wait_for(const std::function<bool()>& condition, int ms = 2000) {
  QEventLoop loop;
  QTimer poll;
  QTimer limit;
  limit.setSingleShot(true);
  QObject::connect(&poll, &QTimer::timeout, &loop, [&] {
    if (condition()) loop.quit();
  });
  QObject::connect(&limit, &QTimer::timeout, &loop, &QEventLoop::quit);
  poll.start(5);
  limit.start(ms);
  loop.exec();
  return condition();
}

void pump(int ms) {
  QEventLoop loop;
  QTimer::singleShot(ms, &loop, &QEventLoop::quit);
  loop.exec();
}

// The largest gap, in ms, between consecutive ticks of a 10 ms timer that is running while `during` is done 30 ms in. The loop runs for `total_ms`.
qint64 largest_gap(const std::function<void()>& during, int total_ms) {
  QElapsedTimer clock;
  std::vector<qint64> ticks;
  QTimer ticker;
  QObject::connect(&ticker, &QTimer::timeout, [&] { ticks.push_back(clock.elapsed()); });
  QEventLoop loop;
  QTimer::singleShot(30, [&] { during(); });
  QTimer::singleShot(total_ms, &loop, &QEventLoop::quit);
  clock.start();
  ticker.start(10);
  loop.exec();
  qint64 worst = 0;
  for (std::size_t i = 1; i < ticks.size(); ++i) worst = std::max(worst, ticks[i] - ticks[i - 1]);
  return worst;
}

}  // namespace

RC_TEST("a plan worked out in a handler stops a clock that ticks every 10 ms, and the same plan in the runner does not") {
  const qint64 blocked = largest_gap([] {
    QThread::msleep(200);                              // the plan, on the thread that runs the interface
  }, 500);

  PlanRunner runner;
  int results = 0;
  int result = -1;
  QObject::connect(&runner, &PlanRunner::finished, [&](int r) {
    ++results;
    result = r;
  });
  const qint64 free = largest_gap([&] { runner.start(20, 10); }, 500);
  std::cout << "  a 200 ms plan in a handler: the largest gap between ticks of a 10 ms timer was " << blocked << " ms; the same plan, 20 steps of 10 ms, in the runner: " << free
            << " ms, and " << results << " result, " << result << "\n\n";
  RC_CHECK(blocked >= 180);                            // the clock stopped for the plan
  RC_CHECK(free < 100);                                // not 200: the interface went on
  RC_CHECK_EQ(results, 1);
  RC_CHECK_EQ(result, 20);
  RC_CHECK_EQ(runner.finished_count(), 1);
  RC_CHECK(!runner.running());
}

RC_TEST("the result and the progress arrive on the thread that asked, and a second plan is refused while one is running") {
  PlanRunner runner;
  QThread* const owner = QThread::currentThread();
  QThread* result_on = nullptr;
  QThread* progress_on = nullptr;
  std::vector<int> progress;
  QObject::connect(&runner, &PlanRunner::finished, [&](int) { result_on = QThread::currentThread(); });
  QObject::connect(&runner, &PlanRunner::progress, [&](int p) {
    progress_on = QThread::currentThread();
    progress.push_back(p);
  });
  RC_CHECK(runner.start(20, 10));
  RC_CHECK(runner.running());
  RC_CHECK(!runner.start(5, 5));                       // refused, and the plan that is running is not disturbed
  RC_CHECK(wait_for([&] { return runner.finished_count() == 1; }));
  pump(20);
  std::cout << "  the result arrived on " << (result_on == owner ? "the thread that asked" : "another thread") << ", progress on " << (progress_on == owner ? "the same" : "another") << "; "
            << progress.size() << " progress values, the last " << (progress.empty() ? -1 : progress.back()) << "\n";
  RC_CHECK(result_on == owner);
  RC_CHECK(progress_on == owner);
  RC_CHECK(!progress.empty());
  if (!progress.empty()) RC_CHECK_EQ(progress.back(), 20);
  for (std::size_t i = 1; i < progress.size(); ++i) RC_CHECK(progress[i] > progress[i - 1]);       // in order, each a step further
  RC_CHECK(!runner.running());
  RC_CHECK(runner.start(3, 1));                        // free again
  RC_CHECK(wait_for([&] { return runner.finished_count() == 2; }));
}

RC_TEST("cancel stops a plan of a second long before the end, and says it was cancelled and not finished") {
  PlanRunner runner;
  int finished = 0;
  int canceled = 0;
  int last_progress = 0;
  QObject::connect(&runner, &PlanRunner::finished, [&](int) { ++finished; });
  QObject::connect(&runner, &PlanRunner::canceled, [&] { ++canceled; });
  QObject::connect(&runner, &PlanRunner::progress, [&](int p) { last_progress = p; });
  QElapsedTimer clock;
  clock.start();
  RC_CHECK(runner.start(100, 10));                     // a second of work
  pump(50);
  runner.cancel();
  const bool stopped = wait_for([&] { return canceled > 0; });
  const qint64 took = clock.elapsed();
  pump(30);
  std::cout << "  a plan of a second, cancelled after 50 ms: cancelled " << canceled << ", finished " << finished << ", it got to step " << last_progress << ", over after " << took << " ms\n\n";
  RC_CHECK(stopped);
  RC_CHECK_EQ(canceled, 1);
  RC_CHECK_EQ(finished, 0);
  RC_CHECK(last_progress < 100);
  RC_CHECK(took < 500);
  RC_CHECK_EQ(runner.canceled_count(), 1);
  RC_CHECK(!runner.running());
}

RC_TEST("a runner that is destroyed while its plan is running waits for it, and nothing is delivered after") {
  int delivered = 0;
  {
    PlanRunner runner;
    QObject::connect(&runner, &PlanRunner::finished, [&](int) { ++delivered; });
    QObject::connect(&runner, &PlanRunner::canceled, [&] { ++delivered; });
    RC_CHECK(runner.start(100, 10));
    pump(30);
  }                                                    // destroyed with a second of work to do
  pump(100);
  std::cout << "  a runner destroyed 30 ms into a plan of a second: " << delivered << " deliveries afterwards\n\n";
  RC_CHECK_EQ(delivered, 0);
}

int main(int argc, char** argv) {
  qputenv("QT_QPA_PLATFORM", "offscreen");
  QCoreApplication app(argc, argv);
  return rc::test::run_all();
}
