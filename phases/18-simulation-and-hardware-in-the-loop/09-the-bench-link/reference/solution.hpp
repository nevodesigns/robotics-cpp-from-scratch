#ifndef LESSON_SOLUTION_HPP
#define LESSON_SOLUTION_HPP

#include <cmath>

#include <rc/sim/plant.hpp>

using rc::sim::SimMotor;

// What the controller sends: a number for each command, so that an old one arriving late can be told from a new one.
struct CommandFrame {
  int seq = 0;
  double value = 0.0;
};

// What the emulator sends back each tick: which tick it is, which command it used, and where the plant is.
struct MeasureFrame {
  int tick = 0;
  int command_seq = 0;
  double position = 0.0;
};

// The plant side of a bench: a simulated motor that acts as if it were hardware at the other end of a link. It acts on the newest command it has been
// sent and holds it between commands, as a motor driver does. If no new command has come for more than `watchdog` ticks it stops acting on the old one and
// uses zero: a motor left running on a command from a controller that has gone quiet is the dangerous way to fail. A watchdog of zero means none.
class PlantEmulator {
 public:
  PlantEmulator(double gain, double tau, double limit, int watchdog) : motor_(gain, tau, limit), watchdog_(watchdog < 0 ? 0 : watchdog) {}

  // A command arrived. One older than the newest already seen (it was overtaken on the way) is ignored.
  void on_command(const CommandFrame& c) {
    if (have_command_ && c.seq <= newest_seq_) {
      ++ignored_;
      return;
    }
    have_command_ = true;
    newest_seq_ = c.seq;
    command_ = c.value;
    last_command_tick_ = tick_;
  }

  // One step of `dt` seconds: act on the command (or on zero, with none yet or the watchdog expired) and report.
  MeasureFrame step(double dt) {
    double used = have_command_ ? command_ : 0.0;
    if (have_command_ && watchdog_ > 0 && tick_ - last_command_tick_ > watchdog_) {
      used = 0.0;
      ++watchdog_ticks_;
    }
    motor_.apply(used, dt);
    MeasureFrame m;
    m.tick = tick_++;
    m.command_seq = have_command_ ? newest_seq_ : 0;
    m.position = motor_.position();
    return m;
  }

  int ignored() const { return ignored_; }
  int watchdog_ticks() const { return watchdog_ticks_; }
  double velocity() const { return motor_.velocity(); }

 private:
  SimMotor motor_;
  int watchdog_ = 0;
  int tick_ = 0;
  bool have_command_ = false;
  int newest_seq_ = 0;
  double command_ = 0.0;
  int last_command_tick_ = 0;
  int ignored_ = 0;
  int watchdog_ticks_ = 0;
};

// The controller side of the bench. Measurements come back late, lost, twice, and out of order. It keeps the newest by the emulator's tick and refuses any
// that is not newer than what it has, so that a measurement overtaken on the way is never taken for the present.
class ControllerSide {
 public:
  // Returns whether this measurement was taken as the newest.
  bool accept(const MeasureFrame& m) {
    if (have_ && m.tick <= newest_tick_) {
      ++stale_;
      return false;
    }
    have_ = true;
    newest_tick_ = m.tick;
    position_ = m.position;
    ++fresh_;
    fresh_this_tick_ = true;
    return true;
  }

  // Call at the end of each tick. A tick in which nothing new came is counted as held: the controller went on with the last measurement.
  void end_tick() {
    if (!fresh_this_tick_) ++held_;
    fresh_this_tick_ = false;
  }

  double position() const { return position_; }
  int fresh() const { return fresh_; }
  int stale() const { return stale_; }
  int held() const { return held_; }

 private:
  bool have_ = false;
  int newest_tick_ = 0;
  double position_ = 0.0;
  bool fresh_this_tick_ = false;
  int fresh_ = 0;
  int stale_ = 0;
  int held_ = 0;
};

#endif  // LESSON_SOLUTION_HPP
