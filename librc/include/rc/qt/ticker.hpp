// rc/qt/ticker.hpp
//
// A ticker that keeps a fixed grid of slots on a QTimer, from lesson 09-11, graduated.
//
// A timer re-armed in its handler for 10 ms ticked 22 times in 300 ms with a 3 ms handler, where 30 periods went by. A 100 ms block of a 10 ms ticker left 30 ticks
// and 10 missed slots, none of them delivered afterwards, and counting ticks gave 300 or 310 ms in 400 ms of real time. The ticker reports the skipped slots, and ticks() + missed()
// equals last_slot().

#ifndef RC_QT_TICKER
#define RC_QT_TICKER

#include <QElapsedTimer>
#include <QObject>
#include <QTimer>

namespace rc {
namespace qt {

// A QTimer with an interval of 10 ms does not tick every 10 ms. It ticks when the event loop next gets to it, at least 10 ms after it was
// armed. If the loop is busy for 100 ms the ticks in that time are not delivered later in a burst: they are gone, and nothing says so.
// A timer re-armed from its own handler is later every time by however long the handler took. And a program that counts ticks to know the
// time is wrong by every tick that was lost.
//
// The way out is to keep a grid. Slot 0 is the start, slot 1 is one period after it, and so on. When a tick is delivered, the slot it
// belongs to is read from the clock, so the ticker knows how many slots went by without one.

// The slot that the time `now_ns` falls in: the number of whole periods since `start_ns`. Before the start it is 0.
inline long long slot_at(long long start_ns, long long period_ns, long long now_ns) {
  if (period_ns <= 0 || now_ns <= start_ns) return 0;
  return (now_ns - start_ns) / period_ns;
}

// Whole milliseconds until the next slot begins, rounded up so that the timer never fires early, and never less than 1.
inline long long delay_to_next_ms(long long start_ns, long long period_ns, long long now_ns) {
  if (period_ns <= 0) return 1;
  const long long boundary = start_ns + (slot_at(start_ns, period_ns, now_ns) + 1) * period_ns;
  const long long left_ns = boundary - now_ns;
  const long long ms = (left_ns + 999999) / 1000000;
  return ms < 1 ? 1 : ms;
}

class Ticker : public QObject {
  Q_OBJECT

 public:
  explicit Ticker(int period_ms, QObject* parent = nullptr) : QObject(parent), period_ns_(static_cast<long long>(period_ms) * 1000000) {
    timer_.setSingleShot(true);
    timer_.setTimerType(Qt::PreciseTimer);
    connect(&timer_, &QTimer::timeout, this, &Ticker::fire);
  }

  void start() {
    clock_.start();
    ticks_ = 0;
    missed_ = 0;
    last_slot_ = 0;
    timer_.start(static_cast<int>(delay_to_next_ms(0, period_ns_, 0)));
  }
  void stop() { timer_.stop(); }

  // Ticks delivered, slots that went by without one, and the slot of the last tick. After a tick, ticks() + missed() == last_slot().
  long long ticks() const { return ticks_; }
  long long missed() const { return missed_; }
  long long last_slot() const { return last_slot_; }

 signals:
  // `slot` is the slot this tick belongs to; `missed` is how many slots between the last tick and this one had none.
  void tick(long long slot, long long missed);

 private slots:
  void fire() {
    const long long slot = slot_at(0, period_ns_, clock_.nsecsElapsed());
    // A timer that fires a hair early lands in the slot already delivered. That is not a tick, so it is not counted.
    if (slot > last_slot_) {
      const long long skipped = slot - last_slot_ - 1;
      missed_ += skipped;
      ++ticks_;
      last_slot_ = slot;
      emit tick(slot, skipped);
    }
    // Aim at the next boundary from the clock as it is now, after the handler, so that a slow handler does not move the grid.
    timer_.start(static_cast<int>(delay_to_next_ms(0, period_ns_, clock_.nsecsElapsed())));
  }

 private:
  QTimer timer_;
  QElapsedTimer clock_;
  long long period_ns_;
  long long ticks_ = 0;
  long long missed_ = 0;
  long long last_slot_ = 0;
};

}  // namespace qt
}  // namespace rc

#endif  // RC_QT_TICKER
