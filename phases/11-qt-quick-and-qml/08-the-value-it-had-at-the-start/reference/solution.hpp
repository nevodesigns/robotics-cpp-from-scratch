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
  const auto errors_text = [&component]() {
    QString text;
    for (const QQmlError& e : component.errors()) text += e.description() + QLatin1Char('\n');
    return text;
  };
  if (component.status() != QQmlComponent::Ready) {
    if (why != nullptr) *why = component.isError() ? errors_text() : QStringLiteral("the component is not ready");
    return nullptr;
  }

  QObject* object = component.beginCreate(component.engine()->rootContext());
  if (object == nullptr) {
    if (why != nullptr) *why = errors_text();
    return nullptr;
  }

  QString unknown;
  for (auto it = initial.constBegin(); it != initial.constEnd(); ++it)
    if (!QQmlProperty(object, it.key()).isValid()) unknown += QStringLiteral("no property named ") + it.key() + QLatin1Char('\n');
  if (!unknown.isEmpty()) {
    component.completeCreate();
    delete object;
    if (why != nullptr) *why = unknown;
    return nullptr;
  }

  component.setInitialProperties(object, initial);
  component.completeCreate();
  if (!component.errors().isEmpty()) {
    const QString text = errors_text();
    delete object;
    if (why != nullptr) *why = text;
    return nullptr;
  }
  return object;
}

#endif  // LESSON_SOLUTION_HPP
