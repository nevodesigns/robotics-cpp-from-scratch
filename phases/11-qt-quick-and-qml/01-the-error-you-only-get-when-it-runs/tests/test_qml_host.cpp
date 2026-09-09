// The framework's main is suppressed because QML needs a QGuiApplication to
// exist before an engine does.
#define RC_TEST_NO_MAIN
#include <rc/test/rc_test.hpp>

#include <QGuiApplication>
#include <QQmlContext>
#include <QQmlEngine>
#include <QTemporaryDir>
#include <QFile>
#include <QTextStream>
#include <QVariant>

#include <iomanip>
#include <iostream>
#include <string>

#include "solution.hpp"

namespace {

// Writes a QML file and hands back its URL. Every case in this suite is a few
// lines of QML, so keeping them inline keeps the mistake next to the result.
QUrl write_qml(QTemporaryDir& dir, const QString& name, const QString& body) {
  const QString path = dir.path() + "/" + name;
  QFile file(path);
  file.open(QIODevice::WriteOnly | QIODevice::Truncate);
  QTextStream(&file) << body;
  file.close();
  return QUrl::fromLocalFile(path);
}

struct Case {
  const char* name;
  const char* body;
};

// A file that is correct, so the suite is measuring the mistakes rather than
// its own setup.
const char* kClean = "import QtQuick\nItem { width: 100; height: 40 }\n";

std::string as_text(const QString& text) { return text.toStdString(); }

}  // namespace

RC_TEST("a correct file loads, and its properties are what the file says") {
  QTemporaryDir dir;
  QQmlEngine engine;
  QmlHost host(engine);

  const auto loaded = host.load(write_qml(dir, "Clean.qml", kClean));
  RC_REQUIRE(loaded.has_value());

  QObject* root = loaded.value();
  RC_REQUIRE(root != nullptr);
  RC_CHECK_NEAR(root->property("width").toDouble(), 100.0, 1e-12);
  RC_CHECK_NEAR(root->property("height").toDouble(), 40.0, 1e-12);

  // Nothing to report, and the count says so rather than the absence of output.
  RC_CHECK(host.warning_count() == 0);
}

RC_TEST("what QML refuses to load at all, and where it says the mistake is") {
  QTemporaryDir dir;

  const Case cases[] = {
      {"a syntax error", "import QtQuick\nItem { width: }\n"},
      {"a type that does not exist", "import QtQuick\nButtn { }\n"},
      {"a property that does not exist", "import QtQuick\nItem { widht: 100 }\n"},
      {"a missing import", "Item { width: 100 }\n"},
      {"a string where a number goes", "import QtQuick\nItem { width: \"wide\" }\n"},
      {"a colour that is not one", "import QtQuick\nRectangle { color: \"notacolour\" }\n"},
  };

  std::cout << "\n  mistakes QML can see written down\n\n";
  std::cout << "  " << std::left << std::setw(34) << "case" << std::setw(9) << "loaded"
            << std::setw(9) << "errors" << "first location\n" << std::right;

  int refused = 0;
  for (const Case& item : cases) {
    QQmlEngine engine;
    QmlHost host(engine);
    const auto loaded = host.load(write_qml(dir, "Case.qml", item.body));

    std::string where = "-";
    int errors = 0;
    if (!loaded.has_value()) {
      ++refused;
      errors = static_cast<int>(loaded.error().size());
      if (!loaded.error().empty()) {
        const QmlProblem& first = loaded.error().front();
        where = std::to_string(first.line) + ":" + std::to_string(first.column);
      }
    }

    std::cout << "  " << std::left << std::setw(34) << item.name << std::setw(9)
              << (loaded.has_value() ? "yes" : "NO") << std::setw(9) << errors << where
              << "\n" << std::right;

    // Every one of these is a mistake QML can see by reading the file, so every
    // one of them must stop the load and arrive with somewhere to look.
    RC_CHECK(!loaded.has_value());
    if (!loaded.has_value()) {
      // Guarded, because RC_CHECK records and carries on, and front() on an
      // empty list is not a test report.
      RC_CHECK(!loaded.error().empty());
      if (!loaded.error().empty()) {
        RC_CHECK(loaded.error().front().line > 0);
        RC_CHECK(!loaded.error().front().message.isEmpty());
      }
    }
  }

  std::cout << "\n  " << refused << " of 6 refused before anything was created\n\n";
  RC_CHECK(refused == 6);
}

RC_TEST("what loads anyway and only warns, with the property left at its default") {
  QTemporaryDir dir;

  const Case cases[] = {
      {"a name that does not exist",
       "import QtQuick\nItem { width: missingThing.value }\n"},
      {"a typo on a context property", "import QtQuick\nItem { width: robto.speed }\n"},
      {"a binding loop", "import QtQuick\nItem { id: r; width: r.height + 1; height: r.width + 1 }\n"},
  };

  std::cout << "\n  mistakes QML only finds by running the binding\n\n";
  std::cout << "  " << std::left << std::setw(34) << "case" << std::setw(9) << "loaded"
            << std::setw(11) << "warnings" << "width\n" << std::right;

  for (const Case& item : cases) {
    QQmlEngine engine;
    QObject robot;
    engine.rootContext()->setContextProperty("robot", &robot);
    QmlHost host(engine);

    const auto loaded = host.load(write_qml(dir, "Case.qml", item.body));

    std::cout << "  " << std::left << std::setw(34) << item.name << std::setw(9)
              << (loaded.has_value() ? "yes" : "NO") << std::setw(11)
              << host.warning_count();
    if (loaded.has_value()) {
      std::cout << loaded.value()->property("width").toDouble();
    }
    std::cout << "\n" << std::right;

    // The interface exists. Nothing stopped. That is the danger.
    RC_CHECK(loaded.has_value());

    // And the only reason we know is that something connected to the signal.
    RC_CHECK(host.warning_count() > 0);
    if (host.warning_count() > 0) {
      RC_CHECK(!host.warnings().front().message.isEmpty());
    }
  }
  std::cout << "\n";
}

RC_TEST("what loads and says nothing at all") {
  QTemporaryDir dir;

  const Case cases[] = {
      {"zero divided by zero", "import QtQuick\nItem { width: 0/0 }\n"},
      {"undefined assigned to a number", "import QtQuick\nItem { width: undefined }\n"},
      {"anchors against a parent it has not got",
       "import QtQuick\nItem { anchors.fill: parent }\n"},
  };

  std::cout << "\n  mistakes nothing reports\n\n";
  std::cout << "  " << std::left << std::setw(42) << "case" << std::setw(9) << "loaded"
            << std::setw(9) << "errors" << std::setw(11) << "warnings" << "width\n"
            << std::right;

  for (const Case& item : cases) {
    QQmlEngine engine;
    QmlHost host(engine);
    const auto loaded = host.load(write_qml(dir, "Case.qml", item.body));

    RC_REQUIRE(loaded.has_value());
    const double width = loaded.value()->property("width").toDouble();

    std::cout << "  " << std::left << std::setw(42) << item.name << std::setw(9) << "yes"
              << std::setw(9) << 0 << std::setw(11) << host.warning_count() << width
              << "\n" << std::right;

    // No error, no warning, and a width of zero. An interface built out of
    // these draws, and draws wrong, and the only instrument that catches it is
    // a test that asserts what the value should be.
    RC_CHECK(host.warning_count() == 0);
    RC_CHECK_NEAR(width, 0.0, 1e-12);
  }

  std::cout << "\n  the rule: QML checks what it can read, and runs the rest\n\n";
}

RC_TEST("without the hook, the middle tier is invisible") {
  QTemporaryDir dir;
  const QString body = "import QtQuick\nItem { width: missingThing.value }\n";

  // An engine nobody connected to. This is what an application that did not
  // think about it has.
  {
    QQmlEngine engine;
    QQmlComponent component(&engine, write_qml(dir, "Silent.qml", body));
    QObject* root = component.create();

    RC_REQUIRE(root != nullptr);
    // Everything the ordinary API offers says this file is fine.
    RC_CHECK(!component.isError());
    RC_CHECK(component.errors().isEmpty());
    RC_CHECK(component.status() == QQmlComponent::Ready);
    RC_CHECK_NEAR(root->property("width").toDouble(), 0.0, 1e-12);
    delete root;
  }

  // The same file, through the host.
  {
    QQmlEngine engine;
    QmlHost host(engine);
    const auto loaded = host.load(write_qml(dir, "Silent.qml", body));
    RC_REQUIRE(loaded.has_value());
    RC_CHECK(host.warning_count() > 0);

    std::cout << "\n  the same file, unhooked: Ready, no errors, width 0\n";
    std::cout << "  through the host:          " << host.warning_count()
              << " warning, at line " << host.warnings().front().line << "\n";
    std::cout << "  message: " << as_text(host.warnings().front().message) << "\n\n";
  }
}

RC_TEST("a file that is not there is a failure with something to say") {
  QQmlEngine engine;
  QmlHost host(engine);

  const auto loaded = host.load(QUrl::fromLocalFile("/no/such/file/Nope.qml"));
  RC_CHECK(!loaded.has_value());
  if (!loaded.has_value()) {
    RC_CHECK(!loaded.error().empty());
    if (!loaded.error().empty()) {
      RC_CHECK(!loaded.error().front().message.isEmpty());
    }
  }
}

RC_TEST("warnings accumulate across loads, and can be cleared") {
  QTemporaryDir dir;
  QQmlEngine engine;
  QmlHost host(engine);

  const QString body = "import QtQuick\nItem { width: missingThing.value }\n";
  const auto first = host.load(write_qml(dir, "One.qml", body));
  RC_REQUIRE(first.has_value());
  const std::size_t after_one = host.warning_count();
  RC_CHECK(after_one > 0);

  const auto second = host.load(write_qml(dir, "Two.qml", body));
  RC_REQUIRE(second.has_value());
  RC_CHECK(host.warning_count() > after_one);

  host.clear_warnings();
  RC_CHECK(host.warning_count() == 0);
}

int main(int argc, char** argv) {
  // A QML engine cannot exist before this. With QT_QPA_PLATFORM=offscreen it
  // needs no display, which is how these tests run in continuous integration.
  QGuiApplication application(argc, argv);
  return rc::test::run_all();
}
