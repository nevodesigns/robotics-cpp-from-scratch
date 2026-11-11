#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <QtCore/QString>
#include <QtCore/QtGlobal>
#include <atomic>
#include <cstddef>
#include <mutex>
#include <vector>

// Qt reports a great deal by printing it: a QML handler that throws, a binding that loops, a deprecated call, a qWarning from your own code. It goes to standard
// error through a message handler, where a person might see it and a test does not. A suite that passes with forty warnings in its log has passed with forty things
// wrong.
//
// A scope that installs its own handler keeps the messages, so that the test can look at them: how many warnings, whether this one is among them. It puts the
// old handler back when it ends, because a handler that is left installed outlives the object it writes into.

struct CapturedMessage {
  QtMsgType type = QtDebugMsg;
  QString text;
  QString file;
  int line = 0;
};

class MessageCapture {
 public:
  // If `forward` is true each message is also passed on to the handler that was installed before, so that it still shows in the log.
  explicit MessageCapture(bool forward = false) : forward_(forward) {
    outer_ = current().load();
    current().store(this);
    previous_ = qInstallMessageHandler(&MessageCapture::handle);
  }
  ~MessageCapture() {
    qInstallMessageHandler(previous_);
    current().store(outer_);
  }
  MessageCapture(const MessageCapture&) = delete;
  MessageCapture& operator=(const MessageCapture&) = delete;

  // Whether messages are also passed on to the handler that was there before.
  bool forwards() const { return forward_; }

  std::vector<CapturedMessage> messages() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return messages_;
  }
  std::size_t size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return messages_.size();
  }
  // How many messages of this type were seen.
  std::size_t count(QtMsgType type) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::size_t n = 0;
    for (const CapturedMessage& m : messages_)
      if (m.type == type) ++n;
    return n;
  }
  // Whether any message contains this text.
  bool contains(const QString& text) const {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const CapturedMessage& m : messages_)
      if (m.text.contains(text)) return true;
    return false;
  }
  void clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    messages_.clear();
  }

 private:
  static std::atomic<MessageCapture*>& current() {
    static std::atomic<MessageCapture*> instance(nullptr);
    return instance;
  }

  // The handler Qt calls, from whichever thread printed. It is a plain function, so it finds the scope through `current`.
  static void handle(QtMsgType type, const QMessageLogContext& context, const QString& text) {
    // TODO: capture = current().load(); return if it is null. Under capture->mutex_ (std::lock_guard<std::mutex>), push a CapturedMessage with the type, the
    // text, the file (QString::fromUtf8(context.file), which may be a null pointer: use "" then) and the line. After the lock, if capture->forward_ and
    // capture->previous_ is not null, call capture->previous_(type, context, text).
    (void)type;
    (void)context;
    (void)text;
  }

  bool forward_;
  QtMessageHandler previous_ = nullptr;
  MessageCapture* outer_ = nullptr;
  mutable std::mutex mutex_;
  std::vector<CapturedMessage> messages_;
};

#endif  // LESSON_SOLUTION_HPP
