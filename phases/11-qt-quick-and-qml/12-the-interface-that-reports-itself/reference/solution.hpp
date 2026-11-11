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
    std::unique_ptr<QQmlComponent> component(new QQmlComponent(&engine_, url, QQmlComponent::Asynchronous));
    const rc::qt::LoadResult result = rc::qt::wait_until_ready(*component, timeout_ms_);
    QString why;
    QObject* object = nullptr;
    if (result == rc::qt::LoadResult::kReady) {
      object = rc::qt::create_with(*component, initial, &why);
    } else if (result == rc::qt::LoadResult::kTimeout) {
      why = QStringLiteral("timed out while loading ") + url.toString();
    } else {
      for (const QQmlError& e : component->errors()) why += e.toString() + QLatin1Char('\n');
      if (why.isEmpty()) why = QStringLiteral("the component has no source");
    }
    if (object == nullptr) own_.push_back(Failure{QStringLiteral("load"), why.trimmed()});
    else objects_.push_back(object);
    components_.push_back(std::move(component));
    return object;
  }

  // Every failure: those this harness recorded itself, in order, and then each warning, critical or fatal message Qt printed while it was alive.
  std::vector<Failure> failures() const {
    std::vector<Failure> all = own_;
    for (const rc::qt::CapturedMessage& m : capture_.messages()) {
      if (m.type == QtWarningMsg) all.push_back(Failure{QStringLiteral("warning"), m.text});
      else if (m.type == QtCriticalMsg || m.type == QtFatalMsg) all.push_back(Failure{QStringLiteral("critical"), m.text});
    }
    return all;
  }
  bool ok() const { return failures().empty(); }

  // How long a load may take, in milliseconds, before it is a failure.
  int timeout_ms() const { return timeout_ms_; }

  // Reading, writing and evaluating through the probe, each of which records a "probe" failure when it is refused.
  rc::qt::ProbeResult read(QObject* object, const QString& name) { return note(rc::qt::read(object, name)); }
  rc::qt::ProbeResult set(QObject* object, const QString& name, const QVariant& value) { return note(rc::qt::set(object, name, value)); }
  rc::qt::ProbeResult evaluate(QObject* scope, const QString& expression) { return note(rc::qt::evaluate(engine_, scope, expression)); }

  // The failures as text, one on each line, as "kind: text".
  QString report() const {
    QString text;
    for (const Failure& f : failures()) text += f.kind + QStringLiteral(": ") + f.text + QLatin1Char('\n');
    return text;
  }

  // Forgets every failure so far, so that a test can look at what one step caused.
  void clear() {
    own_.clear();
    capture_.clear();
  }

 private:
  rc::qt::ProbeResult note(const rc::qt::ProbeResult& result) {
    if (!result.ok) own_.push_back(Failure{QStringLiteral("probe"), result.error});
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
