#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <QList>
#include <QObject>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQmlError>
#include <QString>
#include <QUrl>

#include <memory>
#include <vector>

#include <rc/core/compat.hpp>

// One thing QML has to say about your file: a message, and where.
//
// Keeping the location is the whole difference between a report and a
// complaint. QML knows the line and column of almost everything it objects to,
// and the default handling prints it to a console that, in a running
// application, nobody is watching.
struct QmlProblem {
  QString file;
  QString message;
  int line = 0;
  int column = 0;
};

// TODO 1: carry across everything QQmlError knows.
//
// url(), description(), line() and column(). All four, because a report without
// a location is a complaint, and the test checks that a refused file arrives
// with a line above zero and a message that is not empty.
inline QmlProblem problem_from(const QQmlError& error) {
  (void)error;
  return QmlProblem{};
}

// Loads QML and tells you what happened.
//
// Two kinds of failure, and the reason this class exists is that Qt reports
// them through two different channels.
//
// Errors stop the load. QQmlComponent collects them and hands them over when
// asked, so they are hard to miss.
//
// Warnings do not stop anything. A binding that refers to a name which does not
// exist evaluates to undefined, the property keeps its default, the interface
// draws, and the only trace is a line on stderr. QQmlEngine emits them as a
// signal, and nothing is connected to it unless you connect it.
class QmlHost {
 public:
  // TODO 2: connect to the warnings nobody connects to.
  //
  // QQmlEngine has a signal called warnings, carrying a QList<QQmlError>. It
  // fires when a binding refers to a name that does not exist, when a binding
  // loops, when something assigns to a read only property: none of which stop
  // the file loading, and all of which mean the interface is wrong.
  //
  // Connect it and append each error to warnings_, through problem_from. Pass
  // &sink_ as the receiver so the connection dies with this object rather than
  // outliving it, which is lesson 09-04 applied to a signal you did not write.
  explicit QmlHost(QQmlEngine& engine) : engine_(engine) {}

  // The root object on success, or everything that stopped it.
  //
  // Returning the problems rather than a bool is what makes a failure
  // actionable: a caller with a file, a line and a column can point at the
  // mistake, and a caller with false cannot.
  rc::expected<QObject*, std::vector<QmlProblem>> load(const QUrl& url) {
    // TODO 3: load the file, and report either the object or the reasons.
    //
    // Build a QQmlComponent for the url, against engine_. If it isError(),
    // there is no point calling create(). Otherwise create() gives the root
    // object, or nullptr.
    //
    // On failure, turn component->errors() into a vector of QmlProblem and
    // return rc::unexpected of it. If that list somehow comes back empty, put
    // one problem in it saying so, because a caller handed an empty list of
    // reasons has been told nothing.
    //
    // On success, keep the root and the component alive in roots_ and
    // components_, the way QQmlApplicationEngine keeps what it loads, and
    // return the pointer. A component destroyed while its object is alive
    // takes the object's type information with it.
    (void)url;
    return rc::unexpected(std::vector<QmlProblem>{});
  }

  // What loaded anyway, and should not have.
  const std::vector<QmlProblem>& warnings() const { return warnings_; }
  std::size_t warning_count() const { return warnings_.size(); }
  void clear_warnings() { warnings_.clear(); }

 private:
  QQmlEngine& engine_;

  // The connection needs a receiver whose lifetime bounds it, so that a warning
  // arriving after this host is gone does not reach a dead lambda.
  QObject sink_;

  std::vector<QmlProblem> warnings_;
  std::vector<std::unique_ptr<QQmlComponent>> components_;
  std::vector<std::unique_ptr<QObject>> roots_;
};

#endif  // LESSON_SOLUTION_HPP
