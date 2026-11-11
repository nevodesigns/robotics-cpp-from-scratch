#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QVariantMap>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlContext>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlError>
#include <QtQml/QQmlProperty>

// A QML object runs code when it starts: bindings are evaluated, and Component.onCompleted handlers run, during create(). A C++ program that creates an object and then sets its
// properties has set them after all of that. The handler saw the default, and the binding that read it follows the change, which is why the bug is easy to miss: some things
// update and some are fixed at the value they had.
//
// The way to give an object its values before it starts is to create it in two steps, set the initial properties between them, and complete it. A property declared
// `required` in the QML makes forgetting one an error, and a name that the object does not declare is a typo that the engine only warns about.

// Creates the object of a ready component with `initial` set before it starts. Returns null, with the reason in `*why` if it is not null, when the component is not
// ready, when a name in `initial` is not a property the object declares (all such names are listed), or when the component reported an error while starting, which is what a
// `required` property that was not given is. Nothing is left behind on failure.
inline QObject* create_with(QQmlComponent& component, const QVariantMap& initial, QString* why) {
  // TODO: if the status is not Ready: null, with *why the errors (each description and a newline) or "the component is not ready". Then
  // object = component.beginCreate(component.engine()->rootContext()); null: the errors. For each key of `initial`, if QQmlProperty(object, key) is not valid
  // add "no property named " + key and a newline to a list of problems; if there are any, component.completeCreate(), delete the object, set *why, return null.
  // Otherwise component.setInitialProperties(object, initial); component.completeCreate(); if component.errors() is not empty, delete the object, *why is
  // them, return null. Otherwise return the object.
  (void)component;
  (void)initial;
  if (why != nullptr) *why = QStringLiteral("not written yet");
  return nullptr;
}

#endif  // LESSON_SOLUTION_HPP
