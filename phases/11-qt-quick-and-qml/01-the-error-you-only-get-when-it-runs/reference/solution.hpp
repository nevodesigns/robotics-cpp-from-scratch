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

inline QmlProblem problem_from(const QQmlError& error) {
  QmlProblem problem;
  problem.file = error.url().toString();
  problem.message = error.description();
  problem.line = error.line();
  problem.column = error.column();
  return problem;
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
  explicit QmlHost(QQmlEngine& engine) : engine_(engine) {
    // Connecting this is the whole point. Qt does not do it for you, and an
    // application that has not done it cannot tell a working binding from one
    // that quietly evaluated to nothing.
    QObject::connect(&engine_, &QQmlEngine::warnings, &sink_,
                     [this](const QList<QQmlError>& reported) {
                       for (const QQmlError& error : reported)
                         warnings_.push_back(problem_from(error));
                     });
  }

  // The root object on success, or everything that stopped it.
  //
  // Returning the problems rather than a bool is what makes a failure
  // actionable: a caller with a file, a line and a column can point at the
  // mistake, and a caller with false cannot.
  rc::expected<QObject*, std::vector<QmlProblem>> load(const QUrl& url) {
    auto component = std::unique_ptr<QQmlComponent>(new QQmlComponent(&engine_, url));

    QObject* root = component->isError() ? nullptr : component->create();
    if (root == nullptr) {
      std::vector<QmlProblem> problems;
      for (const QQmlError& error : component->errors())
        problems.push_back(problem_from(error));

      // A component that failed with no errors listed would leave a caller with
      // nothing to act on, so say that much rather than hand back an empty list.
      if (problems.empty()) {
        QmlProblem unexplained;
        unexplained.file = url.toString();
        unexplained.message = QStringLiteral("the component produced no root object");
        problems.push_back(unexplained);
      }
      return rc::unexpected(problems);
    }

    // The host keeps the root alive, the way QQmlApplicationEngine does, so a
    // caller cannot accidentally outlive it. Lesson 09-04 is the reason this is
    // stated rather than assumed.
    roots_.push_back(std::unique_ptr<QObject>(root));
    components_.push_back(std::move(component));
    return root;
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
