#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <QtCore/QEventLoop>
#include <QtCore/QString>
#include <QtCore/QTimer>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlError>

// A QQmlComponent loaded in the background is not ready when the constructor returns. Its status is Loading, create() on it gives a null pointer, and the code that was
// written for a component that loads at once, which is what a file on a local disk does by default, uses the null.
//
// The way to use one is to let the event loop run until its status changes, and to give up after a time: a component that loads from a network, or from a file that is
// being written, can stay Loading, and a test that waits for ever is a test that hangs.

enum class LoadResult { kReady, kError, kTimeout, kNotLoading };

// Runs the event loop until the component is Ready or has failed, or `timeout_ms` has passed. A component that is already Ready or in Error returns at once, and one
// with no source at all (status Null) is kNotLoading: there is nothing to wait for.
inline LoadResult wait_until_ready(QQmlComponent& component, int timeout_ms) {
  const auto result_now = [&component]() {
    if (component.status() == QQmlComponent::Ready) return LoadResult::kReady;
    if (component.status() == QQmlComponent::Error) return LoadResult::kError;
    if (component.status() == QQmlComponent::Null) return LoadResult::kNotLoading;
    return LoadResult::kTimeout;
  };
  if (component.status() != QQmlComponent::Loading) return result_now();

  QEventLoop loop;
  QTimer timer;
  timer.setSingleShot(true);
  QObject::connect(&component, &QQmlComponent::statusChanged, &loop, [&loop](QQmlComponent::Status status) {
    if (status != QQmlComponent::Loading) loop.quit();
  });
  QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
  timer.start(timeout_ms);
  loop.exec();
  return result_now();
}

// The object a component makes once it is ready, or null with the reason in `*why`: the errors of the component, joined, or that it timed out.
inline QObject* create_when_ready(QQmlComponent& component, int timeout_ms, QString* why) {
  const LoadResult result = wait_until_ready(component, timeout_ms);
  if (result != LoadResult::kReady) {
    if (why != nullptr) {
      if (result == LoadResult::kTimeout) *why = QStringLiteral("timed out while loading");
      else if (result == LoadResult::kNotLoading) *why = QStringLiteral("the component has no source");
      else {
        QString text;
        for (const QQmlError& e : component.errors()) text += e.toString() + QLatin1Char('\n');
        *why = text;
      }
    }
    return nullptr;
  }
  QObject* object = component.create();
  if (object == nullptr && why != nullptr) {
    QString text;
    for (const QQmlError& e : component.errors()) text += e.toString() + QLatin1Char('\n');
    *why = text.isEmpty() ? QStringLiteral("the component made no object") : text;
  }
  return object;
}

#endif  // LESSON_SOLUTION_HPP
