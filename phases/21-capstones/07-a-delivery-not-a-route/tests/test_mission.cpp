#include <rc/test/rc_test.hpp>

#include <iomanip>
#include <iostream>

#include "solution.hpp"

namespace {

MissionResult run(double battery, double actual, bool check = true, const MissionWorld& base = MissionWorld()) {
  MissionConfig c;
  c.check_on_the_way = check;
  MissionWorld w = base;
  w.battery_start = battery;
  w.wh_per_m_actual = actual;
  return run_mission(c, w, 2000.0);
}

const char* word(const MissionResult& r) { return mission_state_name(r.final_state); }

}  // namespace

RC_TEST("a round trip costs twice the distance, and the mission starts only if the battery covers it and the reserve, and turns back only when what is left would not get it home") {
  RC_CHECK_NEAR(energy_for(100.0, 0.1), 10.0, 1e-12);
  RC_CHECK_NEAR(energy_for(-100.0, 0.1), 10.0, 1e-12);
  RC_CHECK(energy_for(50.0, -1.0) == 0.0);
  RC_CHECK(can_start(25.0, 100.0, 0.1, 5.0));                                // 20 for the round trip and 5 over: exactly enough
  RC_CHECK(!can_start(24.9, 100.0, 0.1, 5.0));
  RC_CHECK(!can_start(24.0, 100.0, 0.1, 5.0));
  RC_CHECK(!must_turn_back(26.0, 100.0, 0.1, 5.0));                          // 26 - 10 = 16
  RC_CHECK(must_turn_back(14.9, 100.0, 0.1, 5.0));
  RC_CHECK(!must_turn_back(15.0, 100.0, 0.1, 5.0));
  RC_CHECK(!must_turn_back(5.0, 0.0, 0.1, 5.0));                             // at home with the reserve: nothing to get home
  RC_CHECK(must_turn_back(4.0, 0.0, 0.1, 5.0));
  RC_CHECK(std::string(mission_state_name(MissionState::to_drop)) == "to drop");
  RC_CHECK(std::string(mission_state_name(MissionState::help)) == "needs help");
  std::cout << "  100 m to the drop at 0.1 Wh/m: 20 Wh for the round trip, so 25 Wh with a 5 Wh reserve; the robot refuses to start on 24.9\n\n";
}

RC_TEST("with enough battery the whole mission is done, and with too little it is refused before the robot moves") {
  const MissionResult full = run(40.0, 0.1);
  const MissionResult exact = run(26.0, 0.1);
  const MissionResult short_of = run(24.0, 0.1);
  std::cout << "  the delivery of 100 m, 0.1 Wh/m expected and used:\n";
  std::cout << "  40 Wh: " << word(full) << ", delivered, " << std::fixed << std::setprecision(2) << full.battery_left << " Wh left after " << std::setprecision(0) << full.time << " s\n";
  std::cout << "  26 Wh: " << word(exact) << ", " << std::setprecision(2) << exact.battery_left << " Wh left\n";
  std::cout << "  24 Wh: " << word(short_of) << " at once, at " << std::setprecision(0) << short_of.time << " s, with the load not taken\n\n";
  RC_CHECK(full.final_state == MissionState::done && full.delivered && !full.turned_back);
  RC_CHECK(full.battery_left > 19.9 && full.battery_left < 20.1);
  RC_CHECK(exact.final_state == MissionState::done && exact.battery_left > 5.9);
  RC_CHECK(short_of.final_state == MissionState::aborted && !short_of.delivered && short_of.position_end == 0.0);
  RC_CHECK(short_of.time < 1.0);
}

RC_TEST("a robot that is using more than it expects turns back in time if it checks on the way, and one that checks only at the start is stranded with the road ahead of it") {
  const double rates[] = {0.10, 0.12, 0.15, 0.18, 0.25};
  MissionResult with[5];
  MissionResult without[5];
  std::cout << "  26 Wh at the start, 0.1 Wh/m expected, the Wh/m it really uses down the left:\n  actual   checking on the way                      checking at the start only\n";
  for (int i = 0; i < 5; ++i) {
    with[i] = run(26.0, rates[i], true);
    without[i] = run(26.0, rates[i], false);
    std::cout << "  " << std::fixed << std::setprecision(2) << rates[i] << "     " << std::left << std::setw(10) << word(with[i]) << std::right << (with[i].stranded ? " STRANDED " + std::to_string(with[i].position_end).substr(0, 4) + " m out" : " home, " + std::to_string(with[i].battery_left).substr(0, 4) + " Wh left")
              << std::string(with[i].stranded ? 12 : 12, ' ') << std::left << std::setw(10) << word(without[i]) << std::right << (without[i].stranded ? " STRANDED " + std::to_string(without[i].position_end).substr(0, 4) + " m out" : " home, " + std::to_string(without[i].battery_left).substr(0, 4) + " Wh left") << "\n";
  }
  std::cout << "\n";
  RC_CHECK(with[0].final_state == MissionState::done && without[0].final_state == MissionState::done);
  RC_CHECK(with[1].final_state == MissionState::aborted && !with[1].delivered);          // it turns back although it would have made it: the price of the check
  RC_CHECK(without[1].final_state == MissionState::done && without[1].delivered);
  RC_CHECK(with[2].final_state == MissionState::aborted && !with[2].stranded && with[2].battery_left > 0.0);
  RC_CHECK(without[2].stranded && without[2].position_end > 20.0);
  RC_CHECK(with[3].stranded && without[3].stranded);
  RC_CHECK(with[3].position_end < 0.2 * without[3].position_end);                         // beyond what the reserve covers, but nearer home
  RC_CHECK(with[4].stranded && without[4].stranded);
}

RC_TEST("a road blocked for eight seconds is waited out, one blocked for twelve sends the robot home with its load, and one blocked both ways asks for help") {
  MissionConfig c;
  MissionWorld a;
  a.blocked_from = 50.0;
  a.blocked_to = 58.0;
  MissionWorld b = a;
  b.blocked_to = 62.0;
  MissionWorld both;
  both.blocked_from = 50.0;
  both.blocked_to = 1e9;
  both.blocked_back_from = 70.0;
  const MissionResult ra = run_mission(c, a, 2000.0);
  const MissionResult rb = run_mission(c, b, 2000.0);
  const MissionResult rc2 = run_mission(c, both, 2000.0);
  std::cout << "  blocked 8 s: " << word(ra) << ", delivered, waited " << std::fixed << std::setprecision(1) << ra.waited << " s\n";
  std::cout << "  blocked 12 s: " << word(rb) << ", not delivered, waited " << rb.waited << " s, back at home with " << std::setprecision(1) << rb.battery_left << " Wh\n";
  std::cout << "  blocked ahead and behind: " << word(rc2) << " at " << rc2.position_end << " m from home\n\n";
  RC_CHECK(ra.final_state == MissionState::done && ra.delivered);
  RC_CHECK(ra.waited > 7.9 && ra.waited < 8.2);
  RC_CHECK(rb.final_state == MissionState::aborted && !rb.delivered && rb.turned_back && rb.position_end == 0.0);
  RC_CHECK(rb.waited > 10.0 && rb.waited < 10.3);
  RC_CHECK(rc2.final_state == MissionState::help);
  RC_CHECK(rc2.position_end > 0.0 && rc2.position_end < 100.0);
}

RC_TEST("a stop by the supervisor holds the mission where it is and the mission goes on from there, and one that never ends leaves it held for ever") {
  MissionConfig c;
  MissionWorld lost;
  lost.link_lost_from = 30.0;
  lost.link_lost_to = 60.0;
  MissionWorld gone;
  gone.link_lost_from = 30.0;
  const MissionResult clear = run_mission(c, MissionWorld(), 2000.0);
  const MissionResult held = run_mission(c, lost, 2000.0);
  const MissionResult forever = run_mission(c, gone, 600.0);
  std::cout << "  the link lost from 30 s to 60 s: the mission " << word(held) << ", delivered, held for " << std::fixed << std::setprecision(1) << held.held_time << " s, taking " << std::setprecision(0) << held.time << " s against " << clear.time << " s with no loss\n";
  std::cout << "  the link lost for ever: " << word(forever) << " at " << std::setprecision(1) << forever.position_end << " m, held for " << std::setprecision(0) << forever.held_time << " s of 600\n\n";
  RC_CHECK(held.final_state == MissionState::done && held.delivered);
  RC_CHECK(held.held_time > 30.0 && held.held_time < 32.0);
  RC_CHECK(held.time - clear.time > 30.0 && held.time - clear.time < 32.0);                // it lost the time it was held, and no distance
  RC_CHECK(held.battery_left > clear.battery_left - 0.01);                                  // holding costs no energy
  RC_CHECK(forever.final_state == MissionState::to_drop);
  RC_CHECK(forever.held_time > 500.0);
  RC_CHECK(!forever.delivered);
}

RC_TEST("over seventy two worlds the robot is never beyond the road and always ends in a state somebody can name, and a margin on the rate turns every stranding into a refusal") {
  struct Totals {
    int combos = 0;
    int within = 0;
    int stranded = 0;
    int ended = 0;
    int refused = 0;
    int delivered = 0;
  };
  Totals plain;
  Totals margin;
  Totals unchecked;
  const double batteries[] = {24.0, 26.0, 30.0, 40.0};
  const double rates[] = {0.10, 0.12, 0.15};
  for (double battery : batteries)
    for (double actual : rates)
      for (int blocked = 0; blocked < 3; ++blocked)
        for (int link = 0; link < 2; ++link) {
          MissionWorld w;
          w.battery_start = battery;
          w.wh_per_m_actual = actual;
          if (blocked == 1) {
            w.blocked_from = 50.0;
            w.blocked_to = 58.0;
          }
          if (blocked == 2) {
            w.blocked_from = 50.0;
            w.blocked_to = 70.0;
          }
          if (link == 1) {
            w.link_lost_from = 30.0;
            w.link_lost_to = 60.0;
          }
          MissionConfig a;
          MissionConfig b;
          b.rate_margin = 1.5;
          MissionConfig c;
          c.check_on_the_way = false;
          const MissionConfig cfgs[3] = {a, b, c};
          Totals* totals[3] = {&plain, &margin, &unchecked};
          for (int k = 0; k < 3; ++k) {
            const MissionResult r = run_mission(cfgs[k], w, 3000.0);
            Totals& t = *totals[k];
            ++t.combos;
            if (r.position_end >= 0.0 && r.position_end <= 100.0) ++t.within;
            if (r.stranded) ++t.stranded;
            const MissionState s = r.final_state;
            if (s == MissionState::done || s == MissionState::aborted || s == MissionState::help || r.stranded) ++t.ended;
            if (s == MissionState::aborted && r.time < 1.0) ++t.refused;
            if (r.delivered) ++t.delivered;
          }
        }
  std::cout << "  72 worlds (battery, consumption, blocked road, lost link):\n  planning with                     stranded   refused at the start   delivered\n";
  std::cout << "  the expected rate, checking       " << std::setw(5) << plain.stranded << std::setw(15) << plain.refused << std::setw(18) << plain.delivered << "\n";
  std::cout << "  1.5 times it, checking            " << std::setw(5) << margin.stranded << std::setw(15) << margin.refused << std::setw(18) << margin.delivered << "\n";
  std::cout << "  the expected rate, no check       " << std::setw(5) << unchecked.stranded << std::setw(15) << unchecked.refused << std::setw(18) << unchecked.delivered << "\n\n";
  RC_CHECK(plain.combos == 72 && margin.combos == 72 && unchecked.combos == 72);
  RC_CHECK(plain.within == 72 && margin.within == 72 && unchecked.within == 72);
  RC_CHECK(plain.ended == 72 && margin.ended == 72 && unchecked.ended == 72);
  RC_CHECK(unchecked.stranded > plain.stranded);
  RC_CHECK(plain.stranded > 0);
  RC_CHECK(margin.stranded == 0);
  RC_CHECK(margin.refused > plain.refused);
  RC_CHECK(margin.delivered < plain.delivered);
}
