#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <QtCore/QObject>
#include <QtQml/QQmlEngine>
#include <cstddef>

// QML has a garbage collector, and it can delete a C++ object. When a function that QML calls returns a QObject* that has no parent, the engine takes the object for
// itself: when the JavaScript value that refers to it is no longer reachable, the collector deletes it. A C++ pointer to the same object, kept in a member for later, is
// then dangling, and nothing says so.
//
// The fixes are two and both are about saying who owns the object: give it a parent, or tell the engine that C++ owns it. A pool does both for every object it is given,
// and deletes them all when it is destroyed, so there is exactly one owner and it is in C++.

// Makes `owner` the owner of `object` in both senses Qt has: the parent that deletes it, and the engine's record that the garbage collector must leave it
// alone. Either alone is not enough to be sure: a parent is ignored for an object that was already marked as owned by JavaScript, and an unparented one marked as
// C++ owned is nobody's to delete.
inline void keep_alive(QObject* object, QObject* owner) {
  if (object == nullptr) return;
  object->setParent(owner);
  QQmlEngine::setObjectOwnership(object, QQmlEngine::CppOwnership);
}

// Objects that C++ owns on behalf of QML. Adding hands back the same pointer, so it can be returned from an invokable in one line.
class Pool : public QObject {
 public:
  explicit Pool(QObject* parent = nullptr) : QObject(parent) {}

  QObject* add(QObject* object) {
    keep_alive(object, this);
    return object;
  }

  // How many objects the pool owns.
  std::size_t count() const { return static_cast<std::size_t>(children().size()); }

  // Deletes every object the pool owns.
  void clear() {
    const QObjectList owned = children();
    for (QObject* child : owned) delete child;
  }
};

#endif  // LESSON_SOLUTION_HPP
