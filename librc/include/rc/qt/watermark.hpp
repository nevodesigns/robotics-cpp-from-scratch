// rc/qt/watermark.hpp
//
// The battery monitor from lesson 09-01 and the threshold sensor from 09-02,
// graduated.
//
// Both lessons wrote the same idea in opposite directions, so both are here
// under the names that say which direction they are: a low watermark for
// battery, fuel and signal strength, a high watermark for temperature, current
// and speed. Merging them into one class with a direction flag was the other
// option and it reads worse at every call site, where "low battery" is the
// thing being said.
//
// The point of both is the second threshold. A single threshold emits on every
// flicker across it, and a value hovering exactly at the limit produces a storm
// of alerts that operators learn to ignore, which is worse than no alert. The
// gap between the two thresholds is how much the value must recover before the
// condition is declared over.

#ifndef RC_QT_WATERMARK_HPP
#define RC_QT_WATERMARK_HPP

#include <QObject>

namespace rc {
namespace qt {

// Enters at or below enter_at, clears at or above clear_at.
//
// clear_at should be the higher of the two. Passing them the wrong way round
// leaves the object permanently in whichever state it first reaches, which is
// visible in the tests rather than diagnosed at construction, because a
// constructor that reorders its arguments to be helpful is a constructor that
// hides a caller's mistake.
class LowWatermark : public QObject {
  // Q_OBJECT must come first in the class body. It declares the meta object
  // machinery and the signal bodies; moc writes the definitions into a separate
  // generated file, which is why a missing macro shows up at link time rather
  // than at compile time.
  Q_OBJECT

 public:
  explicit LowWatermark(double enter_at, double clear_at, QObject* parent = nullptr)
      : QObject(parent), enter_at_(enter_at), clear_at_(clear_at) {}

  void update(double value) {
    last_ = value;

    if (!active_ && value <= enter_at_) {
      active_ = true;
      emit entered(value);
      return;
    }

    if (active_ && value >= clear_at_) {
      active_ = false;
      emit cleared(value);
    }

    // Everything else is a reading inside the band, or one that does not change
    // the state. Emitting nothing is the correct behaviour, not an omission.
  }

  bool active() const { return active_; }
  double last() const { return last_; }

 signals:
  void entered(double value);
  void cleared(double value);

 private:
  double enter_at_ = 0.0;
  double clear_at_ = 0.0;
  double last_ = 0.0;
  bool active_ = false;
};

// Enters at or above enter_at, clears at or below clear_at.
//
// Lesson 09-02 announced only the upward crossing and rearmed silently. This
// emits both, because a caller that does not connect cleared() pays nothing,
// and one that needs to know when an over temperature ended would otherwise
// have to poll.
class HighWatermark : public QObject {
  Q_OBJECT

 public:
  explicit HighWatermark(double enter_at, double clear_at, QObject* parent = nullptr)
      : QObject(parent), enter_at_(enter_at), clear_at_(clear_at) {}

  void update(double value) {
    last_ = value;

    if (!active_ && value >= enter_at_) {
      active_ = true;
      emit entered(value);
      return;
    }

    if (active_ && value <= clear_at_) {
      active_ = false;
      emit cleared(value);
    }
  }

  bool active() const { return active_; }
  double last() const { return last_; }

 signals:
  void entered(double value);
  void cleared(double value);

 private:
  double enter_at_ = 0.0;
  double clear_at_ = 0.0;
  double last_ = 0.0;
  bool active_ = false;
};

}  // namespace qt
}  // namespace rc

#endif  // RC_QT_WATERMARK_HPP
