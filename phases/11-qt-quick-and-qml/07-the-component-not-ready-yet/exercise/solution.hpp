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
  // TODO: if the status is not Loading, return kReady for Ready, kError for Error and kNotLoading for Null. Otherwise a QEventLoop, and a single shot QTimer
  // of timeout_ms that quits it, and a connection from component.statusChanged that quits it when the status is not Loading any more (a lambda taking the
  // status). timer.start(timeout_ms); loop.exec(); then answer from the status as before, with kTimeout if it is still Loading.
  (void)component;
  (void)timeout_ms;
  return LoadResult::kTimeout;
}

// The object a component makes once it is ready, or null with the reason in `*why`: the errors of the component, joined, or that it timed out.
inline QObject* create_when_ready(QQmlComponent& component, int timeout_ms, QString* why) {
  // TODO: result = wait_until_ready(component, timeout_ms). If it is not kReady: set *why (if why is not null) to "timed out while loading", "the component has
  // no source" or the errors of the component each with toString() and a newline, and return nullptr. Otherwise component.create(); if that is null, *why is the
  // errors, or "the component made no object" if there are none.
  (void)component;
  (void)timeout_ms;
  if (why != nullptr) *why = QStringLiteral("not written yet");
  return nullptr;
}

#endif  // LESSON_SOLUTION_HPP
