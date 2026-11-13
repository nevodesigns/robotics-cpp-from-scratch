#include <rc/test/rc_test.hpp>

#include <cmath>

#include "solution.hpp"
#include <iomanip>
#include <iostream>

static void rec(const char* key, double value, int digits = 3) {
  std::cout << "@" << key << " " << std::fixed << std::setprecision(digits) << value << std::endl;
  std::cout.unsetf(std::ios::floatfield);
}
[[maybe_unused]] static void recs(const char* key, const std::string& value) { std::cout << "@" << key << " " << value << std::endl; }


namespace {

unsigned long long g_state = 5;
double uni() {
  g_state = g_state * 6364136223846793005ULL + 1442695040888963407ULL;
  return static_cast<double>(g_state >> 11) / 9007199254740992.0;
}
double gauss() {
  double s = 0.0;
  for (int i = 0; i < 12; ++i) s += uni();
  return s - 6.0;
}

const double kDt = 0.002;     // 500 Hz
const double kW = 29.4;       // N on the foot in stance (12 kg on four feet)

// Per cycle of 0.5 s: stance for the first 0.25 s (force ramps up in 20 ms and down in 20 ms), then a swing in which the leg's acceleration gives a spike of 12 N for 30 ms starting 5 ms after the lift is commanded.
double force_at(double t_cycle, double noise) {
  double f = 0.0;
  if (t_cycle < 0.02) {
    f = kW * t_cycle / 0.02;
  } else if (t_cycle < 0.23) {
    f = kW;
  } else if (t_cycle < 0.25) {
    f = kW * (0.25 - t_cycle) / 0.02;
  } else if (t_cycle >= 0.255 && t_cycle < 0.285) {
    f = 12.0;   // the leg's own acceleration just after it has been told to lift
  }
  return f + noise * gauss();
}

struct Out {
  int transitions = 0;
  double td_ms = 0.0, lo_ms = 0.0;
};

Out run(const ContactParams& p, double noise, int cycles) {
  ContactState s;
  Out o;
  bool prev = false;
  g_state = 5;
  for (int i = 0; i < static_cast<int>(cycles * 0.5 / kDt); ++i) {
    const double t = i * kDt, tc = std::fmod(t, 0.5);
    const double since_lift = tc >= 0.25 ? tc - 0.25 : 1.0;
    const bool d = contact_update(s, force_at(tc, noise), since_lift, p);
    if (d != prev) {
      ++o.transitions;
      if (cycles >= 2 && i > 250 && i < 500 && d) o.td_ms = (tc - 0.0) * 1000.0;
      if (d == false && i > 100 && o.lo_ms == 0.0) o.lo_ms = (tc - 0.25) * 1000.0;
      prev = d;
    }
  }
  return o;
}

// A foot that grazes the ground: a force that hovers around the on threshold for 0.4 s.
int graze_changes(const ContactParams& p) {
  ContactState s;
  g_state = 9;
  int changes = 0;
  bool prev = true;
  s.down = true;
  for (int i = 0; i < 200; ++i) {
    const double f = 8.0 + 2.0 * gauss();
    const bool d = contact_update(s, f, 1.0, p);
    if (d != prev) {
      ++changes;
      prev = d;
    }
  }
  return changes;
}

}  // namespace

RC_TEST("touchdown and lift-off are believed after a run of readings, and a reading on the wrong side resets the run") {
  ContactParams p;
  p.need = 3;
  ContactState s;
  RC_CHECK(!contact_update(s, 20.0, 1.0, p));
  RC_CHECK(!contact_update(s, 20.0, 1.0, p));
  RC_CHECK(!contact_update(s, 1.0, 1.0, p));    // resets
  RC_CHECK(!contact_update(s, 20.0, 1.0, p));
  RC_CHECK(!contact_update(s, 20.0, 1.0, p));
  RC_CHECK(contact_update(s, 20.0, 1.0, p));
  RC_CHECK(contact_update(s, 6.0, 1.0, p));     // between the thresholds: still down
  RC_CHECK(contact_update(s, 1.0, 1.0, p));
  RC_CHECK(contact_update(s, 1.0, 1.0, p));
  RC_CHECK(!contact_update(s, 1.0, 1.0, p));
  // inside the blank a large force does not count
  ContactState b;
  for (int i = 0; i < 10; ++i) RC_CHECK(!contact_update(b, 30.0, 0.01, p));
}

RC_TEST("a stepping foot gives one touchdown and one lift-off a step, found within a few readings") {
  ContactParams p;
  const double noise = 1.0;
  const int cycles = 6;
  const Out o = run(p, noise, cycles);
  rec("rate", 1.0 / kDt, 0);
  rec("noise_n", noise, 0);
  rec("w", kW, 1);
  rec("f_on", p.f_on, 0);
  rec("f_off", p.f_off, 0);
  rec("need", p.need, 0);
  rec("td_ms", o.td_ms, 0);
  rec("lo_ms", o.lo_ms, 0);
  rec("trans", o.transitions, 0);
  rec("cycles", cycles, 0);
  rec("trans_ok", 2 * cycles, 0);
  RC_CHECK(o.transitions == 2 * cycles);
  RC_CHECK(o.td_ms > 0.0 && o.td_ms < 40.0);
  RC_CHECK(o.lo_ms > 0.0 && o.lo_ms < 40.0);
}

RC_TEST("the blank keeps the lift-off spike out, and two thresholds keep a grazing foot from flapping") {
  ContactParams p, noblank, one;
  noblank.blank = 0.0;
  one.f_off = one.f_on;
  const Out with = run(p, 1.0, 6);
  const Out without = run(noblank, 1.0, 6);
  rec("trans_noblank", without.transitions, 0);
  RC_CHECK(without.transitions > with.transitions);
  const int two = graze_changes(p), single = graze_changes(one);
  rec("graze_one", single, 0);
  rec("graze_two", two, 0);
  RC_CHECK(single > two);
}
