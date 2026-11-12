#include <rc/test/rc_test.hpp>

#include <cmath>
#include <iomanip>
#include <iostream>

#include "solution.hpp"

namespace {

RemoteResult outage(double speed, double timeout, int delay = 0) {
  RemoteConfig c;
  c.speed = speed;
  c.timeout = timeout;
  c.delay_frames = delay;
  c.outage_from = 10.0;
  c.outage_to = 20.0;
  return run_remote(c, 40.0);
}

RemoteResult lossy(double loss, double timeout) {
  RemoteConfig c;
  c.loss = loss;
  c.timeout = timeout;
  return run_remote(c, 3600.0);
}

}  // namespace

RC_TEST("the timeout that fits a distance is what is left after the braking, and none fits when the braking alone is too long") {
  RC_CHECK_NEAR(max_timeout(1.0, 1.0, 1.5), (1.0 - 1.0 / 3.0) / 1.0, 1e-12);
  RC_CHECK_NEAR(max_timeout(3.0, 2.0, 1.5), (3.0 - 4.0 / 3.0) / 2.0, 1e-12);
  RC_CHECK(max_timeout(1.0, 2.0, 1.5) == 0.0);
  RC_CHECK(max_timeout(1.0, 0.0, 1.5) == 0.0);
  RC_CHECK(max_timeout(1.0, 1.0, 0.0) == 0.0);
  RC_CHECK_NEAR(chance_all_lost(0.1, 2), 0.01, 1e-12);
  RC_CHECK_NEAR(chance_all_lost(0.4, 6), 0.004096, 1e-12);
  RC_CHECK(chance_all_lost(0.3, 0) == 1.0);
  RC_CHECK(chance_all_lost(1.5, 3) == 1.0);
  RC_CHECK(chance_all_lost(-0.5, 3) == 0.0);
  RC_CHECK(should_reset(5.0, 6.0, 1.0));
  RC_CHECK(!should_reset(5.0, 5.9, 1.0));
  RC_CHECK(!should_reset(-1.0, 100.0, 1.0));
  RC_CHECK(!should_reset(5.0, 100.0, -1.0));
  std::cout << "  to stop within 1 m of the heartbeat going, at 1 m/s and 1.5 m/s^2 of braking, the timeout is at most " << std::fixed << std::setprecision(3) << max_timeout(1.0, 1.0, 1.5) << " s; at 2 m/s the braking alone is " << 4.0 / 3.0 << " m and no timeout is short enough\n\n";
}

RC_TEST("a link that goes out stops the robot after the timeout and the braking, and a longer timeout is a longer way") {
  const double timeouts[] = {0.1, 0.3, 1.0};
  RemoteResult slow[3];
  RemoteResult fast[3];
  std::cout << "  the link out from 10 s, the robot stopped at 1 m/s and at 2 m/s (1.5 m/s^2 of braking):\n  timeout    travel at 1 m/s    travel at 2 m/s\n";
  for (int i = 0; i < 3; ++i) {
    slow[i] = outage(1.0, timeouts[i]);
    fast[i] = outage(2.0, timeouts[i]);
    std::cout << "  " << std::fixed << std::setprecision(1) << std::setw(5) << timeouts[i] << " s      " << std::setprecision(3) << std::setw(6) << slow[i].travel_after_outage << " m          " << std::setw(6) << fast[i].travel_after_outage << " m\n";
    RC_CHECK(slow[i].stops_in_outage == 1 && slow[i].stops_otherwise == 0);
  }
  std::cout << "\n";
  RC_CHECK(slow[0].travel_after_outage < slow[1].travel_after_outage && slow[1].travel_after_outage < slow[2].travel_after_outage);
  RC_CHECK(slow[1].travel_after_outage - slow[0].travel_after_outage > 0.18 && slow[1].travel_after_outage - slow[0].travel_after_outage < 0.22);      // 0.2 s more at 1 m/s
  RC_CHECK(fast[0].travel_after_outage > slow[0].travel_after_outage);
  RC_CHECK(fast[2].travel_after_outage > 3.0);
  RC_CHECK(slow[1].time_to_stop > 0.9 && slow[1].time_to_stop < 1.0);
}

RC_TEST("when the link comes back the robot does not go on by itself, and it goes on when a person presses reset after a second of good link") {
  RemoteConfig never;
  never.outage_from = 10.0;
  never.outage_to = 20.0;
  never.reset_after = -1.0;
  const RemoteResult stays = run_remote(never, 40.0);
  const RemoteResult resets = outage(1.0, 0.3);
  std::cout << "  the link back at 20 s: with no reset the robot is " << (stays.moving_at_end ? "moving" : "still") << " at 40 s; with a reset after 1 s of good link it is " << (resets.moving_at_end ? "moving" : "still") << " (" << resets.resets << " resets in all, the first at the start)\n\n";
  RC_CHECK(!stays.moving_at_end);
  RC_CHECK(stays.resets == 0);
  RC_CHECK(resets.moving_at_end);
  RC_CHECK(resets.resets == 2);
}

RC_TEST("a delayed link makes the robot go on obeying old commands, so the stop comes later by the delay times the speed") {
  const int delays[] = {0, 10, 30, 100};
  RemoteResult r[4];
  std::cout << "  the link out from 10 s, a timeout of 0.3 s, at 1 m/s, by the delay of the link:\n";
  for (int i = 0; i < 4; ++i) {
    r[i] = outage(1.0, 0.3, delays[i]);
    std::cout << "  " << std::setw(5) << delays[i] * 10 << " ms: " << std::fixed << std::setprecision(3) << r[i].travel_after_outage << " m\n";
  }
  std::cout << "\n";
  for (int i = 1; i < 4; ++i) RC_CHECK_NEAR(r[i].travel_after_outage - r[0].travel_after_outage, delays[i] * 0.01, 0.02);
  RC_CHECK(r[3].travel_after_outage > 1.5);
}

RC_TEST("a lossy link makes a short timeout stop the robot for nothing, hundreds of times an hour, and a timeout of ten heartbeats almost never does") {
  struct Case { double loss; double timeout; };
  const Case cases[] = {{0.05, 0.1}, {0.05, 0.2}, {0.2, 0.1}, {0.2, 0.2}, {0.2, 0.3}, {0.2, 0.5}, {0.4, 0.2}, {0.4, 0.5}};
  RemoteResult r[8];
  std::cout << "  one hour of driving at 1 m/s, a heartbeat every 50 ms:\n  loss    timeout   false stops   stopped for   chance a run is all lost\n";
  for (int i = 0; i < 8; ++i) {
    r[i] = lossy(cases[i].loss, cases[i].timeout);
    std::cout << "  " << std::fixed << std::setprecision(2) << cases[i].loss << "    " << std::setprecision(1) << cases[i].timeout << " s    " << std::setw(6) << r[i].stops_otherwise << "        " << std::setw(5) << 100.0 * r[i].time_stopped / 3600.0 << " %     "
              << std::setprecision(5) << chance_all_lost(cases[i].loss, static_cast<int>(std::lround(cases[i].timeout / 0.05))) << "\n";
  }
  std::cout << "\n";
  RC_CHECK(r[0].stops_otherwise > 100);                                    // 5 percent loss and two heartbeats
  RC_CHECK(r[1].stops_otherwise == 0);
  RC_CHECK(r[2].stops_otherwise > 1000);
  RC_CHECK(100.0 * r[2].time_stopped / 3600.0 > 40.0);                    // stopped for half the hour
  RC_CHECK(r[3].stops_otherwise > 50 && r[3].stops_otherwise < r[2].stops_otherwise);
  RC_CHECK(r[4].stops_otherwise < 10);
  RC_CHECK(r[5].stops_otherwise == 0);
  RC_CHECK(r[6].stops_otherwise > 500);
  RC_CHECK(r[7].stops_otherwise < 20);
  RC_CHECK(r[7].distance > 3500.0);
  RC_CHECK(r[0].time_stopped > 0.0 && r[1].time_stopped < 5.0);
}

RC_TEST("a timeout chosen from a distance of one metre at one metre a second stops it inside it, and at two a second no timeout can") {
  const double t = max_timeout(1.0, 1.0, 1.5);
  const RemoteResult ok = outage(1.0, t);
  const RemoteResult too_fast = outage(2.0, 0.1);
  std::cout << "  the timeout " << std::fixed << std::setprecision(3) << t << " s at 1 m/s: the robot went " << ok.travel_after_outage << " m after the link went; at 2 m/s even a timeout of 0.1 s went " << too_fast.travel_after_outage << " m\n\n";
  RC_CHECK(ok.travel_after_outage < 1.0);
  RC_CHECK(ok.travel_after_outage > 0.8);
  RC_CHECK(too_fast.travel_after_outage > 1.0);
  RC_CHECK(max_timeout(1.0, 2.0, 1.5) == 0.0);
}
