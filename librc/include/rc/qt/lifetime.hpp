// rc/qt/lifetime.hpp
//
// The three things needed to destroy a Qt object at the right time, on the
// right thread, and know that it happened, from lesson 09-04.
//
// deleteLater deletes nothing. It posts an event, and processEvents does not
// run it: deferred deletions are held back from processEvents on purpose, so
// that an object cannot be destroyed while a nested loop is still inside one of
// its own functions. Measured, on an object that has had deleteLater called:
//
//   after deleteLater                        1 alive
//   after processEvents                      1 alive
//   after sendPostedEvents(DeferredDelete)   0 alive
//
// A test that calls processEvents and expects the object gone finds it still
// there, with nothing anywhere reporting anything.

#ifndef RC_QT_LIFETIME
#define RC_QT_LIFETIME

#include <QtCore/QCoreApplication>
#include <QtCore/QEvent>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QThread>

namespace rc {
namespace qt {

// Run the deletions that deleteLater has posted.
//
// deleteLater does not delete anything. It posts an event, and that event is
// only acted on when an event loop gets to it, which is exactly the thing a
// test usually does not have.
//
// And processEvents does not do it. Deferred deletions are held back from
// processEvents deliberately, so that an object cannot be destroyed while a
// nested loop is still inside a function belonging to it. The consequence is
// that a test which calls processEvents and expects the object gone finds it
// still there, with no error anywhere.
//
// Measured: after deleteLater, one alive. After processEvents, one alive. After
// this, none.
inline void drain_deferred_deletes(QObject* only = nullptr) {
  QCoreApplication::sendPostedEvents(only, QEvent::DeferredDelete);
}

// Destroy an object from whichever thread it belongs to.
//
// A QObject may only be deleted by the thread it lives in, because its
// destructor touches the connection lists and the event queue that thread owns.
// Deleting one from elsewhere is a race that usually looks like nothing at all
// until the day it looks like a crash inside Qt.
//
// If this is that thread, delete it now, which is simple and immediate. If it
// is not, post the deletion to the thread that owns it, which is what
// deleteLater is for and the one place it is not optional.
inline void delete_from_its_own_thread(QObject* object) {
  if (object == nullptr) return;

  if (object->thread() == QThread::currentThread()) {
    delete object;
    return;
  }
  object->deleteLater();
}

// Whether a pointer still refers to something.
//
// A raw pointer to a destroyed QObject is dangling and there is no way to ask
// it anything. A QPointer is told when its object is destroyed and becomes
// null, which is the only way to hold a reference to something you do not own
// and still be able to check.
//
// It is the same idea as the weak_ptr in lesson 02-06, arrived at from the
// other direction: there, ownership is shared and a weak reference does not
// keep the object alive. Here, ownership belongs to a parent and a QPointer
// does not pretend otherwise.
template <class T>
bool still_alive(const QPointer<T>& pointer) {
  return !pointer.isNull();
}

}  // namespace qt
}  // namespace rc

#endif  // RC_QT_LIFETIME
