#define RC_TEST_NO_MAIN
#include <rc/test/rc_test.hpp>

#include <QtCore/QCoreApplication>
#include <QtCore/QElapsedTimer>
#include <QtCore/QEventLoop>
#include <QtCore/QTimer>

#include <iostream>
#include <vector>

#include "solution.hpp"

namespace {

// Holds the thread for `ms` without letting the event loop run: a handler that does real work.
void busy_for(int ms) {
  QElapsedTimer t;
  t.start();
  while (t.elapsed() < ms) {
  }
}

// Runs the event loop for `ms` and then returns.
void run_for(int ms) {
  QEventLoop loop;
  QTimer::singleShot(ms, &loop, &QEventLoop::quit);
  loop.exec();
}

}  // namespace

RC_TEST("a slot is the number of whole periods since the start, and the delay is the time left in the current one") {
  const long long ms = 1000000;
  RC_CHECK_EQ(slot_at(0, 10 * ms, 0), 0);
  RC_CHECK_EQ(slot_at(0, 10 * ms, 9 * ms + 999999), 0);
  RC_CHECK_EQ(slot_at(0, 10 * ms, 10 * ms), 1);
  RC_CHECK_EQ(slot_at(0, 10 * ms, 25 * ms), 2);
  RC_CHECK_EQ(slot_at(5 * ms, 10 * ms, 25 * ms), 2);        // counted from the start, not from zero
  RC_CHECK_EQ(slot_at(5 * ms, 10 * ms, 3 * ms), 0);         // before the start
  RC_CHECK_EQ(slot_at(0, 0, 25 * ms), 0);                   // a period that is not positive

  RC_CHECK_EQ(delay_to_next_ms(0, 10 * ms, 0), 10);
  RC_CHECK_EQ(delay_to_next_ms(0, 10 * ms, 3 * ms), 7);
  RC_CHECK_EQ(delay_to_next_ms(0, 10 * ms, 3 * ms + 200000), 7);   // 6.8 ms left, rounded up so the timer is never early
  RC_CHECK_EQ(delay_to_next_ms(0, 10 * ms, 10 * ms), 10);          // exactly on a boundary: the next one is a whole period away
  RC_CHECK_EQ(delay_to_next_ms(0, 10 * ms, 29 * ms + 500000), 1);  // half a millisecond left: never less than 1
  RC_CHECK_EQ(delay_to_next_ms(0, 0, 5), 1);
}

RC_TEST("a ticker ticks on its grid, and every slot up to the last is a tick or a missed one") {
  Ticker ticker(10);
  std::vector<long long> seen;
  QObject::connect(&ticker, &Ticker::tick, [&](long long slot, long long) {
    seen.push_back(slot);
    busy_for(3);                      // a handler that takes 3 ms of every 10
  });
  ticker.start();
  run_for(300);
  ticker.stop();

  std::cout << "  a 10 ms ticker, a 3 ms handler, 300 ms: " << ticker.ticks() << " ticks, " << ticker.missed() << " missed, last slot "
            << ticker.last_slot() << "\n\n";
  RC_CHECK(ticker.ticks() >= 5);
  RC_CHECK_EQ(ticker.ticks() + ticker.missed(), ticker.last_slot());
  RC_CHECK_EQ(static_cast<long long>(seen.size()), ticker.ticks());
  for (std::size_t i = 1; i < seen.size(); ++i) RC_CHECK(seen[i] > seen[i - 1]);   // never two ticks in one slot
}

RC_TEST("a handler that blocks for 100 ms makes the ticker skip the slots and not burst to catch up") {
  Ticker ticker(10);
  std::vector<long long> seen;
  long long reported_missed = 0;
  QObject::connect(&ticker, &Ticker::tick, [&](long long slot, long long missed) {
    seen.push_back(slot);
    reported_missed += missed;
    if (seen.size() == 3) busy_for(100);
  });
  ticker.start();
  run_for(400);
  ticker.stop();

  std::cout << "  a 10 ms ticker, one block of 100 ms: " << ticker.ticks() << " ticks, " << ticker.missed() << " missed, last slot "
            << ticker.last_slot() << "\n\n";
  RC_CHECK(ticker.ticks() >= 3);
  RC_CHECK(ticker.missed() >= 5);                                   // about 9 seen went by with nobody there
  RC_CHECK_EQ(reported_missed, ticker.missed());                    // every one was reported to a tick, none were lost
  RC_CHECK_EQ(ticker.ticks() + ticker.missed(), ticker.last_slot());
  for (std::size_t i = 1; i < seen.size(); ++i) RC_CHECK(seen[i] > seen[i - 1]);
}

RC_TEST("a timer re-armed in its handler for 10 ms ticks every 13 when the handler takes 3") {
  QEventLoop loop;
  QTimer rearmed;
  rearmed.setSingleShot(true);
  int rearmed_ticks = 0;
  QObject::connect(&rearmed, &QTimer::timeout, [&] {
    ++rearmed_ticks;
    busy_for(3);
    rearmed.start(10);                // "10 ms from now", counted from the end of the work
  });
  Ticker ticker(10);
  QObject::connect(&ticker, &Ticker::tick, [&](long long, long long) { busy_for(3); });

  QTimer::singleShot(300, &loop, &QEventLoop::quit);
  rearmed.start(10);
  ticker.start();
  loop.exec();
  ticker.stop();
  rearmed.stop();

  std::cout << "  300 ms, a 3 ms handler: the re-armed timer ticked " << rearmed_ticks << " times; the grid ticker reached slot "
            << ticker.last_slot() << " with " << ticker.ticks() << " ticks and " << ticker.missed() << " missed\n\n";
  RC_CHECK(rearmed_ticks <= 24);                                   // 300 / 13 is 23.1: each cycle is at least 13 ms, so it cannot do more
  RC_CHECK(ticker.ticks() >= 5);
  RC_CHECK_EQ(ticker.ticks() + ticker.missed(), ticker.last_slot());
  RC_CHECK(ticker.last_slot() >= rearmed_ticks);                   // the grid does not fall behind the schedule
}

RC_TEST("counting ticks as time is short by every tick that a block lost") {
  QElapsedTimer wall;
  QTimer naive;
  naive.setInterval(10);
  int naive_ticks = 0;
  Ticker ticker(10);
  long long worst_overshoot_ms = -1000000;
  QObject::connect(&naive, &QTimer::timeout, [&] {
    ++naive_ticks;
    if (naive_ticks == 3) busy_for(100);
  });
  QObject::connect(&ticker, &Ticker::tick, [&](long long slot, long long) {
    const long long overshoot = slot * 10 - wall.elapsed();       // the grid's time minus the real time: never ahead of it
    if (overshoot > worst_overshoot_ms) worst_overshoot_ms = overshoot;
  });

  QEventLoop loop;
  QTimer::singleShot(400, &loop, &QEventLoop::quit);
  wall.start();
  naive.start();
  ticker.start();
  loop.exec();
  const long long real_ms = wall.elapsed();
  ticker.stop();
  naive.stop();

  const long long counted_ms = static_cast<long long>(naive_ticks) * 10;
  std::cout << "  after " << real_ms << " ms of real time, " << naive_ticks << " ticks of 10 ms count as " << counted_ms << " ms: short by "
            << real_ms - counted_ms << " ms. The grid says " << ticker.last_slot() * 10 << " ms\n\n";
  RC_CHECK(ticker.ticks() >= 3);
  RC_CHECK(real_ms - counted_ms >= 50);                            // the 100 ms block took about nine ticks with it
  RC_CHECK(worst_overshoot_ms <= 1);                                // a slot's time is never later than the clock
  RC_CHECK(ticker.last_slot() * 10 + 40 >= counted_ms);           // and the grid's time is at least what counting gave
}

int main(int argc, char** argv) {
  qputenv("QT_QPA_PLATFORM", "offscreen");
  QCoreApplication app(argc, argv);
  return rc::test::run_all();
}
