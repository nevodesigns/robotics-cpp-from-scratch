#include <rc/test/rc_test.hpp>

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <string>

#include "solution.hpp"

namespace {

struct Rng {
  std::uint32_t state;
  std::uint32_t next() {
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
  }
  double uniform() { return static_cast<double>(next() >> 8) / 16777216.0; }
};

}  // namespace

RC_TEST("a silence longer than the deadline is a miss, reported once, and a message makes the monitor ready to report the next") {
  DeadlineMonitor m(0.1);
  RC_CHECK(m.check(5.0) == 0);                          // nothing is missed before the first message
  m.on_message(0.0);
  m.on_message(0.1);
  m.on_message(0.2);
  RC_CHECK(m.check(0.25) == 0);
  RC_CHECK(m.check(0.30) == 0);                         // exactly the deadline is not a miss
  RC_CHECK(m.check(0.35) == 1);
  RC_CHECK(m.check(0.40) == 0);                         // the same silence is not reported again
  RC_CHECK(m.check(9.00) == 0);
  m.on_message(9.1);
  RC_CHECK(m.check(9.3) == 1);                          // a new silence
  std::cout << "  a stream promised every 0.1 s: " << m.missed() << " misses reported for two silences, longest gap between messages " << std::fixed
            << std::setprecision(1) << m.longest_gap() << " s\n\n";
  RC_CHECK(m.missed() == 2);
  RC_CHECK_NEAR(m.longest_gap(), 8.9, 1e-9);
}

RC_TEST("a publisher that dies is noticed one deadline after its last message, and not before") {
  DeadlineMonitor m(0.25);
  double noticed = -1.0;
  for (int tick = 0; tick <= 200; ++tick) {
    const double now = tick * 0.01;
    if (tick % 10 == 0 && now <= 1.0) m.on_message(now);     // every 0.1 s until 1.0 s
    if (m.check(now) == 1 && noticed < 0.0) noticed = now;
  }
  std::cout << "  last message at 1.0 s, deadline 0.25 s: noticed at " << std::fixed << std::setprecision(2) << noticed << " s\n\n";
  RC_CHECK(noticed > 1.25);
  RC_CHECK(noticed < 1.27);
  RC_CHECK(m.missed() == 1);
}

RC_TEST("a deadline equal to the period cries wolf at the slightest jitter, and one a little longer than the worst gap never does") {
  std::cout << "  a message every 0.10 s, each late or early by up to 0.03 s, 2000 messages\n  deadline (s)   misses reported\n";
  int tight = -1;
  int loose = -1;
  int previous = 1 << 30;
  bool falls = true;
  for (double deadline : {0.10, 0.12, 0.14, 0.17}) {
    Rng rng{23};
    DeadlineMonitor m(deadline);
    double t = 0.0;
    for (int i = 0; i < 2000; ++i) {
      const double nominal = i * 0.10;
      t = nominal + (rng.uniform() * 2.0 - 1.0) * 0.03;
      // The monitor is asked just before the message arrives, as it would be by a timer.
      if (i > 0) m.check(t - 1e-9);
      m.on_message(t);
    }
    std::cout << "  " << std::setw(12) << std::fixed << std::setprecision(2) << deadline << "   " << std::setw(15) << m.missed() << "\n";
    if (m.missed() > previous) falls = false;
    previous = m.missed();
    if (deadline == 0.10) tight = m.missed();
    if (deadline == 0.17) loose = m.missed();
  }
  std::cout << "\n";
  RC_CHECK(tight > 500);
  RC_CHECK(falls);
  RC_CHECK(loose == 0);
}

RC_TEST("a queue of messages that expire hands over only those still good, and counts the ones it threw away") {
  ExpiringQueue q(0.25);
  for (int i = 0; i < 5; ++i) q.push("m" + std::to_string(i), i * 0.1);       // stamps 0.0 to 0.4
  std::string payload;
  double age = 0.0;
  RC_CHECK(q.take(0.5, payload, age));
  std::cout << "  five messages made at 0.0 to 0.4 s, a lifespan of 0.25 s, taken at 0.5 s: got " << payload << " aged " << std::fixed << std::setprecision(2)
            << age << " s; " << q.expired() << " expired\n\n";
  RC_CHECK(payload == "m3");
  RC_CHECK_NEAR(age, 0.2, 1e-9);
  RC_CHECK(q.expired() == 3);
  RC_CHECK(q.take(0.5, payload, age));
  RC_CHECK(payload == "m4");
  RC_CHECK(!q.take(0.5, payload, age));
  ExpiringQueue forever(0.0);
  forever.push("old", 0.0);
  RC_CHECK(forever.take(1000.0, payload, age));
  RC_CHECK(payload == "old");
  RC_CHECK(forever.expired() == 0);
}

RC_TEST("a consumer that is always behind serves older and older data, until a lifespan puts a ceiling on how old") {
  std::cout << "  published every 10 ms, taken every 20 ms, for 10 s\n  lifespan         served   expired   oldest age served (s)\n";
  double oldest_without = 0.0;
  double oldest_with = 1e9;
  for (double lifespan : {0.0, 0.5, 0.1}) {
    ExpiringQueue q(lifespan);
    int served = 0;
    double oldest = 0.0;
    for (int tick = 0; tick < 1000; ++tick) {
      const double now = tick * 0.01;
      q.push("m", now);
      if (tick % 2 == 1) {
        std::string payload;
        double age = 0.0;
        if (q.take(now, payload, age)) {
          ++served;
          if (age > oldest) oldest = age;
        }
      }
    }
    std::cout << "  " << std::setw(8) << std::fixed << std::setprecision(1) << lifespan << " s   " << std::setw(6) << served << "   " << std::setw(7)
              << q.expired() << "   " << std::setw(21) << std::setprecision(2) << oldest << "\n";
    if (lifespan == 0.0) oldest_without = oldest;
    if (lifespan == 0.1) oldest_with = oldest;
  }
  std::cout << "\n";
  RC_CHECK(oldest_without > 2.0);
  RC_CHECK(oldest_with <= 0.1 + 1e-9);
  RC_CHECK(oldest_with > 0.0);
}
