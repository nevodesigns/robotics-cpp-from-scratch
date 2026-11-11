#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <QtCore/QLoggingCategory>
#include <QtCore/QString>
#include <QtCore/QtGlobal>
#include <atomic>
#include <cstddef>
#include <deque>
#include <mutex>
#include <vector>

// A station that has been up for a week has something to say about the last ten minutes, and nobody can read a week. The log has to be bounded, so that it does not fill a disk,
// and searchable, so that a person can ask for the messages about the link, and only the warnings.
//
// Qt gives each message a category (`station.link`, `station.plan`) and a type, and gives the program the means to turn categories off by rule. Three things in that are easy to get
// wrong. A disabled message does not evaluate its arguments, which is a saving and a trap. Rules are applied in order, and the last that matches wins. And the types of message
// are numbered in a way that is not the order of how bad they are: information has a larger number than warning, critical and fatal.

struct LogEntry {
  QtMsgType type = QtDebugMsg;
  QString category;
  QString text;
};

// How bad a type of message is: debug 0, info 1, warning 2, critical 3, fatal 4. This is not the order of the numbers of QtMsgType, in which info is 4.
inline int severity(QtMsgType type) {
  switch (type) {
    case QtDebugMsg: return 0;
    case QtInfoMsg: return 1;
    case QtWarningMsg: return 2;
    case QtCriticalMsg: return 3;
    case QtFatalMsg: return 4;
  }
  return 0;
}

// The last `capacity` messages, oldest first. When it is full the oldest is dropped and counted. Safe to use from any thread.
class LogRing {
 public:
  explicit LogRing(std::size_t capacity) : capacity_(capacity < 1 ? 1 : capacity) {}

  void push(const LogEntry& entry) {
    std::lock_guard<std::mutex> lock(mutex_);
    entries_.push_back(entry);
    if (entries_.size() > capacity_) {
      entries_.pop_front();
      ++dropped_;
    }
  }

  std::size_t size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return entries_.size();
  }
  std::size_t capacity() const { return capacity_; }
  std::size_t dropped() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return dropped_;
  }

  // The messages that are at least as bad as `min_type` and whose category starts with `category_prefix` (any category if the prefix is empty), oldest first.
  std::vector<LogEntry> entries(QtMsgType min_type, const QString& category_prefix = QString()) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<LogEntry> out;
    for (const LogEntry& e : entries_) {
      if (severity(e.type) < severity(min_type)) continue;
      if (!category_prefix.isEmpty() && !e.category.startsWith(category_prefix)) continue;
      out.push_back(e);
    }
    return out;
  }

 private:
  std::size_t capacity_;
  mutable std::mutex mutex_;
  std::deque<LogEntry> entries_;
  std::size_t dropped_ = 0;
};

// Sends everything Qt prints to a ring, with its category, for as long as it exists, and puts the old handler back after.
class LogHook {
 public:
  explicit LogHook(LogRing& ring) : ring_(ring), outer_(current().load()) {
    current().store(this);
    previous_ = qInstallMessageHandler(&LogHook::handle);
  }
  ~LogHook() {
    qInstallMessageHandler(previous_);
    current().store(outer_);
  }
  LogHook(const LogHook&) = delete;
  LogHook& operator=(const LogHook&) = delete;

 private:
  static std::atomic<LogHook*>& current() {
    static std::atomic<LogHook*> instance(nullptr);
    return instance;
  }
  static void handle(QtMsgType type, const QMessageLogContext& context, const QString& text) {
    LogHook* hook = current().load();
    if (hook == nullptr) return;
    LogEntry e;
    e.type = type;
    e.category = QString::fromUtf8(context.category ? context.category : "");
    e.text = text;
    hook->ring_.push(e);
  }

  LogRing& ring_;
  LogHook* outer_;
  QtMessageHandler previous_ = nullptr;
};

#endif  // LESSON_SOLUTION_HPP
