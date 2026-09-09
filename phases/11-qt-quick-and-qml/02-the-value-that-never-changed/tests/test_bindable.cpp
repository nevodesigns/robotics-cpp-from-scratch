// The framework's main is suppressed because a QML engine needs a
// QGuiApplication to exist before it does.
#define RC_TEST_NO_MAIN
#include <rc/test/rc_test.hpp>

#include <QGuiApplication>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlError>
#include <QFile>
#include <QTemporaryDir>
#include <QTextStream>
#include <QUrl>

#include <iomanip>
#include <iostream>
#include <vector>

#include <rc/qt/qml_host.hpp>

#include "solution.hpp"

namespace {

// Three ways of getting it wrong, kept here rather than in the artifact
// because they are demonstrations and not something to ship.

// The declaration is incomplete: no NOTIFY at all.
class NoNotify : public QObject {
  Q_OBJECT
  Q_PROPERTY(double speed READ speed WRITE setSpeed)
 public:
  double speed() const { return speed_; }
  void setSpeed(double value) { speed_ = value; }
 private:
  double speed_ = 1.0;
};

// The declaration is right and the implementation is not.
class ForgotToEmit : public QObject {
  Q_OBJECT
  Q_PROPERTY(double speed READ speed WRITE setSpeed NOTIFY speedChanged)
 public:
  double speed() const { return speed_; }
  void setSpeed(double value) { speed_ = value; }
 signals:
  void speedChanged();
 private:
  double speed_ = 1.0;
};

// A promise that the value never changes, made by an object that changes it.
class ConstantLie : public QObject {
  Q_OBJECT
  Q_PROPERTY(double speed READ speed CONSTANT)
 public:
  double speed() const { return speed_; }
  void setSpeed(double value) { speed_ = value; }
 private:
  double speed_ = 1.0;
};

// A setter that emits whether or not anything changed.
class Chatty : public QObject {
  Q_OBJECT
  Q_PROPERTY(double speed READ speed WRITE setSpeed NOTIFY speedChanged)
 public:
  double speed() const { return speed_; }
  void setSpeed(double value) { speed_ = value; emit speedChanged(); }
 signals:
  void speedChanged();
 private:
  double speed_ = 1.0;
};

// Counts how often QML re-ran the binding, which is not the same as how often
// the answer changed.
class Tally : public QObject {
  Q_OBJECT
 public:
  Q_INVOKABLE double tap(double value) { ++evaluations; return value; }
  int evaluations = 0;
};

// A panel of one property, which reads the source through the tally so every
// evaluation is counted.
const char* kPanel =
    "import QtQuick\n"
    "Item {\n"
    "  property real shown: tally.tap(source.speed)\n"
    "}\n";

QUrl write_panel(QTemporaryDir& dir) {
  const QString path = dir.path() + "/Panel.qml";
  QFile file(path);
  file.open(QIODevice::WriteOnly | QIODevice::Truncate);
  QTextStream(&file) << kPanel;
  file.close();
  return QUrl::fromLocalFile(path);
}

// Loads the panel against one source object, changes the value, and reports
// what QML ended up showing.
struct Observation {
  double cpp_value = 0.0;
  double qml_value = 0.0;
  int evaluations = 0;
  std::size_t warnings = 0;
  bool loaded = false;
};

template <class Source, class Mutate>
Observation observe(QTemporaryDir& dir, Source& source, Mutate mutate) {
  QQmlEngine engine;
  rc::qt::QmlHost host(engine);
  Tally tally;
  engine.rootContext()->setContextProperty("source", &source);
  engine.rootContext()->setContextProperty("tally", &tally);

  Observation seen;
  const auto loaded = host.load(write_panel(dir));
  if (!loaded.has_value()) return seen;
  seen.loaded = true;

  mutate(source);

  seen.cpp_value = source.speed();
  seen.qml_value = loaded.value()->property("shown").toDouble();
  seen.evaluations = tally.evaluations;
  seen.warnings = host.warning_count();
  return seen;
}

const auto set_to_seven = [](auto& source) { source.setSpeed(7.5); };

}  // namespace

RC_TEST("a property declared and written properly reaches QML") {
  QTemporaryDir dir;
  Telemetry telemetry;
  const Observation seen = observe(dir, telemetry, set_to_seven);

  RC_REQUIRE(seen.loaded);
  RC_CHECK_NEAR(seen.cpp_value, 7.5, 1e-12);
  RC_CHECK_NEAR(seen.qml_value, 7.5, 1e-12);
  RC_CHECK(seen.warnings == 0);

  // Once at creation and once when the value changed.
  RC_CHECK(seen.evaluations == 2);
}

RC_TEST("three ways of losing the value, none of which say anything") {
  QTemporaryDir dir;

  Telemetry correct;
  NoNotify no_notify;
  ForgotToEmit forgot;
  ConstantLie constant;

  const Observation rows[] = {
      observe(dir, correct, set_to_seven),
      observe(dir, no_notify, set_to_seven),
      observe(dir, forgot, set_to_seven),
      observe(dir, constant, set_to_seven),
  };
  const char* names[] = {"NOTIFY, emitted", "no NOTIFY", "NOTIFY, not emitted",
                         "CONSTANT"};

  std::cout << "\n  the same value, exposed four ways\n\n";
  std::cout << "  " << std::left << std::setw(24) << "how it is exposed" << std::right
            << std::setw(10) << "C++ now" << std::setw(11) << "QML sees"
            << std::setw(11) << "re-evals" << std::setw(11) << "warnings" << "\n";

  int stale = 0;
  for (int i = 0; i < 4; ++i) {
    RC_REQUIRE(rows[i].loaded);
    std::cout << "  " << std::left << std::setw(24) << names[i] << std::right
              << std::fixed << std::setprecision(2) << std::setw(10) << rows[i].cpp_value
              << std::setw(11) << rows[i].qml_value << std::setw(11)
              << rows[i].evaluations << std::setw(11) << rows[i].warnings << "\n";

    // Not one of them warns. By the taxonomy of lesson 11-01 this is the third
    // tier: it loads, it runs, and nothing anywhere reports it.
    RC_CHECK(rows[i].warnings == 0);
    if (rows[i].qml_value != rows[i].cpp_value) ++stale;
  }

  std::cout << "\n  " << stale << " of 4 left QML showing a value C++ had moved on from\n\n";

  RC_CHECK_NEAR(rows[0].qml_value, 7.5, 1e-12);
  for (int i = 1; i < 4; ++i) {
    RC_CHECK_NEAR(rows[i].qml_value, 1.0, 1e-12);
    RC_CHECK_NEAR(rows[i].cpp_value, 7.5, 1e-12);
    RC_CHECK(rows[i].evaluations == 1);
  }
  RC_CHECK(stale == 3);
}

RC_TEST("the meta object catches one of the three, and it is honest about which") {
  const QMetaObject* metas[] = {&Telemetry::staticMetaObject, &NoNotify::staticMetaObject,
                                &ForgotToEmit::staticMetaObject,
                                &ConstantLie::staticMetaObject};

  std::cout << "\n  what the meta object knows, before anything runs\n\n";
  std::cout << "  " << std::left << std::setw(16) << "class" << std::setw(11)
            << "property" << std::setw(13) << "notifiable" << std::setw(11) << "constant"
            << "QML can follow\n";

  for (const QMetaObject* meta : metas) {
    for (const PropertyReport& report : inspect_properties(*meta)) {
      std::cout << "  " << std::left << std::setw(16) << meta->className()
                << std::setw(11) << report.name.toStdString() << std::setw(13)
                << (report.notifiable ? "yes" : "NO") << std::setw(11)
                << (report.constant ? "yes" : "no")
                << (report.trackable() ? "yes" : "NO") << "\n";
    }
  }
  std::cout << "\n";

  // The one it catches.
  RC_CHECK(every_property_is_trackable(Telemetry::staticMetaObject));
  RC_CHECK(!every_property_is_trackable(NoNotify::staticMetaObject));
  const std::vector<QString> untrackable =
      properties_qml_cannot_track(NoNotify::staticMetaObject);
  RC_CHECK(untrackable.size() == 1);
  // Guarded, because RC_CHECK records and carries on, and front() on an empty
  // vector is not a test report.
  if (!untrackable.empty()) {
    RC_CHECK(untrackable.front() == QStringLiteral("speed"));
  }

  // The two it does not, and this is the point rather than a shortcoming to be
  // apologised for. Both of these declarations are well formed. What is wrong
  // is in a function body, and the meta object has never seen a function body.
  RC_CHECK(every_property_is_trackable(ForgotToEmit::staticMetaObject));
  RC_CHECK(every_property_is_trackable(ConstantLie::staticMetaObject));

  std::cout << "  the check clears 3 of 4, and 2 of those 3 are broken\n\n";

  // Telemetry declares two properties and both are followable.
  RC_CHECK(inspect_properties(Telemetry::staticMetaObject).size() == 2);
  for (const PropertyReport& report : inspect_properties(Telemetry::staticMetaObject)) {
    RC_CHECK(report.notifiable);
    RC_CHECK(report.writable);
    RC_CHECK(report.trackable());
  }
}

RC_TEST("a setter that emits when nothing changed makes QML re-run every binding") {
  QTemporaryDir dir;

  const auto hammer = [](auto& source) {
    source.setSpeed(3.0);
    for (int i = 0; i < 1000; ++i) source.setSpeed(3.0);
  };

  Chatty chatty;
  Telemetry guarded;
  const Observation loud = observe(dir, chatty, hammer);
  const Observation quiet = observe(dir, guarded, hammer);

  RC_REQUIRE(loud.loaded);
  RC_REQUIRE(quiet.loaded);

  std::cout << "\n  setting the same value a thousand times over\n\n";
  std::cout << "  " << std::left << std::setw(24) << "setter" << std::right
            << std::setw(12) << "re-evals" << "\n";
  std::cout << "  " << std::left << std::setw(24) << "no guard" << std::right
            << std::setw(12) << loud.evaluations << "\n";
  std::cout << "  " << std::left << std::setw(24) << "guarded on equality" << std::right
            << std::setw(12) << quiet.evaluations << "\n\n";

  // Both show the right number. Only one of them worked for it.
  RC_CHECK_NEAR(loud.qml_value, 3.0, 1e-12);
  RC_CHECK_NEAR(quiet.qml_value, 3.0, 1e-12);

  // Two evaluations for the guarded one: creation, and the single real change.
  RC_CHECK(quiet.evaluations == 2);
  RC_CHECK(loud.evaluations > 1000);
}

RC_TEST("replacing the context object does reach a binding that already exists") {
  QTemporaryDir dir;
  QQmlEngine engine;
  rc::qt::QmlHost host(engine);
  Tally tally;

  Telemetry first;
  Telemetry second;
  second.setSpeed(42.0);

  engine.rootContext()->setContextProperty("source", &first);
  engine.rootContext()->setContextProperty("tally", &tally);

  const auto loaded = host.load(write_panel(dir));
  RC_REQUIRE(loaded.has_value());
  const double before = loaded.value()->property("shown").toDouble();
  RC_CHECK_NEAR(before, 0.0, 1e-12);

  engine.rootContext()->setContextProperty("source", &second);
  const double after = loaded.value()->property("shown").toDouble();

  std::cout << "\n  context object replaced after the panel existed: " << std::fixed
            << std::setprecision(2) << before << " became " << after << "\n";

  RC_CHECK_NEAR(after, 42.0, 1e-12);

  // And the new object is connected, not merely read once. Replacing the
  // context property re-evaluates the binding whatever the object does, so
  // without this the test would pass against a source QML cannot follow.
  second.setSpeed(9.0);
  const double moved = loaded.value()->property("shown").toDouble();
  std::cout << "  then the new object moved to 9.00 and the panel showed "
            << moved << "\n\n";
  RC_CHECK_NEAR(moved, 9.0, 1e-12);
  RC_CHECK(host.warning_count() == 0);
}

int main(int argc, char** argv) {
  QGuiApplication application(argc, argv);
  return rc::test::run_all();
}

#include "test_bindable.moc"
