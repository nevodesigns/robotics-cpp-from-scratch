#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <QElapsedTimer>
#include <QObject>
#include <QTimer>

// A QTimer with an interval of 10 ms does not tick every 10 ms. It ticks when the event loop next gets to it, at least 10 ms after it was
// armed. If the loop is busy for 100 ms the ticks in that time are not delivered later in a burst: they are gone, and nothing says so.
// A timer re-armed from its own handler is later every time by however long the handler took. And a program that counts ticks to know the
// time is wrong by every tick that was lost.
//
// The way out is to keep a grid. Slot 0 is the start, slot 1 is one period after it, and so on. When a tick is delivered, the slot it
// belongs to is read from the clock, so the ticker knows how many slots went by without one.

// The slot that the time `now_ns` falls in: the number of whole periods since `start_ns`. Before the start, or with a period that is not
// positive, it is 0.
inline long long slot_at(long long start_ns, long long period_ns, long long now_ns) {
  // TODO: (now_ns - start_ns) / period_ns, with whole number division.
  (void)start_ns;
  (void)period_ns;
  (void)now_ns;
  return -1;
}

// Whole milliseconds until the next slot begins, rounded UP so that the timer never fires early, and never less than 1.
inline long long delay_to_next_ms(long long start_ns, long long period_ns, long long now_ns) {
  // TODO: the next boundary is start_ns + (slot_at(...) + 1) * period_ns. The time left is that minus now_ns, in nanoseconds. Round up to whole
  // milliseconds: (left_ns + 999999) / 1000000. Return 1 if that is less than 1, or if the period is not positive.
  (void)start_ns;
  (void)period_ns;
  (void)now_ns;
  return -1;
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
    // TODO: clock_.start(); zero ticks_, missed_ and last_slot_; then timer_.start(delay) where delay is delay_to_next_ms(0, period_ns_, 0),
    // as an int.
    (void)period_ns_;
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
    // TODO: slot = slot_at(0, period_ns_, clock_.nsecsElapsed()). Only if slot > last_slot_ (a timer that fires a hair early lands in the slot
    // already delivered, and that is not a tick): skipped = slot - last_slot_ - 1; add it to missed_; ++ticks_; last_slot_ = slot;
    // emit tick(slot, skipped). Then, always, timer_.start(...) with delay_to_next_ms(0, period_ns_, clock_.nsecsElapsed()) so that the next
    // tick is aimed at the next boundary of the grid, measured after the handler.
  }

 private:
  QTimer timer_;
  QElapsedTimer clock_;
  long long period_ns_;
  long long ticks_ = 0;
  long long missed_ = 0;
  long long last_slot_ = 0;
};

#endif  // LESSON_SOLUTION_HPP
