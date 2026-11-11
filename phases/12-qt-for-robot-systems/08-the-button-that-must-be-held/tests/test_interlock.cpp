// The framework's main is suppressed because Qt Gui is happier with an application object.
#define RC_TEST_NO_MAIN
#include <rc/test/rc_test.hpp>

#include <QtCore/Qt>
#include <QtGui/QGuiApplication>
#include <QtGui/QKeyEvent>

#include <cmath>
#include <iostream>

#include "solution.hpp"

namespace {

QKeyEvent press(int key, bool repeat = false) { return QKeyEvent(QEvent::KeyPress, key, Qt::NoModifier, QString(), repeat); }
QKeyEvent release(int key, bool repeat = false) { return QKeyEvent(QEvent::KeyRelease, key, Qt::NoModifier, QString(), repeat); }

}  // namespace

RC_TEST("holding a key sends releases that are repeats, and a handler that believes them sees the key let go") {
  // What a held key sends on a platform that reports a repeat as a release and a press: a press, then pairs of release and press, all marked as auto-repeat, then the real release.
  const QKeyEvent events[] = {press(Qt::Key_Space),         release(Qt::Key_Space, true), press(Qt::Key_Space, true), release(Qt::Key_Space, true), press(Qt::Key_Space, true),
                              release(Qt::Key_Space, true), press(Qt::Key_Space, true),   release(Qt::Key_Space, true), press(Qt::Key_Space, true),   release(Qt::Key_Space, true),
                              press(Qt::Key_Space, true),   release(Qt::Key_Space, false)};
  bool naive_held = false;
  int naive_lets_go = 0;
  DeadmanKey key(Qt::Key_Space);
  int lets_go = 0;
  bool was_held = false;
  for (const QKeyEvent& e : events) {
    const bool before = naive_held;
    naive_held = e.type() == QEvent::KeyPress;
    if (before && !naive_held && &e != &events[11]) ++naive_lets_go;
    key.on_key(e);
    if (was_held && !key.held() && &e != &events[11]) ++lets_go;
    was_held = key.held();
  }
  std::cout << "  a held space bar: a handler that takes every release as the end saw it let go " << naive_lets_go << " times before the real release; this one " << lets_go << " times\n";
  RC_CHECK_EQ(naive_lets_go, 5);
  RC_CHECK_EQ(lets_go, 0);
  RC_CHECK(!key.held());                          // and the real release at the end ended it

  DeadmanKey held(Qt::Key_Space);
  held.on_key(press(Qt::Key_Space));
  RC_CHECK(held.held());
  held.on_key(release(Qt::Key_Space, true));
  RC_CHECK(held.held());                          // a repeat is not a release
  held.on_key(release(Qt::Key_Space, false));
  RC_CHECK(!held.held());
  held.on_key(press(Qt::Key_Space));
  held.on_key(press(Qt::Key_A));                  // another key
  held.on_key(release(Qt::Key_A));
  RC_CHECK(held.held());
}

RC_TEST("losing focus ends the hold, and the key must be pressed again") {
  DeadmanKey key(Qt::Key_Shift);
  key.on_key(press(Qt::Key_Shift));
  RC_CHECK(key.held());
  key.on_focus_lost();
  RC_CHECK(!key.held());
  std::cout << "  focus lost while held: the key is " << (key.held() ? "still held" : "not held") << "\n\n";
  key.on_key(release(Qt::Key_Shift));              // the release that goes to this window later changes nothing
  RC_CHECK(!key.held());
  key.on_key(press(Qt::Key_Shift));
  RC_CHECK(key.held());
}

RC_TEST("speed is allowed only with the key held, the link alive and the stop clear, and is held within the limit") {
  int allowed = 0;
  int total = 0;
  for (int held = 0; held < 2; ++held) {
    for (int alive = 0; alive < 2; ++alive) {
      for (int stop = 0; stop < 2; ++stop) {
        Interlock i(1.5);
        i.set_link_alive(alive != 0);
        i.set_estop(stop != 0);
        i.set_enable(held != 0);
        ++total;
        if (i.permitted(1.0) != 0.0) ++allowed;
        const bool should = held && alive && !stop;
        RC_CHECK_EQ(i.permitted(1.0) != 0.0, should);
      }
    }
  }
  std::cout << "  " << total << " combinations of key, link and stop: speed allowed in " << allowed << "\n";
  RC_CHECK_EQ(total, 8);
  RC_CHECK_EQ(allowed, 1);

  Interlock i(1.5);
  i.set_link_alive(true);
  i.set_enable(true);
  RC_CHECK_NEAR(i.permitted(1.0), 1.0, 1e-12);
  RC_CHECK_NEAR(i.permitted(9.0), 1.5, 1e-12);        // held to the limit
  RC_CHECK_NEAR(i.permitted(-9.0), -1.5, 1e-12);
  RC_CHECK_NEAR(i.permitted(0.0), 0.0, 1e-12);
  RC_CHECK_NEAR(i.permitted(std::nan("")), 0.0, 1e-12);       // a request that is not a number is no request
  RC_CHECK_NEAR(i.permitted(HUGE_VAL), 0.0, 1e-12);
  Interlock negative(-3.0);
  negative.set_link_alive(true);
  negative.set_enable(true);
  RC_CHECK_NEAR(negative.permitted(1.0), 0.0, 1e-12);         // a limit below nothing is nothing
}

RC_TEST("a link that comes back while the key is still held does not move the robot, and neither does a key held from before the link was good") {
  Interlock i(1.0);
  i.set_link_alive(true);
  i.set_enable(true);
  RC_CHECK_NEAR(i.permitted(1.0), 1.0, 1e-12);
  i.set_link_alive(false);                             // the link is lost, the key is still held
  RC_CHECK_NEAR(i.permitted(1.0), 0.0, 1e-12);
  i.set_link_alive(true);                              // and comes back, the key is still held
  const double after_return = i.permitted(1.0);
  std::cout << "  link lost and back with the key held: allowed speed " << after_return << "\n";
  RC_CHECK_NEAR(after_return, 0.0, 1e-12);            // it does not start by itself
  i.set_enable(false);
  i.set_enable(true);                                  // let go and press again
  RC_CHECK_NEAR(i.permitted(1.0), 1.0, 1e-12);

  Interlock early(1.0);
  early.set_enable(true);                              // the key was down before the link was good
  early.set_link_alive(true);
  RC_CHECK_NEAR(early.permitted(1.0), 0.0, 1e-12);
  early.set_enable(false);
  early.set_enable(true);
  RC_CHECK_NEAR(early.permitted(1.0), 1.0, 1e-12);
}

RC_TEST("an emergency stop latches, letting go of it does not clear it, and a reset is refused while the key is held") {
  Interlock i(1.0);
  i.set_link_alive(true);
  i.set_enable(true);
  RC_CHECK_NEAR(i.permitted(1.0), 1.0, 1e-12);
  i.set_estop(true);
  RC_CHECK_NEAR(i.permitted(1.0), 0.0, 1e-12);
  RC_CHECK(i.latched());
  RC_CHECK(!i.reset());                                // the stop is pressed
  i.set_estop(false);                                  // the button is let go
  RC_CHECK(i.latched());                               // and the latch stays
  RC_CHECK_NEAR(i.permitted(1.0), 0.0, 1e-12);
  const bool reset_while_held = i.reset();
  std::cout << "  stop pressed and let go with the key still held: latched " << i.latched() << ", a reset is " << (reset_while_held ? "allowed" : "refused") << "\n\n";
  RC_CHECK(!reset_while_held);                         // a reset must not be a start
  RC_CHECK_NEAR(i.permitted(1.0), 0.0, 1e-12);
  i.set_enable(false);
  RC_CHECK(i.reset());
  RC_CHECK(!i.latched());
  RC_CHECK_NEAR(i.permitted(1.0), 0.0, 1e-12);         // reset does not arm
  i.set_enable(true);
  RC_CHECK_NEAR(i.permitted(1.0), 1.0, 1e-12);         // pressing the key does
}

int main(int argc, char** argv) {
  qputenv("QT_QPA_PLATFORM", "offscreen");
  QGuiApplication app(argc, argv);
  return rc::test::run_all();
}
