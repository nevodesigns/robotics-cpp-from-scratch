// rc/qt/qml_host.hpp
//
// Loading QML and finding out what went wrong, from lesson 11-01, graduated.
//
// C++ tells you about a mistake when you build. QML has three answers and only
// the first of them is loud.
//
// Measured on Qt 6.2 with the lesson's own cases. Six mistakes that can be seen
// by reading the file, a syntax error, an unknown type, an unknown property, a
// missing import, a string literal where a number goes, and a colour that is
// not one, all six refused the load and all six arrived with a line and a
// column. Three that cannot, a name that does not exist, a typo on a context
// property, and a binding loop, all loaded, left the property at its default,
// and reported themselves only through QQmlEngine::warnings, which nothing is
// connected to unless you connect it. And three said nothing whatever: 0/0,
// undefined, and anchors against a parent the item has not got each produced a
// width of zero with no error and no warning.
//
// The rule those twelve cases add up to: QML checks what it can read, and runs
// the rest. A literal in the file is checked. What a binding computes is not.
//
// The unhooked case is worth stating plainly, because it is what an application
// that has not thought about this has. Loading a file whose binding refers to a
// name that does not exist gives a component whose status() is Ready, whose
// errors() is empty, and whose root object has a width of zero.

#ifndef RC_QT_QML_HOST_HPP
#define RC_QT_QML_HOST_HPP

#include <QList>
#include <QObject>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQmlError>
#include <QString>
#include <QUrl>

#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

#include <rc/core/compat.hpp>

namespace rc {
namespace qt {

// One thing QML has to say about your file: a message, and where.
//
// Keeping the location is the difference between a report and a complaint.
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

// Loads QML and tells you what happened, through both of Qt's channels.
class QmlHost {
 public:
  explicit QmlHost(QQmlEngine& engine) : engine_(engine) {
    // Qt does not connect this for you. An application that has not connected
    // it cannot tell a working binding from one that evaluated to nothing.
    QObject::connect(&engine_, &QQmlEngine::warnings, &sink_,
                     [this](const QList<QQmlError>& reported) {
                       for (const QQmlError& error : reported)
                         warnings_.push_back(problem_from(error));
                     });
  }

  QmlHost(const QmlHost&) = delete;
  QmlHost& operator=(const QmlHost&) = delete;

  // The root object on success, or everything that stopped it.
  rc::expected<QObject*, std::vector<QmlProblem>> load(const QUrl& url) {
    auto component = std::unique_ptr<QQmlComponent>(new QQmlComponent(&engine_, url));

    QObject* root = component->isError() ? nullptr : component->create();
    if (root == nullptr) {
      std::vector<QmlProblem> problems;
      for (const QQmlError& error : component->errors())
        problems.push_back(problem_from(error));

      // A caller handed an empty list of reasons has been told nothing.
      if (problems.empty()) {
        QmlProblem unexplained;
        unexplained.file = url.toString();
        unexplained.message = QStringLiteral("the component produced no root object");
        problems.push_back(unexplained);
      }
      return rc::unexpected(problems);
    }

    // The host keeps both alive, the way QQmlApplicationEngine does. A
    // component destroyed while its object lives takes the object's type
    // information with it.
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

  // The connection needs a receiver whose lifetime bounds it, so a warning
  // arriving after this host is gone does not reach a dead lambda.
  QObject sink_;

  std::vector<QmlProblem> warnings_;
  std::vector<std::unique_ptr<QQmlComponent>> components_;
  std::vector<std::unique_ptr<QObject>> roots_;
};

}  // namespace qt
}  // namespace rc

#endif  // RC_QT_QML_HOST_HPP
