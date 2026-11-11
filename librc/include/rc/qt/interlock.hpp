// rc/qt/interlock.hpp
//
// An enable switch and the interlock behind it, from lesson 12-08, graduated.
//
// A held key made five release and press pairs marked as auto-repeat, and a handler that took each release as the end of the hold saw it let go five times; the robot was allowed speed only with the key held, the link alive and the stop clear; a link that came back with the key still held did not move it; the stop latched, and a reset while the key was held was refused.

#ifndef RC_QT_INTERLOCK_HPP
#define RC_QT_INTERLOCK_HPP

#include <QtCore/Qt>
#include <QtGui/QKeyEvent>
#include <cmath>

namespace rc {
namespace qt {

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
    if (event.key() != key_) return;
    if (event.isAutoRepeat()) return;
    held_ = event.type() == QEvent::KeyPress;
  }

  // The window lost focus: the release of the key will go somewhere else, so the hold is over.
  void on_focus_lost() { held_ = false; }

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
    enable_ = held;
    if (!held) armed_ = false;
    else if (link_alive_ && !estop_ && !latched_) armed_ = true;
  }

  // The link is alive or not. Losing it disarms. Getting it back does not arm: only a press does.
  void set_link_alive(bool alive) {
    link_alive_ = alive;
    if (!alive) armed_ = false;
  }

  // The emergency stop is pressed or not. Pressing it latches: letting go of it does not clear the latch.
  void set_estop(bool pressed) {
    estop_ = pressed;
    if (pressed) {
      latched_ = true;
      armed_ = false;
    }
  }

  // Clears the latch. Refused (false) while the emergency stop is pressed or the enable key is held: a reset must not be a start.
  bool reset() {
    if (estop_ || enable_) return false;
    latched_ = false;
    return true;
  }

  bool latched() const { return latched_; }
  bool armed() const { return armed_; }

  // The speed the robot may be given for a request, in metres a second: zero unless armed (and for a request that is not a finite number), otherwise the request held between
  // minus the limit and the limit.
  double permitted(double requested) const {
    if (!armed_ || !std::isfinite(requested)) return 0.0;
    if (requested > limit_) return limit_;
    if (requested < -limit_) return -limit_;
    return requested;
  }

 private:
  double limit_;
  bool enable_ = false;
  bool link_alive_ = false;
  bool estop_ = false;
  bool latched_ = false;
  bool armed_ = false;
};
}  // namespace qt
}  // namespace rc

#endif  // RC_QT_INTERLOCK_HPP
