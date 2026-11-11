#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <QCoreApplication>
#include <QObject>
#include <QString>
#include <QStringList>

#include <deque>
#include <functional>
#include <utility>

// A handler for readings that does some work for each, and in the middle of that work lets the event loop run so that the program stays
// responsive. Many handlers do. It is a trap, and the trap is that the loop delivers the NEXT reading to this same handler, from inside
// the handler, before it has finished with the first.
class Controller : public QObject {
  Q_OBJECT

 public:
  explicit Controller(bool guarded, QObject* parent = nullptr) : QObject(parent), guarded_(guarded) {}

  // What happened, in order: "begin 1", "end 1" and so on.
  QStringList log() const { return log_; }
  // The deepest the handler was nested inside itself.
  int max_depth() const { return max_depth_; }
  // How many times the reading in progress was found changed when the handler came back to it.
  int corrupted() const { return corrupted_; }
  // How many readings were handled.
  int handled() const { return handled_; }

  // Something that happens while a reading is being worked on, with that reading's value. In a real program it is the world: the
  // next readings arrive from the sensor while this one is in progress. A test sets it to post them.
  void set_while_working(std::function<void(int)> hook) { while_working_ = std::move(hook); }

 public slots:
  void on_reading(int value) {
    if (!guarded_) {
      process(value);                      // straight in, however deep that makes it
      return;
    }

    // Guarded: while one is being handled, the others wait in a line, and the first call works through the line.
    pending_.push_back(value);
    if (running_) return;
    running_ = true;
    while (!pending_.empty()) {
      const int next = pending_.front();
      pending_.pop_front();
      process(next);
    }
    running_ = false;
  }

 private:
  // The work for one reading. It keeps the reading it is on in a member, which is exactly what a handler with state does, lets the loop
  // run, and then checks that the reading it was on is still the one.
  void process(int value) {
    ++depth_;
    if (depth_ > max_depth_) max_depth_ = depth_;
    log_.push_back(QStringLiteral("begin %1").arg(value));

    current_ = value;
    if (while_working_) while_working_(value);
    QCoreApplication::processEvents();     // "stay responsive"
    if (current_ != value) ++corrupted_;   // somebody else's reading was written over ours

    log_.push_back(QStringLiteral("end %1").arg(value));
    ++handled_;
    --depth_;
  }

  bool guarded_;
  std::function<void(int)> while_working_;
  bool running_ = false;
  std::deque<int> pending_;
  QStringList log_;
  int depth_ = 0;
  int max_depth_ = 0;
  int current_ = 0;
  int corrupted_ = 0;
  int handled_ = 0;
};

#endif  // LESSON_SOLUTION_HPP
