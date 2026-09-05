// A QObject needs an application object to exist before it does, so the test
// framework's own main is suppressed and one is written at the bottom.
#define RC_TEST_NO_MAIN
#include <rc/test/rc_test.hpp>

#include <QtCore/QCoreApplication>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QThread>

#include <atomic>
#include <iomanip>
#include <iostream>

#include "solution.hpp"

namespace {

// How many of these exist right now. A destructor that did not run is the thing
// this lesson is about, and a counter is the only way to see it.
int alive = 0;

class Counted : public QObject {
 public:
  explicit Counted(QObject* parent = nullptr) : QObject(parent) { ++alive; }
  ~Counted() override { --alive; }
};

// A worker that creates an object on its own thread, so the test has something
// living somewhere else.
class Maker : public QObject {
 public:
  // Atomic because two threads look at it. The thread sanitizer is right about
  // this and lesson 07-01 is where that was learned.
  std::atomic<Counted*> made{nullptr};

  void make() { made.store(new Counted()); }
};

}  // namespace

RC_TEST("a parent destroys its children, and a child leaving does not surprise it") {
  RC_REQUIRE_EQ(alive, 0);

  {
    Counted* parent = new Counted();
    new Counted(parent);
    new Counted(parent);

    RC_CHECK_EQ(alive, 3);
    RC_CHECK_EQ(static_cast<int>(parent->children().size()), 2);

    delete parent;
    RC_CHECK_EQ(alive, 0);
  }

  // A child deleted on its own takes itself off the parent's list, so the
  // parent has nothing to delete twice. That is why deleting a child early is
  // safe and giving a stack object a parent is not: the stack object's
  // destructor runs at the closing brace whatever Qt thinks.
  {
    Counted* parent = new Counted();
    Counted* child = new Counted(parent);
    RC_CHECK_EQ(static_cast<int>(parent->children().size()), 1);

    delete child;
    RC_CHECK_EQ(static_cast<int>(parent->children().size()), 0);
    RC_CHECK_EQ(alive, 1);

    delete parent;
    RC_CHECK_EQ(alive, 0);
  }
}

RC_TEST("deleteLater deletes nothing, and processEvents does not either") {
  RC_REQUIRE_EQ(alive, 0);

  Counted* orphan = new Counted();
  QPointer<Counted> watch(orphan);

  std::cout << "\n    " << std::left << std::setw(46) << "after new" << std::right
            << alive << " alive\n";

  orphan->deleteLater();
  std::cout << "    " << std::left << std::setw(46) << "after deleteLater"
            << std::right << alive << " alive\n";
  RC_CHECK_EQ(alive, 1);
  RC_CHECK(still_alive(watch));

  QCoreApplication::processEvents();
  std::cout << "    " << std::left << std::setw(46) << "after processEvents"
            << std::right << alive << " alive\n";
  RC_CHECK_EQ(alive, 1);
  RC_CHECK(still_alive(watch));

  drain_deferred_deletes();
  std::cout << "    " << std::left << std::setw(46)
            << "after sendPostedEvents(DeferredDelete)" << std::right << alive
            << " alive\n";
  RC_CHECK_EQ(alive, 0);
  RC_CHECK(!still_alive(watch));

  std::cout << "\n    deferred deletions are held back from processEvents on\n";
  std::cout << "    purpose, so that an object cannot be destroyed while a\n";
  std::cout << "    nested loop is still inside one of its own functions. A\n";
  std::cout << "    test that calls processEvents and expects it gone finds it\n";
  std::cout << "    still there, with nothing reporting anything\n";
}

RC_TEST("a pointer that knows its object has gone") {
  RC_REQUIRE_EQ(alive, 0);

  Counted* object = new Counted();
  QPointer<Counted> watching(object);
  Counted* raw = object;

  RC_CHECK(still_alive(watching));
  RC_CHECK(watching.data() == raw);

  delete object;

  // The QPointer is null. The raw pointer is not, and there is no way to ask it
  // anything: it holds an address that used to mean something.
  RC_CHECK(!still_alive(watching));
  RC_CHECK(watching.isNull());
  RC_CHECK(raw != nullptr);
  RC_CHECK_EQ(alive, 0);

  // It works through a parent too, which is the case that matters: somebody
  // else owns the object and may destroy it without telling you.
  Counted* parent = new Counted();
  QPointer<Counted> child(new Counted(parent));
  RC_CHECK(still_alive(child));
  delete parent;
  RC_CHECK(!still_alive(child));
  RC_CHECK_EQ(alive, 0);

  // A null QPointer is simply not alive, rather than a special case.
  const QPointer<Counted> nothing;
  RC_CHECK(!still_alive(nothing));
}

RC_TEST("reparenting moves who is responsible") {
  RC_REQUIRE_EQ(alive, 0);

  Counted* first = new Counted();
  Counted* second = new Counted();
  Counted* child = new Counted(first);
  QPointer<Counted> watching(child);

  RC_CHECK_EQ(static_cast<int>(first->children().size()), 1);
  RC_CHECK_EQ(static_cast<int>(second->children().size()), 0);

  child->setParent(second);
  RC_CHECK_EQ(static_cast<int>(first->children().size()), 0);
  RC_CHECK_EQ(static_cast<int>(second->children().size()), 1);

  // Deleting the old parent no longer takes the child with it, which is the
  // point and is also how an object outlives the scope somebody expected.
  delete first;
  RC_CHECK(still_alive(watching));
  RC_CHECK_EQ(alive, 2);

  delete second;
  RC_CHECK(!still_alive(watching));
  RC_CHECK_EQ(alive, 0);
}

RC_TEST("an object is destroyed by the thread it lives in") {
  RC_REQUIRE_EQ(alive, 0);

  QThread thread;
  Maker maker;
  maker.moveToThread(&thread);

  // Make the object on the worker thread, so it belongs there.
  QObject::connect(&thread, &QThread::started, &maker, [&maker] { maker.make(); });
  thread.start();
  while (maker.made.load() == nullptr) QThread::msleep(1);

  Counted* made = maker.made.load();
  QPointer<Counted> watching(made);
  RC_CHECK_EQ(alive, 1);
  RC_CHECK(made->thread() == &thread);
  RC_CHECK(made->thread() != QThread::currentThread());

  // From here, deleting it directly would touch the connection lists and event
  // queue that the other thread owns. So the deletion is posted to that thread
  // instead, and this one carries on.
  delete_from_its_own_thread(made);
  RC_CHECK(still_alive(watching));   // nothing has happened yet

  // The worker's own loop is what performs it.
  thread.quit();
  thread.wait();
  RC_CHECK(!still_alive(watching));
  RC_CHECK_EQ(alive, 0);

  std::cout << "\n    the object was made on the worker thread and destroyed by\n";
  std::cout << "    it, from a request posted by this one. deleteLater is not a\n";
  std::cout << "    convenience here, it is the only correct answer\n";
}

RC_TEST("on its own thread it is deleted at once") {
  RC_REQUIRE_EQ(alive, 0);

  Counted* here = new Counted();
  QPointer<Counted> watching(here);
  RC_CHECK(here->thread() == QThread::currentThread());

  delete_from_its_own_thread(here);

  // No event loop was needed and none was run: this thread owns it, so the
  // deletion happened on the spot.
  RC_CHECK(!still_alive(watching));
  RC_CHECK_EQ(alive, 0);

  // And nothing at all is not an error.
  delete_from_its_own_thread(nullptr);
  RC_CHECK_EQ(alive, 0);
}

int main(int argc, char** argv) {
  qputenv("QT_QPA_PLATFORM", "offscreen");
  QCoreApplication app(argc, argv);
  return rc::test::run_all();
}
