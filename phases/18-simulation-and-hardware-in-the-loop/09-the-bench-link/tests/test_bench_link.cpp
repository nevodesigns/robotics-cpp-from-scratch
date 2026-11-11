#include <rc/test/rc_test.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <vector>

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

// One direction of the link: each frame is lost with a chance, and otherwise arrives after a delay of a base number of ticks plus up to `jitter` more.
// Frames come out in the order they arrive, which is not the order they were sent when the jitter lets one overtake another.
template <class Frame>
struct Pipe {
  struct Item {
    int arrives;
    int order;
    Frame frame;
  };
  std::vector<Item> items;
  Rng rng;
  double loss;
  int base;
  int jitter;
  int sent = 0;
  bool dead = false;

  Pipe(std::uint32_t seed, double l, int b, int j) : rng{seed}, loss(l), base(b), jitter(j) {}

  void send(const Frame& f, int now) {
    if (dead || rng.uniform() < loss) return;
    const int extra = jitter > 0 ? static_cast<int>(rng.uniform() * (jitter + 1)) : 0;
    items.push_back(Item{now + base + extra, sent++, f});
  }

  std::vector<Frame> deliver(int now) {
    std::vector<Item> due;
    std::vector<Item> rest;
    for (const Item& it : items) (it.arrives <= now ? due : rest).push_back(it);
    items = rest;
    std::sort(due.begin(), due.end(), [](const Item& a, const Item& b) { return a.arrives != b.arrives ? a.arrives < b.arrives : a.order < b.order; });
    std::vector<Frame> out;
    for (const Item& it : due) out.push_back(it.frame);
    return out;
  }
};

struct Outcome {
  double final_error = 0.0;
  double position_at_outage_end = 0.0;
  int held = 0;
  int stale = 0;
  int fresh = 0;
  int ignored_commands = 0;
  int watchdog_ticks = 0;
  int went_back_in_time = 0;       // times a naive controller replaced a newer measurement with an older one
  double naive_final_error = 0.0;
  double mean_error = 0.0;           // over the whole run, of the controller side's view
  double naive_mean_error = 0.0;
  double age_checked = 0.0;           // how many ticks old, on average, the measurement the controller acts on is
  double age_naive = 0.0;
};

// 3 seconds of a position loop at 100 Hz over a link. `target_at_95` steps the target to -1 at tick 95, for the outage test. `outage` kills both directions from tick 100 to 200.
Outcome run(double loss, int base, int jitter, int watchdog, bool target_at_95, bool outage, std::uint32_t seed) {
  PlantEmulator emu(2.0, 0.1, 12.0, watchdog);
  ControllerSide ctl;
  Pipe<CommandFrame> down(seed, loss, base, jitter);
  Pipe<MeasureFrame> up(seed + 1, loss, base, jitter);
  Outcome o;
  int seq = 0;
  double naive_position = 0.0;
  int naive_newest = -1;
  double all_err = 0.0;
  double naive_all_err = 0.0;
  MeasureFrame last_frame;
  int checked_tick = -1;
  int naive_tick = -1;
  double age_sum_checked = 0.0;
  double age_sum_naive = 0.0;
  double worst_tail = 0.0;
  int tail_n = 0;
  double naive_tail = 0.0;
  for (int tick = 0; tick < 300; ++tick) {
    down.dead = up.dead = outage && tick >= 100 && tick < 200;
    const double target = (target_at_95 && tick >= 95) ? -1.0 : 1.0;
    down.send(CommandFrame{++seq, 8.0 * (target - ctl.position())}, tick);
    for (const CommandFrame& c : down.deliver(tick)) emu.on_command(c);
    last_frame = emu.step(0.01);
    up.send(last_frame, tick);
    for (const MeasureFrame& m : up.deliver(tick)) {
      if (ctl.accept(m)) checked_tick = m.tick;
      naive_tick = m.tick;
      if (m.tick <= naive_newest) {
        ++o.went_back_in_time;                                  // older than one already seen: a naive controller takes it as the present
      } else {
        naive_newest = m.tick;
      }
      naive_position = m.position;
    }
    ctl.end_tick();
    if (tick == 199) o.position_at_outage_end = last_frame.position;                // where the plant really is
    age_sum_checked += tick - checked_tick;
    age_sum_naive += tick - naive_tick;
    all_err += std::fabs(target - ctl.position());
    naive_all_err += std::fabs(target - naive_position);
    if (tick >= 250) {
      worst_tail += std::fabs(target - ctl.position());
      naive_tail += std::fabs(target - naive_position);
      ++tail_n;
    }
  }
  o.age_checked = age_sum_checked / 300.0;
  o.age_naive = age_sum_naive / 300.0;
  o.mean_error = all_err / 300.0;
  o.naive_mean_error = naive_all_err / 300.0;
  o.final_error = worst_tail / tail_n;
  o.naive_final_error = naive_tail / tail_n;
  o.held = ctl.held();
  o.stale = ctl.stale();
  o.fresh = ctl.fresh();
  o.ignored_commands = emu.ignored();
  o.watchdog_ticks = emu.watchdog_ticks();
  return o;
}

}  // namespace

RC_TEST("the emulator acts on the newest command and holds it, and one that was overtaken on the way is ignored") {
  PlantEmulator emu(2.0, 0.1, 12.0, 0);
  const MeasureFrame idle = emu.step(0.01);                               // no command yet: zero
  RC_CHECK(idle.tick == 0 && idle.command_seq == 0);
  emu.on_command(CommandFrame{2, 3.0});
  emu.on_command(CommandFrame{1, -9.0});                                  // sent before the other, arrived after it
  for (int i = 0; i < 100; ++i) emu.step(0.01);                           // held for a second with no new command
  const MeasureFrame m = emu.step(0.01);
  std::cout << "  commands 2 (value 3) then 1 (value -9) arriving in that order: the plant used command " << m.command_seq << ", position " << std::fixed << std::setprecision(3) << m.position
            << ", " << emu.ignored() << " ignored\n\n";
  RC_CHECK(m.command_seq == 2);
  RC_CHECK(m.position > 3.0);
  RC_CHECK(emu.ignored() == 1);
  RC_CHECK(m.tick == 101);
  RC_CHECK(emu.velocity() > 5.0);
}

RC_TEST("with a watchdog the emulator stops acting on a command that has gone stale, and without one it keeps going") {
  PlantEmulator guarded(2.0, 0.1, 12.0, 5);
  PlantEmulator unguarded(2.0, 0.1, 12.0, 0);
  guarded.on_command(CommandFrame{1, 6.0});
  unguarded.on_command(CommandFrame{1, 6.0});
  MeasureFrame g;
  MeasureFrame u;
  for (int i = 0; i < 100; ++i) {
    g = guarded.step(0.01);
    u = unguarded.step(0.01);
  }
  std::cout << "  one command of 6, then silence for a second: with a watchdog of 5 ticks the plant ended at " << std::fixed << std::setprecision(3) << g.position << " (speed "
            << guarded.velocity() << "), without one at " << u.position << " (speed " << unguarded.velocity() << "); watchdog ticks " << guarded.watchdog_ticks() << "\n\n";
  RC_CHECK(u.position > 5.0 * g.position);
  RC_CHECK(std::fabs(guarded.velocity()) < 0.01);
  RC_CHECK(unguarded.velocity() > 10.0);
  RC_CHECK(guarded.watchdog_ticks() > 80);
  RC_CHECK(unguarded.watchdog_ticks() == 0);
  guarded.on_command(CommandFrame{2, 6.0});                              // a new command wakes it again
  const double before = guarded.step(0.01).position;
  for (int i = 0; i < 20; ++i) guarded.step(0.01);
  RC_CHECK(guarded.step(0.01).position > before);
}

RC_TEST("the controller keeps the newest measurement by the emulator's tick, and counts those that were overtaken or repeated") {
  ControllerSide c;
  RC_CHECK(c.accept(MeasureFrame{5, 1, 0.5}));
  RC_CHECK(c.accept(MeasureFrame{7, 2, 0.7}));
  RC_CHECK(!c.accept(MeasureFrame{6, 2, 0.6}));                          // overtaken
  RC_CHECK(!c.accept(MeasureFrame{7, 2, 0.7}));                          // a repeat
  RC_CHECK_NEAR(c.position(), 0.7, 1e-12);
  RC_CHECK(c.fresh() == 2);
  RC_CHECK(c.stale() == 2);
  c.end_tick();                                                          // something new came: not held
  c.end_tick();                                                          // nothing came
  c.end_tick();
  std::cout << "  measurements 5, 7, 6, 7: kept " << c.fresh() << ", refused " << c.stale() << ", then two quiet ticks: held " << c.held() << "\n\n";
  RC_CHECK(c.held() == 2);
}

RC_TEST("over a clean link the loop settles, and over a lossy one it still does, with more ticks spent on a measurement that was not new") {
  std::cout << "  position loop to 1, 3 s at 100 Hz, link delay 1 tick, 20 trials each at a different seed would be too long: one run per row\n"
            << "  loss each way   error in the last half second   measurements held   commands ignored\n";
  double clean = 0.0;
  double bad = 0.0;
  int held_clean = -1;
  int held_bad = -1;
  for (double loss : {0.0, 0.05, 0.1, 0.2, 0.4}) {
    const Outcome o = run(loss, 1, 0, 0, false, false, 11);
    std::cout << "  " << std::setw(13) << std::fixed << std::setprecision(2) << loss << "   " << std::setw(28) << std::setprecision(4) << o.final_error << "   " << std::setw(17) << o.held << "   "
              << std::setw(16) << o.ignored_commands << "\n";
    if (loss == 0.0) {
      clean = o.final_error;
      held_clean = o.held;
    }
    if (loss == 0.4) {
      bad = o.final_error;
      held_bad = o.held;
    }
  }
  std::cout << "\n";
  RC_CHECK(clean < 0.01);
  RC_CHECK(held_clean <= 2);                                          // the first tick or two, before the first measurement has come back
  RC_CHECK(held_bad > 50);
  RC_CHECK(bad < 0.2);
}

RC_TEST("when the link dies the controller's last command is the one the plant would keep acting on, which is why the watchdog exists") {
  const Outcome hold = run(0.0, 1, 0, 0, true, true, 5);
  const Outcome guard = run(0.0, 1, 0, 5, true, true, 5);
  std::cout << "  the target steps to -1 at 0.95 s and the link dies from 1.0 s to 2.0 s (both ways)\n  plant side           position when the link comes back   ticks on watchdog\n  holds the last command   "
            << std::fixed << std::setprecision(2) << std::setw(14) << hold.position_at_outage_end << "                   " << std::setw(5) << hold.watchdog_ticks << "\n  watchdog of 5 ticks      "
            << std::setw(14) << guard.position_at_outage_end << "                   " << std::setw(5) << guard.watchdog_ticks << "\n\n";
  RC_CHECK(hold.position_at_outage_end < -15.0);                       // it ran on, away from where anyone wanted it
  RC_CHECK(std::fabs(guard.position_at_outage_end) < 3.0);              // it coasts a little on the way down, and not for a second
  RC_CHECK(guard.watchdog_ticks > 80);
  RC_CHECK(hold.watchdog_ticks == 0);
}

RC_TEST("on a link where frames overtake each other, taking each measurement as it arrives sends the controller back in time, and the tick check does not") {
  const Outcome o = run(0.0, 1, 3, 5, false, false, 21);
  std::cout << "  delay 1 to 4 ticks: measurements that arrived older than one already seen: " << o.went_back_in_time << "; refused by the controller side " << o.stale
            << "; commands overtaken and ignored by the emulator " << o.ignored_commands << "\n  error in the last half second: with the tick check " << std::fixed << std::setprecision(4) << o.final_error
            << ", taking each as it arrives " << o.naive_final_error << "\n  error over the whole run: with the tick check " << o.mean_error << ", taking each as it arrives " << o.naive_mean_error << "\n  how old, in ticks, the measurement acted on was on average: with the tick check " << std::setprecision(2) << o.age_checked
            << ", taking each as it arrives " << o.age_naive << "\n\n";
  RC_CHECK(o.went_back_in_time > 20);
  RC_CHECK(o.stale == o.went_back_in_time);
  RC_CHECK(o.ignored_commands > 10);
  RC_CHECK(o.mean_error <= o.naive_mean_error);
  RC_CHECK(o.age_checked < o.age_naive);
}
