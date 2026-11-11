#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QUrl>
#include <QtCore/QVariantMap>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlError>
#include <memory>
#include <vector>
#include <rc/qt/message_capture.hpp>
#include <rc/qt/qml_create.hpp>
#include <rc/qt/qml_loader.hpp>
#include <rc/qt/qml_probe.hpp>

// Phase 11 ends with a QML interface whose failures are visible from a test rather than from a console nobody is watching. Each lesson found one place where QML goes wrong
// quietly, and made it loud:
//
//   a load that fails, with its file and line                11-01
//   what Qt prints, kept for the test                        11-03
//   a property set on a name that is not one                 11-06
//   a component that is not ready when it is used            11-07
//   a value set after the object started, a required one      11-08
//
// The harness is the place they meet. A test loads its QML through it, and afterwards asks one question: were there any failures? The answer is a list, in the order they
// happened, with what each was, and a test that expects a failure says which one.

struct Failure {
  QString kind;     // "load" for a file that did not become an object, "probe" for a read or write that was refused, "warning" or "critical" for what Qt printed
  QString text;
};

class QmlHarness {
 public:
  explicit QmlHarness(QQmlEngine& engine, int timeout_ms = 5000) : engine_(engine), timeout_ms_(timeout_ms) {}

  ~QmlHarness() {
    for (QObject* object : objects_) delete object;
    objects_.clear();
    components_.clear();
  }
  QmlHarness(const QmlHarness&) = delete;
  QmlHarness& operator=(const QmlHarness&) = delete;

  // Loads a file in the background, waits for it, and creates its object with `initial` set before it starts. Returns null and records a "load" failure with the reason if
  // it did not time out, parse, or start. The harness owns the object.
  QObject* load(const QUrl& url, const QVariantMap& initial = QVariantMap()) {
    // TODO: component = new QQmlComponent(&engine_, url, QQmlComponent::Asynchronous) in a unique_ptr. result = rc::qt::wait_until_ready(*component, timeout_ms_).
    // If it is kReady: object = rc::qt::create_with(*component, initial, &why). If kTimeout: why = "timed out while loading " + the url. Otherwise why is the
    // errors of the component (each toString() and a newline), or "the component has no source". If there is no object, push Failure{"load", why.trimmed()} onto
    // own_; otherwise push the object onto objects_. Either way keep the component (components_.push_back(std::move(component))). Return the object.
    (void)url;
    (void)initial;
    return nullptr;
  }

  // Every failure: those this harness recorded itself, in order, and then each warning, critical or fatal message Qt printed while it was alive.
  std::vector<Failure> failures() const {
    // TODO: start from own_. Then for each message of capture_.messages(): a QtWarningMsg is Failure{"warning", text}; a QtCriticalMsg or QtFatalMsg is
    // Failure{"critical", text}. Anything quieter (debug, info) is not a failure.
    return std::vector<Failure>();
  }
  bool ok() const { return failures().empty(); }

  // How long a load may take, in milliseconds, before it is a failure.
  int timeout_ms() const { return timeout_ms_; }

  // Reading, writing and evaluating through the probe, each of which records a "probe" failure when it is refused.
  rc::qt::ProbeResult read(QObject* object, const QString& name) { return rc::qt::read(object, name); }
  rc::qt::ProbeResult set(QObject* object, const QString& name, const QVariant& value) { return rc::qt::set(object, name, value); }
  rc::qt::ProbeResult evaluate(QObject* scope, const QString& expression) { return rc::qt::evaluate(engine_, scope, expression); }

  // The failures as text, one on each line, as "kind: text".
  QString report() const {
    // TODO: for each failure, kind + ": " + text + a newline.
    return QString();
  }

  // Forgets every failure so far, so that a test can look at what one step caused.
  void clear() {
    // TODO: own_.clear() and capture_.clear().
  }

 private:
  // TODO: the three wrappers above should pass what they get through here, which records a "probe" failure on own_ when result.ok is false, and returns the result.
  rc::qt::ProbeResult note(const rc::qt::ProbeResult& result) {
    (void)result;
    return result;
  }

  QQmlEngine& engine_;
  int timeout_ms_;
  rc::qt::MessageCapture capture_;
  std::vector<Failure> own_;
  std::vector<std::unique_ptr<QQmlComponent>> components_;
  std::vector<QObject*> objects_;
};

#endif  // LESSON_SOLUTION_HPP
