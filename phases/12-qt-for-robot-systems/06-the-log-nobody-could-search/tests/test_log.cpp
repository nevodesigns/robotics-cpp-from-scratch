// The framework's main is suppressed because the logging needs a QCoreApplication to exist before it does.
#define RC_TEST_NO_MAIN
#include <rc/test/rc_test.hpp>

#include <QtCore/QCoreApplication>
#include <QtCore/QLoggingCategory>

#include <iostream>
#include <thread>
#include <vector>

#include "solution.hpp"

Q_LOGGING_CATEGORY(lcLink, "station.link")
Q_LOGGING_CATEGORY(lcPlan, "station.plan")

namespace {

int g_evaluated = 0;
int evaluate() {
  ++g_evaluated;
  return 42;
}

int count_of(const LogRing& ring, const char* category, QtMsgType min = QtDebugMsg) {
  return static_cast<int>(ring.entries(min, QString::fromLatin1(category)).size());
}

}  // namespace

RC_TEST("a disabled category does not print, and does not evaluate what it was going to print") {
  QLoggingCategory::setFilterRules("station.link.debug=false\n");
  LogRing ring(100);
  g_evaluated = 0;
  {
    LogHook hook(ring);
    qCDebug(lcLink) << "link says" << evaluate();
    qCDebug(lcPlan) << "plan says" << evaluate();
  }
  std::cout << "  station.link.debug=false: " << ring.size() << " message kept, the arguments were worked out " << g_evaluated << " time\n";
  RC_CHECK_EQ(ring.size(), 1u);
  RC_CHECK_EQ(g_evaluated, 1);                       // only the plan's: a disabled message costs nothing, and does nothing
  RC_CHECK_EQ(count_of(ring, "station.plan"), 1);
  RC_CHECK_EQ(count_of(ring, "station.link"), 0);
  const std::vector<LogEntry> kept = ring.entries(QtDebugMsg);
  RC_CHECK_EQ(kept.size(), 1u);
  if (kept.size() == 1) {
    RC_CHECK(kept[0].category == "station.plan");
    RC_CHECK(kept[0].text.contains("42"));
    RC_CHECK(kept[0].type == QtDebugMsg);
  }
  QLoggingCategory::setFilterRules("");
}

RC_TEST("rules are applied in order and the last that matches wins") {
  LogRing a(100), b(100);
  QLoggingCategory::setFilterRules("*.debug=false\nstation.link.debug=true\n");
  {
    LogHook hook(a);
    qCDebug(lcLink) << "x";
    qCDebug(lcPlan) << "y";
  }
  QLoggingCategory::setFilterRules("station.link.debug=true\n*.debug=false\n");          // the same two rules, the other way round
  {
    LogHook hook(b);
    qCDebug(lcLink) << "x";
    qCDebug(lcPlan) << "y";
  }
  std::cout << "  \"*.debug=false\" then \"station.link.debug=true\": " << count_of(a, "station.link") << " from link, " << count_of(a, "station.plan") << " from plan; the other way round: "
            << count_of(b, "station.link") << " and " << count_of(b, "station.plan") << "\n\n";
  RC_CHECK_EQ(count_of(a, "station.link"), 1);        // the specific rule came last
  RC_CHECK_EQ(count_of(a, "station.plan"), 0);
  RC_CHECK_EQ(count_of(b, "station.link"), 0);        // the general rule came last and took it back
  RC_CHECK_EQ(count_of(b, "station.plan"), 0);
  QLoggingCategory::setFilterRules("");
}

RC_TEST("the types of message are numbered out of order of how bad they are, and severity puts them right") {
  std::cout << "  QtDebugMsg " << QtDebugMsg << ", QtWarningMsg " << QtWarningMsg << ", QtCriticalMsg " << QtCriticalMsg << ", QtFatalMsg " << QtFatalMsg << ", QtInfoMsg " << QtInfoMsg << "\n\n";
  RC_CHECK(static_cast<int>(QtInfoMsg) > static_cast<int>(QtWarningMsg));          // the trap: information is numbered above a warning
  RC_CHECK(static_cast<int>(QtInfoMsg) > static_cast<int>(QtFatalMsg));
  RC_CHECK(severity(QtDebugMsg) < severity(QtInfoMsg));
  RC_CHECK(severity(QtInfoMsg) < severity(QtWarningMsg));
  RC_CHECK(severity(QtWarningMsg) < severity(QtCriticalMsg));
  RC_CHECK(severity(QtCriticalMsg) < severity(QtFatalMsg));
  RC_CHECK_EQ(severity(QtDebugMsg), 0);
  RC_CHECK_EQ(severity(QtFatalMsg), 4);

  LogRing ring(100);
  ring.push(LogEntry{QtDebugMsg, "a", "d"});
  ring.push(LogEntry{QtInfoMsg, "a", "i"});
  ring.push(LogEntry{QtWarningMsg, "a", "w"});
  ring.push(LogEntry{QtCriticalMsg, "a", "c"});
  const int warnings_or_worse = static_cast<int>(ring.entries(QtWarningMsg).size());
  int naive = 0;
  for (const LogEntry& e : ring.entries(QtDebugMsg))
    if (e.type >= QtWarningMsg) ++naive;                                           // what a filter written with >= gives
  std::cout << "  of a debug, an info, a warning and a critical, at least a warning: " << warnings_or_worse << "; with >= on the numbers: " << naive << "\n\n";
  RC_CHECK_EQ(warnings_or_worse, 2);                 // the warning and the critical, and not the information
  RC_CHECK_EQ(naive, 3);                             // the information got in
  RC_CHECK_EQ(static_cast<int>(ring.entries(QtInfoMsg).size()), 3);
  RC_CHECK_EQ(static_cast<int>(ring.entries(QtCriticalMsg).size()), 1);
  RC_CHECK_EQ(static_cast<int>(ring.entries(QtFatalMsg).size()), 0);
}

RC_TEST("the ring keeps the last messages, oldest first, and counts what it let go") {
  LogRing ring(1000);
  for (int i = 0; i < 5000; ++i) ring.push(LogEntry{QtWarningMsg, "station.link", QString::number(i)});
  std::cout << "  5000 messages into a ring of 1000: kept " << ring.size() << ", dropped " << ring.dropped() << "\n";
  RC_CHECK_EQ(ring.size(), 1000u);
  RC_CHECK_EQ(ring.dropped(), 4000u);
  RC_CHECK_EQ(ring.capacity(), 1000u);
  const std::vector<LogEntry> all = ring.entries(QtDebugMsg);
  RC_CHECK_EQ(all.size(), 1000u);
  if (all.size() == 1000) {
    RC_CHECK(all.front().text == "4000");            // the oldest that is left
    RC_CHECK(all.back().text == "4999");             // the newest
    bool in_order = true;
    for (std::size_t i = 1; i < all.size(); ++i)
      if (all[i].text.toInt() != all[i - 1].text.toInt() + 1) in_order = false;
    RC_CHECK(in_order);
  }
  LogRing tiny(0);                                   // a capacity of nothing is one
  tiny.push(LogEntry{QtWarningMsg, "a", "1"});
  tiny.push(LogEntry{QtWarningMsg, "a", "2"});
  RC_CHECK_EQ(tiny.size(), 1u);
  RC_CHECK_EQ(tiny.dropped(), 1u);
}

RC_TEST("a query by category prefix and severity, and messages from other threads all arrive with their category") {
  QLoggingCategory::setFilterRules("");
  LogRing ring(100000);
  {
    LogHook hook(ring);
    qCWarning(lcLink) << "link warning";
    qCCritical(lcLink) << "link critical";
    qCWarning(lcPlan) << "plan warning";
    qCInfo(lcPlan) << "plan info";
    std::vector<std::thread> workers;
    for (int t = 0; t < 4; ++t)
      workers.emplace_back([] {
        for (int i = 0; i < 500; ++i) qCWarning(lcLink) << "from a thread";
      });
    for (std::thread& w : workers) w.join();
  }
  const int link_all = count_of(ring, "station.link");
  const int link_critical = count_of(ring, "station.link", QtCriticalMsg);
  const int station = count_of(ring, "station");
  const int plan_warnings = count_of(ring, "station.plan", QtWarningMsg);
  std::cout << "  station.link: " << link_all << " messages, " << link_critical << " critical; station.plan at warning or worse: " << plan_warnings << "; every station message: " << station << "\n\n";
  RC_CHECK_EQ(link_all, 2002);
  RC_CHECK_EQ(link_critical, 1);
  RC_CHECK_EQ(plan_warnings, 1);                      // the information is not one
  RC_CHECK_EQ(station, 2004);                         // a prefix of "station" takes both parts
  RC_CHECK_EQ(count_of(ring, "nothing"), 0);
}

int main(int argc, char** argv) {
  qputenv("QT_QPA_PLATFORM", "offscreen");
  QCoreApplication app(argc, argv);
  return rc::test::run_all();
}
