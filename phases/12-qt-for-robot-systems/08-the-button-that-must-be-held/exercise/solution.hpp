#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <QtCore/Qt>
#include <QtGui/QKeyEvent>
#include <cmath>

// A robot that is driven from a station should move only while a person is holding the means to stop it. The usual form is an enable switch: a key or a button that has to be held
// for the robot to move, and that stops it the moment it is let go. That is simple to say and has three ways to be wrong.
//
// The first is the keyboard. When a key is held down the system repeats it, and on some platforms a repeat is delivered as a release followed by a press, both marked as auto-repeat.
// A handler that takes every release as the end of the hold sees the key let go several times a second while it is held. The second is everything else that can end a hold without a
// release: the window loses focus, and the key release goes to another window. And the third is the one that matters most: when a reason to stop goes away, the robot must not move
// again by itself. A link that comes back while the key is still held is not permission to start. The person has to let go and press again.

// The enable key, as the events of the keyboard report it.
class DeadmanKey {
 public:
  explicit DeadmanKey(int key) : key_(key) {}

  // A press starts the hold and a release ends it, and the repeats the system makes of a held key (marked as auto-repeat) are neither. Other keys are not ours.
  void on_key(const QKeyEvent& event) {
    // TODO: return if event.key() is not key_. Return if event.isAutoRepeat() (a repeat of a held key is neither a press nor a release). Otherwise
    // held_ = (event.type() == QEvent::KeyPress).
    (void)event;
    (void)key_;
  }

  // The window lost focus: the release of the key will go somewhere else, so the hold is over.
  void on_focus_lost() {
    // TODO: held_ = false.
  }

  bool held() const { return held_; }

 private:
  int key_;
  bool held_ = false;
};

// What the station allows the robot to do. It allows speed only while the enable key is held, the link is alive and no emergency stop has been pressed, and not after any of them
// has been lost until the person has let go of the key and pressed it again.
class Interlock {
 public:
  explicit Interlock(double limit_mps) : limit_(limit_mps < 0.0 ? 0.0 : limit_mps) {}

  // The enable key is held or not. A press arms the interlock only if the link is alive and the emergency stop is clear and not latched; a release disarms it.
  void set_enable(bool held) {
    // TODO: enable_ = held. A release disarms: armed_ = false. A press arms (armed_ = true) only if link_alive_ and not estop_ and not latched_.
    enable_ = held;
  }

  // The link is alive or not. Losing it disarms. Getting it back does not arm: only a press does.
  void set_link_alive(bool alive) {
    // TODO: link_alive_ = alive; if it is not alive, armed_ = false.
    link_alive_ = alive;
  }

  // The emergency stop is pressed or not. Pressing it latches: letting go of it does not clear the latch.
  void set_estop(bool pressed) {
    // TODO: estop_ = pressed. If pressed: latched_ = true and armed_ = false.
    estop_ = pressed;
  }

  // Clears the latch. Refused (false) while the emergency stop is pressed or the enable key is held: a reset must not be a start.
  bool reset() {
    // TODO: false if estop_ or enable_. Otherwise latched_ = false and true.
    return false;
  }

  bool latched() const { return latched_; }
  bool armed() const { return armed_; }

  // The speed the robot may be given for a request, in metres a second: zero unless armed (and for a request that is not a finite number), otherwise the request held between
  // minus the limit and the limit.
  double permitted(double requested) const {
    // TODO: 0.0 if not armed_ or requested is not finite (std::isfinite). Otherwise requested, held between -limit_ and limit_.
    (void)requested;
    (void)limit_;
    return 99.0;
  }

 private:
  double limit_;
  bool enable_ = false;
  bool link_alive_ = false;
  bool estop_ = false;
  bool latched_ = false;
  bool armed_ = false;
};

#endif  // LESSON_SOLUTION_HPP
