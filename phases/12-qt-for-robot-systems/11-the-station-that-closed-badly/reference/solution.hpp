#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <QtCore/QCoreApplication>
#include <QtCore/QEvent>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <exception>
#include <functional>
#include <string>
#include <utility>
#include <vector>

// A station starts things in an order: the settings are read, the log is opened, the link is made, the monitor of the heartbeat is started, a plan is running. Each depends on what was
// started before it. Closing them in any other order closes a thing that something still running is using: the last message of the plan goes to a log that is closed, the settings are saved
// by a window that was already destroyed, the link is closed under a thread that was reading it.
//
// So a program that starts things should be able to say how to close each as it starts it, and close them all in the reverse order when it ends. And three things about doing that:
// a step that fails must not stop the ones after it, because the one after it may be the one that stops the robot; doing it twice must not do anything the second time, because a
// program is asked to quit more than once; and an object that was told deleteLater() is not deleted if the loop that would have deleted it does not run again.

struct ShutdownReport {
  QStringList order;           // the names of the steps, in the order they were run
  QStringList failed;          // the steps that returned false or threw, with a reason for those that threw
  bool ok() const { return failed.isEmpty(); }
};

class ShutdownSequence {
 public:
  // Records how to close something that has just been started. The step returns true if it closed cleanly.
  void add(const QString& name, std::function<bool()> step) { steps_.emplace_back(name, std::move(step)); }

  std::size_t size() const { return steps_.size(); }
  bool done() const { return done_; }

  // Closes everything that was added, the last first. A step that returns false, or throws, is recorded and the rest still run. The second and every later call does nothing and
  // returns an empty report.
  ShutdownReport run() {
    ShutdownReport report;
    if (done_) return report;
    done_ = true;
    for (std::size_t i = steps_.size(); i > 0; --i) {
      const std::pair<QString, std::function<bool()>>& step = steps_[i - 1];
      report.order.append(step.first);
      try {
        if (!step.second()) report.failed.append(step.first);
      } catch (const std::exception& e) {
        report.failed.append(step.first + QStringLiteral(": threw ") + QString::fromUtf8(e.what()));
      } catch (...) {
        report.failed.append(step.first + QStringLiteral(": threw something that is not an exception"));
      }
    }
    return report;
  }

 private:
  std::vector<std::pair<QString, std::function<bool()>>> steps_;
  bool done_ = false;
};

// Deletes the objects that were told deleteLater() and whose turn has not come, which an event loop that is not running again will not do. A program that is leaving calls this
// as one of its last steps.
inline void flush_deferred_deletes() { QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete); }

#endif  // LESSON_SOLUTION_HPP
