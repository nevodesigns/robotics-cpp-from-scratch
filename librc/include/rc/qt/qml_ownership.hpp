// rc/qt/qml_ownership.hpp
//
// Owning what QML is given, from lesson 11-04, graduated.
//
// An unparented object returned from an invokable was deleted by the garbage collector once the JavaScript value was gone, and a C++ pointer to it was dangling; with a parent, or with the engine told C++ owns it, it survived the same collection, and
// an object read from a property was never collected.

#ifndef RC_QT_QML_OWNERSHIP_HPP
#define RC_QT_QML_OWNERSHIP_HPP

#include <QtCore/QObject>
#include <QtQml/QQmlEngine>
#include <cstddef>

namespace rc {
namespace qt {

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
}  // namespace qt
}  // namespace rc

#endif  // RC_QT_QML_OWNERSHIP_HPP
